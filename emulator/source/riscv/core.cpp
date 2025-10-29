#include <emu/riscv/core.hpp>
#include <emu/riscv/instructions.hpp>

#include <array>
#include <cstdio>
#include <cstring>

namespace ds::emu::riscv {

    StepResult Core::handle_system(const instr::base::type::I &instruction) {
        switch (instruction.funct3) {
            case 0b000: {
                break;
            }
            default: {
                const auto &number = instruction.imm;
                switch (instruction.funct3) {
                    case 0b001: // CSRRW
                        x(instruction.rd) = csr(number);
                        csr(number) = x(instruction.rs1);
                        return StepResult::Ok;
                    case 0b010: // CSRRS
                        x(instruction.rd) = csr(number);
                        csr(number) |= x(instruction.rs1);
                        return StepResult::Ok;
                    case 0b011: // CSRRC
                        x(instruction.rd) = csr(number);
                        csr(number) &= ~x(instruction.rs1);
                        return StepResult::Ok;
                    default:
                        return StepResult::Unimplemented;
                }
            }
        }

        return StepResult::Unimplemented;
    }

    StepResult Core::handle_load(const instr::base::type::I &instruction) {
        const auto offset = instruction.imm;
        const auto address = x(instruction.rs1) + offset;
        const bool sign_extend = (instruction.funct3 >> 2) == 0b1;
        const auto width = 1U << (instruction.funct3 & 0b011);

        std::uint32_t value = 0x00;
        switch (width) {
            case 1:
                value = read<std::uint8_t>(address);
                if (sign_extend)
                    value = util::sign_extend<std::uint32_t, 8>(value);
                break;
            case 2:
                value = read<std::uint16_t>(address);
                if (sign_extend)
                    value = util::sign_extend<std::uint32_t, 16>(value);
                break;
            case 4:
                value = read<std::uint32_t>(address);
                if (sign_extend)
                    return StepResult::InvalidInstruction;
                break;
            default:
                return StepResult::InvalidInstruction;
        }

        x(instruction.rd) = value;
        return StepResult::Ok;
    }

    StepResult Core::handle_store(const instr::base::type::S &instruction) {
        const auto offset = instruction.imm;
        const auto address = x(instruction.rs1) + offset;
        const auto width = 1U << (instruction.funct3 & 0b011);

        switch (width) {
            case 1:
                write<std::uint8_t>(address, x(instruction.rs2));
                break;
            case 2:
                write<std::uint16_t>(address, x(instruction.rs2));
                break;
            case 4:
                write<std::uint32_t>(address, x(instruction.rs2));
                break;
            case 8:
                write<std::uint64_t>(address, x(instruction.rs2));
                break;
            default:
                return StepResult::InvalidInstruction;
        }

        return StepResult::Ok;
    }

    StepResult Core::handle_lui(const instr::base::type::U &instruction) {
        x(instruction.rd) = instruction.imm;
        return StepResult::Ok;
    }

    StepResult Core::handle_auipc(const instr::base::type::U &instruction) {
        x(instruction.rd) = instruction.imm + pc();
        return StepResult::Ok;
    }

    StepResult Core::handle_jal(const instr::base::type::J &instruction) {
        const auto offset = util::sign_extend<std::uint32_t, 21>(instruction.imm);
        const auto destination = pc() + offset;

        pc() = destination - 4;

        return StepResult::Ok;
    }

    StepResult Core::handle_jalr(const instr::base::type::I &instruction) {
        const auto offset = util::sign_extend<std::uint32_t, 12>(instruction.imm);
        const auto destination = (x(instruction.rs1) + offset) & ~0x0000'0001;

        x(instruction.rd) = destination;
        pc() = destination - 4;

        return StepResult::Ok;
    }

