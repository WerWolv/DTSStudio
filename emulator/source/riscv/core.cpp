#include <emu/riscv/core.hpp>
#include <emu/riscv/instructions.hpp>

#include <cstdio>

namespace ds::emu::riscv {

    auto Core::handle_system(const instr::base::type::I &instruction) -> StepResult {
        switch (instruction.funct3) {
            case 0b000: // PRIV
                if (instruction.imm == 0b000000000000) { // ECALL
                    switch (m_privilege_level) {
                        case PrivilegeLevel::User:
                            scause() = 8;
                            stval() = 0;
                            return StepResult::ECallUser;
                        case PrivilegeLevel::Supervisor:
                            scause() = 9;
                            stval() = 0;
                            return StepResult::ECallSupervisor;
                        default:
                            return StepResult::InvalidInstruction;
                    }
                }
                else if (instruction.imm == 0b000000000001) // EBREAK
                    return StepResult::Break;
                else if (instruction.imm == 0b000100100000) { // SFENCE.VMA
                    m_address_space->invalidate();
                    return StepResult::Ok;
                }
                else
                    return StepResult::InvalidInstruction;
            case 0b001: // CSRRW
                if (instruction.rd != 0) x(instruction.rd) = csr(instruction.imm);
                csr(instruction.imm) = x(instruction.rs1);
                return StepResult::Ok;
            case 0b101: // CSRRWI
                if (instruction.rd != 0) x(instruction.rd) = csr(instruction.imm);
                csr(instruction.imm) = instruction.rs1;
                return StepResult::Ok;
            case 0b010: // CSRRS
                if (instruction.rd  != 0) x(instruction.rd) = csr(instruction.imm);
                if (instruction.rs1 != 0) csr(instruction.imm) |= x(instruction.rs1);
                return StepResult::Ok;
            case 0b110: // CSRRSI
                if (instruction.rd  != 0) x(instruction.rd) = csr(instruction.imm);
                if (instruction.rs1 != 0) csr(instruction.imm) |= instruction.rs1;
                return StepResult::Ok;
            case 0b011: // CSRRC
                if (instruction.rd  != 0) x(instruction.rd) = csr(instruction.imm);
                if (instruction.rs1 != 0) csr(instruction.imm) &= ~x(instruction.rs1);
                return StepResult::Ok;
            case 0b111: // CSRRCI
                if (instruction.rd  != 0) x(instruction.rd) = csr(instruction.imm);
                if (instruction.rs1 != 0) csr(instruction.imm) &= ~instruction.rs1;
                return StepResult::Ok;
            default:
                return StepResult::Unimplemented;
        }

        return StepResult::Unimplemented;
    }

    auto Core::handle_load(const instr::base::type::I &instruction) -> StepResult {
        const auto offset = util::sign_extend<std::uint32_t, 12>(instruction.imm);
        const auto address = x(instruction.rs1) + offset;
        const bool sign_extend = util::extract_bits<2, 2>(instruction.funct3) == 0b0;
        const auto width = 1U << util::extract_bits<0, 1>(instruction.funct3);

        std::expected<std::uint32_t, AccessResult> value;
        switch (width) {
            case 1: // LB
                value = read<std::uint8_t>(address);
                if (!value.has_value()) [[unlikely]]
                    return StepResult::InvalidRead;
                if (sign_extend)
                    value = util::sign_extend<std::uint32_t, 8>(*value);
                break;
            case 2: // LH
                value = read<std::uint16_t>(address);
                if (!value.has_value()) [[unlikely]]
                    return StepResult::InvalidRead;
                if (sign_extend)
                    value = util::sign_extend<std::uint32_t, 16>(*value);
                break;
            case 4: // LW
                value = read<std::uint32_t>(address);
                if (!value.has_value()) [[unlikely]]
                    return StepResult::InvalidRead;
                if (!sign_extend) [[unlikely]]
                    return StepResult::InvalidInstruction;
                break;
            default:
                return StepResult::InvalidInstruction;
        }

        x(instruction.rd) = *value;
        return StepResult::Ok;
    }

