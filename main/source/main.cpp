#include <emu/riscv/emulator.hpp>
#include <emu/literals.hpp>
#include <emu/devices/ram.hpp>

constexpr static std::uint8_t LinuxKernel[] = {
    #embed "Image"
};

auto main() -> int {
    using namespace ds;
    using namespace ds::literals;

    emu::dev::Ram ram(512_MiB);
    ram.write(0x00, LinuxKernel);

    emu::riscv::Emulator<1> emulator;
    emulator.address_space().map(0x0000'0000, &ram);
    while (emulator.step() == emu::riscv::StepResult::Ok) {

    }
}