    StepResult Core::handle_op_imm(const instr::base::type::I &instruction) {
        const bool alternative = (instruction.imm >> 5) == 0b010'0000;
        const auto shamt = instruction.imm & 0b11111;
        switch (instruction.funct3) {
            case 0b000: { // ADDI
                x(instruction.rd) =
                    x(instruction.rs1) +
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b001: { // SLLI
                x(instruction.rd) =
                    x(instruction.rs1) <<
                    shamt;
                return StepResult::Ok;
            }
            case 0b101: { // SRLI / SRAI
                if (!alternative) {
                    x(instruction.rd) =
                        x(instruction.rs1) >>
                        shamt;
                } else {
                    x(instruction.rd) =
                        static_cast<std::int32_t>(x(instruction.rs1)) >>
                        shamt;
                }
                return StepResult::Ok;
            }
        }
        return StepResult::InvalidInstruction;
    }

    StepResult Core::handle_op(const instr::base::type::R &instruction) {
        const bool alternative = instruction.funct7 == 0b010'0000;
        switch (instruction.funct3) {
            case 0b000: { // ADD / SUB
                if (!alternative) {
                    x(instruction.rd) =
                       x(instruction.rs1) +
                       x(instruction.rs2);
                } else {
                    x(instruction.rd) =
                       x(instruction.rs1) -
                       x(instruction.rs2);
                }
                return StepResult::Ok;
            }
        }

        return StepResult::InvalidInstruction;
    }

    StepResult Core::handle_branch(const instr::base::type::B &instruction) {
        const auto branch_address = pc() + util::sign_extend<std::uint32_t, 13>(instruction.imm) - 4;
        const bool unsigned_compare = (instruction.imm & 0b010) != 0;
        switch (instruction.funct3 & 0b101) {
            case 0b000: // BEQ
                if (x(instruction.rs1) == x(instruction.rs2))
                    pc() = branch_address;
                return StepResult::Ok;
            case 0b001: // BNE
                if (x(instruction.rs1) != x(instruction.rs2))
                    pc() = branch_address;
                return StepResult::Ok;
            case 0b100: // BLT / BLTU
                if (unsigned_compare) {
                    if (x(instruction.rs1) < x(instruction.rs2))
                        pc() = branch_address;
                } else {
                    if (static_cast<std::int32_t>(x(instruction.rs1)) < static_cast<std::int32_t>(x(instruction.rs2)))
                        pc() = branch_address;
                }

                return StepResult::Ok;
            case 0b101: // BGT / BGTU
                if (unsigned_compare) {
                    if (x(instruction.rs1) > x(instruction.rs2))
                        pc() = branch_address;
                } else {
                    if (static_cast<std::int32_t>(x(instruction.rs1)) < static_cast<std::int32_t>(x(instruction.rs2)))
                        pc() = branch_address;
                }

                return StepResult::Ok;
        }
        return StepResult::InvalidInstruction;
    }

    StepResult Core::handle_unimplemented(std::uint32_t instruction) {
        std::ignore = instruction;
        std::printf("Unimplemented instruction 0x%08x at 0x%08X\n", instruction, (uint32_t)pc());
        return StepResult::Unimplemented;
    }

    StepResult Core::handle_std_instructions(std::uint32_t instruction) {
        constexpr static auto Instructions = jumpTable<2, 6,
            Entry<instr::base::LOAD,        &Core::handle_load>,
            Entry<instr::base::STORE,       &Core::handle_store>,
            Entry<instr::base::MADD,        &Core::handle_unimplemented>,
            Entry<instr::base::BRANCH,      &Core::handle_branch>,
            Entry<instr::base::LOAD_FP,     &Core::handle_unimplemented>,
            Entry<instr::base::STORE_FP,    &Core::handle_unimplemented>,
            Entry<instr::base::MSUB,        &Core::handle_unimplemented>,
            Entry<instr::base::JALR,        &Core::handle_jalr>,
            Entry<instr::base::NMSUB,       &Core::handle_unimplemented>,
            Entry<instr::base::MISC_MEM,    &Core::handle_unimplemented>,
            Entry<instr::base::AMO,         &Core::handle_unimplemented>,
            Entry<instr::base::NMADD,       &Core::handle_unimplemented>,
            Entry<instr::base::JAL,         &Core::handle_jal>,
            Entry<instr::base::OP_IMM,      &Core::handle_op_imm>,
            Entry<instr::base::OP,          &Core::handle_op>,
            Entry<instr::base::OP_FP,       &Core::handle_unimplemented>,
            Entry<instr::base::SYSTEM,      &Core::handle_system>,
            Entry<instr::base::AUIPC,       &Core::handle_auipc>,
            Entry<instr::base::LUI,         &Core::handle_lui>,
            Entry<instr::base::OP_IMM_32,   &Core::handle_unimplemented>,
            Entry<instr::base::OP_32,       &Core::handle_unimplemented>
        >();

        const auto result = Instructions(this, instruction);
        pc() += 4;

        return result;
    }

    StepResult Core::step() {
        constexpr static auto Instructions = jumpTable<0, 1,
            Entry<instr::base::Quadrant, &Core::handle_std_instructions>
        >();

        const auto instruction = read<std::uint32_t>(pc());
        return Instructions(this, instruction);
    }

}
