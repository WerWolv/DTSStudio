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


auto main() -> int {
    using namespace ds;
    using namespace ds::literals;

    emu::riscv::Emulator<1> emulator;
    emulator.cores()[0].a1() = 512_MiB - 1_MiB;

    emu::dev::Ram ram(512_MiB);
    ram.write(0x00, LinuxKernel);
    ram.write(512_MiB - 1_MiB, DeviceTreeBlob);

    emu::dev::UART8250 uart8250;
    emu::dev::riscv::MMU<std::uint32_t> riscv_mmu;

    emulator.address_space().map(0x0000'0000, &ram);
    emulator.address_space().map(0xF400'0000, &uart8250);
    emulator.address_space().add_address_translator(&riscv_mmu);

    for (;;) {

        emulator.step();
    }
}