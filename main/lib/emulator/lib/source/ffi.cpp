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

namespace ds::emu::ffi {

    using namespace ds;
    using namespace ds::literals;

    struct Emulator {
        Emulator() : ram(512_MiB) {
            std::setvbuf(stdout, nullptr, _IONBF, 0);

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

    extern "C" Emulator* create() {
        return new Emulator();
    }

    extern "C" void destroy(Emulator *emulator) {
        delete emulator;
    }

    extern "C" void step(Emulator *emulator) {
        emulator->step();
    }

}