    auto Core::handle_store(const instr::base::type::S &instruction) -> StepResult {
        const auto offset = util::sign_extend<std::uint32_t, 12>(instruction.imm);
        const auto address = x(instruction.rs1) + offset;
        const auto width = 1U << util::extract_bits<0, 1>(instruction.funct3);

        switch (width) {
            case 1:
                if (write<std::uint8_t>(address, x(instruction.rs2)) == AccessResult::Unmapped) [[unlikely]]
                    return StepResult::InvalidWrite;
                break;
            case 2:
                if (write<std::uint16_t>(address, x(instruction.rs2)) == AccessResult::Unmapped) [[unlikely]]
                    return StepResult::InvalidWrite;
                break;
            case 4:
                if (write<std::uint32_t>(address, x(instruction.rs2)) == AccessResult::Unmapped) [[unlikely]]
                    return StepResult::InvalidWrite;
                break;
            case 8:
                if (write<std::uint64_t>(address, x(instruction.rs2)) == AccessResult::Unmapped) [[unlikely]]
                    return StepResult::InvalidWrite;
                break;
            default:
                return StepResult::InvalidInstruction;
        }

        return StepResult::Ok;
    }

    auto Core::handle_lui(const instr::base::type::U &instruction) -> StepResult {
        x(instruction.rd) = instruction.imm;
        return StepResult::Ok;
    }

    auto Core::handle_auipc(const instr::base::type::U &instruction) -> StepResult {
        x(instruction.rd) = instruction.imm + pc();
        return StepResult::Ok;
    }

    auto Core::handle_jal(const instr::base::type::J &instruction) -> StepResult {
        const auto offset = util::sign_extend<std::uint32_t, 21>(instruction.imm);
        const auto destination = pc() + offset;

        x(instruction.rd) = pc() + 4;
        pc() = destination - 4;

        return StepResult::Ok;
    }

    auto Core::handle_jalr(const instr::base::type::I &instruction) -> StepResult {
        const auto offset = util::sign_extend<std::uint32_t, 12>(instruction.imm);
        const auto destination = (x(instruction.rs1) + offset) & ~0x0000'0001;

        x(instruction.rd) = pc() + 4;
        pc() = destination - 4;

        return StepResult::Ok;
    }

