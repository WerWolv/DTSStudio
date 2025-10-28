#include <emu/riscv/emulator.hpp>
#include <emu/literals.hpp>

constexpr static std::uint8_t LinuxKernel[] = {
    #embed "Image"
};

auto main() -> int {
    using namespace ds;
    using namespace ds::literals;

    emu::riscv::Emulator riscv(512_MiB);
    riscv.loadToRam(0x00, LinuxKernel);

    while (riscv.step() == emu::riscv::StepResult::Ok);
}