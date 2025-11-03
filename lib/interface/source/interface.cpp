#include <thread>
#include <emu/riscv/emulator.hpp>
#include <emu/literals.hpp>
#include <emu/devices/ram.hpp>
#include <emu/devices/8250_uart.hpp>
#include <emu/devices/riscv/mmu.hpp>

constexpr static std::uint8_t LinuxKernel[] = {
    #embed "Image"
};

constexpr static std::uint8_t DeviceTreeBlob[] = {
    #embed "device-tree.dtb"
};

constexpr static std::uint8_t InitRamFs[] = {
    #embed "initramfs.cpio"
};

extern "C" void send_terminal_data(const char* terminal_id, const char* text);

namespace ds::emu::ffi {

    using namespace ds;
    using namespace ds::literals;

    struct Emulator {
        Emulator() : ram(512_MiB) {
            std::setvbuf(stdout, nullptr, _IONBF, 0);

            uart8250.output_callback([](std::uint8_t c) {
                const std::array buffer = { char(c), char() };
                send_terminal_data("linux-terminal", buffer.data());
            });

            emulator.address_space().map(0x0000'0000, &ram);
            emulator.address_space().map(0xF400'0000, &uart8250);
            emulator.address_space().add_address_translator(&riscv_mmu);

            emulator.power_up();

            ram.write(0x00, LinuxKernel);

            constexpr static auto DeviceTreeBlobLoadAddress = 512_MiB - 1_MiB;
            constexpr static auto InitRamFsLoadAddress = 0x1F700000;
            ram.write(DeviceTreeBlobLoadAddress, DeviceTreeBlob);
            ram.write(InitRamFsLoadAddress, InitRamFs);
            emulator.cores()[0].a1() = DeviceTreeBlobLoadAddress;
        }

        void step() {
            emulator.step();
        }

    private:
        riscv::Emulator<1> emulator;
        dev::Ram ram;
        dev::UART8250 uart8250;
        dev::riscv::MMU<std::uint32_t> riscv_mmu;
    };

}

static std::jthread s_emulator_thread;

extern "C" void set_device_tree_source(const char *source, std::size_t length) {

}

extern "C" [[gnu::visibility("default")]] bool is_emulation_running() {
    return !s_emulator_thread.get_stop_token().stop_requested();
}

extern "C" [[gnu::visibility("default")]] void start_emulation() {
    s_emulator_thread = std::jthread([](const std::stop_token &stop_token) {
        ds::emu::ffi::Emulator emulator;
        while (!stop_token.stop_requested()) {
            emulator.step();
        }
    });
}

extern "C" [[gnu::visibility("default")]] void stop_emulation() {
    if (!is_emulation_running())
        return;

    s_emulator_thread.request_stop();
    s_emulator_thread.join();
}