    auto Core::handle_op_imm(const instr::base::type::I &instruction) -> StepResult {
        const bool alternative = (instruction.imm >> 5) == 0b010'0000;
        const auto shamt = instruction.imm & 0b11111;
        switch (instruction.funct3) {
            case 0b000: { // ADDI
                x(instruction.rd) =
                    x(instruction.rs1) +
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b111: { // ANDI
                x(instruction.rd) =
                    x(instruction.rs1) &
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b110: { // ORI
                x(instruction.rd) =
                    x(instruction.rs1) |
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b100: { // XORI
                x(instruction.rd) =
                    x(instruction.rs1) ^
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b001: { // SLLI
                x(instruction.rd) =
                    x(instruction.rs1) <<
                    shamt;
                return StepResult::Ok;
            }
            case 0b010: { // SLTI
                x(instruction.rd) =
                    static_cast<std::int32_t>(x(instruction.rs1)) <
                    util::sign_extend<std::uint32_t, 12>(instruction.imm);
                return StepResult::Ok;
            }
            case 0b011: { // SLTIU
                x(instruction.rd) =
                    x(instruction.rs1) <
                    instruction.imm;
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

    auto Core::handle_op(const instr::base::type::R &instruction) -> StepResult {
        switch (instruction.funct7) {
            case 0b000'0000: {
                switch (instruction.funct3) {
                    case 0b000: // ADD
                        x(instruction.rd) =
                           x(instruction.rs1) +
                           x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b001: // SLL
                        x(instruction.rd) =
                            x(instruction.rs1) <<
                            x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b101: // SRL
                        x(instruction.rd) =
                            x(instruction.rs1) >>
                            x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b010: // SLT
                        x(instruction.rd) =
                            static_cast<std::int32_t>(x(instruction.rs1)) <
                            static_cast<std::int32_t>(x(instruction.rs2));
                        return StepResult::Ok;
                    case 0b011: // SLTU
                        x(instruction.rd) =
                            x(instruction.rs1) <
                            x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b110: // OR
                        x(instruction.rd) =
                           x(instruction.rs1) |
                           x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b111: // AND
                        x(instruction.rd) =
                           x(instruction.rs1) &
                           x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b100: // XOR
                        x(instruction.rd) =
                           x(instruction.rs1) ^
                           x(instruction.rs2);
                        return StepResult::Ok;
                    default:
                        return StepResult::InvalidInstruction;
                }
            }
            case 0b000'0001: { // MULDIV
                switch (instruction.funct3) {
                    case 0b000: { // MUL
                        const std::int64_t left  = static_cast<std::int32_t>(x(instruction.rs1));
                        const std::int64_t right = static_cast<std::int32_t>(x(instruction.rs2));
                        x(instruction.rd) = static_cast<std::uint64_t>(left * right) & util::mask<32>();
                        return StepResult::Ok;
                    }
                    case 0b001: { // MULH
                        const std::int64_t left  = static_cast<std::int32_t>(x(instruction.rs1));
                        const std::int64_t right = static_cast<std::int32_t>(x(instruction.rs2));
                        x(instruction.rd) = static_cast<std::uint64_t>(left * right) >> 32;
                        return StepResult::Ok;
                    }
                    case 0b010: { // MULHSU
                        const std::int64_t  left  = static_cast<std::int32_t>(x(instruction.rs1));
                        const std::uint64_t right = static_cast<std::uint32_t>(x(instruction.rs2));
                        x(instruction.rd) = static_cast<std::uint64_t>(left * right) >> 32;
                        return StepResult::Ok;
                    }
                    case 0b011: { // MULHU
                        const std::uint64_t left  = static_cast<std::uint32_t>(x(instruction.rs1));
                        const std::uint64_t right = static_cast<std::uint32_t>(x(instruction.rs2));
                        x(instruction.rd) = static_cast<std::uint64_t>(left * right) >> 32;
                        return StepResult::Ok;
                    }
                    case 0b100: { // DIV
                        const std::uint32_t left  = x(instruction.rs1);
                        const std::uint32_t right = x(instruction.rs2);

                        if (right == 0) {
                            x(instruction.rd) = std::numeric_limits<std::uint32_t>::max();
                        } else if (left == 0x8000'0000 and static_cast<std::int32_t>(right) == -1) {
                            x(instruction.rd) = 0x80000000;
                        } else {
                            x(instruction.rd) = static_cast<std::int32_t>(left) / static_cast<std::int32_t>(right);
                        }

                        return StepResult::Ok;
                    }
                    case 0b101: { // DIVU
                        const std::uint32_t left  = x(instruction.rs1);
                        const std::uint32_t right = x(instruction.rs2);

                        if (right == 0) {
                            x(instruction.rd) = std::numeric_limits<std::uint32_t>::max();
                        } else {
                            x(instruction.rd) = left / right;
                        }

                        return StepResult::Ok;
                    }
                    case 0b110: { // REM
                        const std::uint32_t left  = x(instruction.rs1);
                        const std::uint32_t right = x(instruction.rs2);

                        if (right == 0) {
                            x(instruction.rd) = left;
                        } else if (left == 0x8000'0000 and static_cast<std::int32_t>(right) == -1) {
                            x(instruction.rd) = 0;
                        } else {
                            x(instruction.rd) = static_cast<std::int32_t>(left) % static_cast<std::int32_t>(right);
                        }

                        return StepResult::Ok;
                    }
                    case 0b111: { // REMU
                        const std::uint32_t left  = x(instruction.rs1);
                        const std::uint32_t right = x(instruction.rs2);

                        if (right == 0) {
                            x(instruction.rd) = left;
                        } else {
                            x(instruction.rd) = left % right;
                        }

                        return StepResult::Ok;
                    }
                    default:
                        return StepResult::InvalidInstruction;
                }
            }
            case 0b010'0000: {
                switch (instruction.funct3) {
                    case 0b000: // SUB
                        x(instruction.rd) =
                           x(instruction.rs1) -
                           x(instruction.rs2);
                        return StepResult::Ok;
                    case 0b101: // SRA
                        x(instruction.rd) =
                           static_cast<std::int32_t>(x(instruction.rs1)) >>
                           x(instruction.rs2);
                        return StepResult::Ok;
                    default:
                        return StepResult::InvalidInstruction;
                }
            }
            default:
                return StepResult::InvalidInstruction;
        }
    }

    auto Core::handle_branch(const instr::base::type::B &instruction) -> StepResult {
        const auto branch_address = pc() + util::sign_extend<std::uint32_t, 13>(instruction.imm) - 4;
        const bool unsigned_compare = util::extract_bits<1, 1>(instruction.funct3) == 0b1;
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
            case 0b101: // BGE / BGEU
                if (unsigned_compare) {
                    if (x(instruction.rs1) >= x(instruction.rs2))
                        pc() = branch_address;
                } else {
                    if (static_cast<std::int32_t>(x(instruction.rs1)) >= static_cast<std::int32_t>(x(instruction.rs2)))
                        pc() = branch_address;
                }

                return StepResult::Ok;

        }
        return StepResult::InvalidInstruction;
    }

    auto Core::handle_misc_mem(const instr::base::type::I &instruction) -> StepResult {
        switch (instruction.funct3) {
            case 0b000: // FENCE
            case 0b001: // FENCE.I
                // Nothing to do here
                return StepResult::Ok;
        }
        return StepResult::InvalidInstruction;
    }

    auto Core::handle_amo(const instr::base::type::R &instruction) -> StepResult {
        switch (instruction.funct3) {
            case 0b010: { // RV32A
                const auto rl    = util::extract_bits<0, 0>(instruction.funct7);
                const auto aq    = util::extract_bits<1, 1>(instruction.funct7);
                const auto funct5 = util::extract_bits<2, 5>(instruction.funct7);

                std::ignore = rl;
                std::ignore = aq;

                const std::uint32_t address = x(instruction.rs1);
                const std::uint32_t value   = x(instruction.rs2);
                switch (funct5) {
                    case 0b00010: { // LR.W
                        if (address % 4 != 0)
                            return StepResult::MisalignedAccess;

                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        const auto physical_address = m_address_space->translate_address(*this, address);
                        if (!physical_address.has_value())
                            return StepResult::InvalidRead;

                        this->m_lr_reservation = *physical_address | 0b1;
                        x(instruction.rd) = *result;

                        return StepResult::Ok;
                    }
                    case 0b00011: { // SC.W
                        if (address % 4 != 0)
                            return StepResult::MisalignedAccess;

                        x(instruction.rd) = 1;

                        const auto physical_address = m_address_space->translate_address(*this, address);
                        if (!physical_address.has_value())
                            return StepResult::InvalidRead;

                        if (m_lr_reservation != (*physical_address | 0b1))
                            return StepResult::Ok;

                        const auto result = write<std::uint32_t>(address, value);
                        if (result != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        // TODO: Needs to be cleared on all harts that match the address
                        if ((m_lr_reservation & 0b1) and (m_lr_reservation & ~0b11) == (*physical_address & ~0b11))
                            this->m_lr_reservation = 0;
                        x(instruction.rd) = 0;

                        return StepResult::Ok;
                    }
                    case 0b00001: { // AMOSWAP.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        if (write<std::uint32_t>(address, value) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        x(instruction.rd) = *result;
                        return StepResult::Ok;
                    }
                    case 0b00000: { // AMOADD.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, *result +value) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b00100: { // AMOXOR.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, *result ^ value) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b01100: { // AMOAND.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, *result & value) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b01000: { // AMOOR.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, *result | value) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b10000: { // AMOMIN.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, std::min<std::int32_t>(*result, value)) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b10100: { // AMOMAX.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, std::max<std::int32_t>(*result, value)) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b11000: { // AMOMINU.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, std::min<std::uint32_t>(*result, value)) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    case 0b11100: { // AMOMAXU.W
                        const auto result = read<std::uint32_t>(address);
                        if (!result.has_value())
                            return StepResult::InvalidRead;

                        x(instruction.rd) = *result;
                        if (write<std::uint32_t>(address, std::max<std::uint32_t>(*result, value)) != AccessResult::Ok)
                            return StepResult::InvalidWrite;

                        return StepResult::Ok;
                    }
                    default:
                        return StepResult::Unimplemented;
                }
            }
            default:
                return StepResult::InvalidInstruction;
        }
    }

    auto Core::handle_unimplemented(std::uint32_t instruction) -> StepResult {
        std::ignore = instruction;
        std::printf("Unimplemented instruction 0x%08x at 0x%08X\n", instruction, (uint32_t)pc());
        return StepResult::Unimplemented;
    }

    auto Core::handle_std_instructions(std::uint32_t instruction) -> StepResult {
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
            Entry<instr::base::MISC_MEM,    &Core::handle_misc_mem>,
            Entry<instr::base::AMO,         &Core::handle_amo>,
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

    auto Core::handle_interrupts() -> void {
        // If the SSTATUS.SIE flag is not set, interrupts are disabled in the Supervisor
        // Interrupts will stil trigger when in Userspace
        if (m_privilege_level == PrivilegeLevel::Supervisor and sstatus().get_bit(1)) {
            trap();
            return;
        }

        // Check if any of the enabled interrupts are currently pending
        const auto pending_interrupt = sie() & sip();
        if (m_privilege_level == PrivilegeLevel::User and pending_interrupt != 0x00) {
            trap();
            return;
        }
    }

    auto Core::trap() -> void {
        // Set SSTATUS.SPIE to SSTATUS.SIE
        sstatus().set_bit(5, sstatus().get_bit(1));

        // Set SSTATUS.SPP to the current privilege level
        sstatus().set_bit(8, m_privilege_level == PrivilegeLevel::Supervisor);

        // Set SEPC to the value of PC where the exception happened
        sepc() = pc() - 4;

        // Disable interrupts
        sstatus().set_bit(1, false);

        // Invalidate MMU
        m_address_space->invalidate();

        // Enter supervisor mode
        m_privilege_level = PrivilegeLevel::Supervisor;

        // Jump to the supervisor interrupt vector address
        {
            const auto mode = stvec() & 0b11;
            const auto base = stvec() & ~0b11;

            pc() = base;
            if (mode == 0b01) {
                pc() += (scause() & util::mask<31>()) * 4;
            }
        }
    }

    auto Core::step() -> StepResult {
        constexpr static auto Instructions = jumpTable<0, 1,
            Entry<instr::base::Quadrant, &Core::handle_std_instructions>
        >();

        handle_interrupts();

        StepResult result;
        const auto instruction = read<std::uint32_t>(pc());
        if (instruction.has_value()) [[likely]] {
            result = Instructions(this, *instruction);
            if (result == StepResult::InvalidInstruction)
                scause() = 2;
        } else {
            result = StepResult::InvalidFetch;
        }

        switch (result) {
            case StepResult::Ok:
                break;
            case StepResult::ECallSupervisor: // ECALL from Supervisor mode, delegate it to machine mode
                set_privilege_level(PrivilegeLevel::Machine);
                return StepResult::Ok;
            case StepResult::ECallUser: // ECALL from User mode, jump to supervisor
                set_privilege_level(PrivilegeLevel::Supervisor);
                // TODO: trap
                return StepResult::Ok;
            default:
                this->trap();
                break;
        }


        return result;
    }

}
