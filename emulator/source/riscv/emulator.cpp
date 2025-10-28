#include <emu/riscv/emulator.hpp>
#include <emu/riscv/instructions.hpp>

#include <array>
#include <cstdio>
#include <cstring>

namespace ds::emu::riscv {

    StepResult Emulator::handle_unimplemented(std::uint32_t instruction) {
        std::ignore = instruction;
        std::printf("Unimplemented instruction 0x%08x\n", instruction);
        return StepResult::Unimplemented;
    }

    StepResult Emulator::handle_system(std::uint32_t instruction) {
        std::ignore = instruction;
        printf("System instruction executed\n");
        return StepResult::Ok;
    }

    StepResult Emulator::handle_jal(std::uint32_t instruction) {
        const auto instr = instr::i::type::J(instruction);
        const auto offset = util::sign_extend<std::uint32_t, 21>(instr.imm);
        const auto destination = pc() + offset;

        pc() = destination;

        return StepResult::Ok;
    }

    StepResult Emulator::handle_i_instructions(std::uint32_t instruction) {
        constexpr static auto Instructions = jumpTable<2, 7>(
            instr::i::LOAD::Value,        &Emulator::handle_unimplemented,
            instr::i::STORE::Value,       &Emulator::handle_unimplemented,
            instr::i::MADD::Value,        &Emulator::handle_unimplemented,
            instr::i::BRANCH::Value,      &Emulator::handle_unimplemented,
            instr::i::LOAD_FP::Value,     &Emulator::handle_unimplemented,
            instr::i::STORE_FP::Value,    &Emulator::handle_unimplemented,
            instr::i::MSUB::Value,        &Emulator::handle_unimplemented,
            instr::i::JALR::Value,        &Emulator::handle_unimplemented,
            instr::i::NMSUB::Value,       &Emulator::handle_unimplemented,
            instr::i::MISC_MEM::Value,    &Emulator::handle_unimplemented,
            instr::i::AMO::Value,         &Emulator::handle_unimplemented,
            instr::i::NMADD::Value,       &Emulator::handle_unimplemented,
            instr::i::JAL::Value,         &Emulator::handle_jal,
            instr::i::OP_IMM::Value,      &Emulator::handle_unimplemented,
            instr::i::OP::Value,          &Emulator::handle_unimplemented,
            instr::i::OP_FP::Value,       &Emulator::handle_unimplemented,
            instr::i::SYSTEM::Value,      &Emulator::handle_system,
            instr::i::AUIPC::Value,       &Emulator::handle_unimplemented,
            instr::i::LUI::Value,         &Emulator::handle_unimplemented,
            instr::i::OP_IMM_32::Value,   &Emulator::handle_unimplemented,
            instr::i::OP_32::Value,       &Emulator::handle_unimplemented
        );

        return Instructions(this, instruction);
    }

    auto Emulator::loadToRam(std::uint64_t loadAddress, std::span<const std::uint8_t> data) -> void {
        std::memcpy(m_ram.data() + loadAddress, data.data(), data.size());
    }

    StepResult Emulator::step() {
        constexpr static auto Instructions = jumpTable<0, 1>(
            instr::i::Prefix::Value, &Emulator::handle_i_instructions
        );

        const auto instruction = read<std::uint32_t>(pc());
        return Instructions(this, instruction);
    }

}
