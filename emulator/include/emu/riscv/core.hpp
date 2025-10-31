#pragma once

#include <cstring>
#include <expected>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include <emu/core.hpp>
#include <emu/address_space.hpp>
#include <emu/register.hpp>
#include <emu/riscv/instructions.hpp>
#include <emu/utils.hpp>

namespace ds::emu::riscv {

    enum class StepResult {
        Ok,
        Unimplemented,
        InvalidInstruction,
        InvalidFetch,
        InvalidRead,
        InvalidWrite,
        MisalignedAccess,
        ECallUser,
        ECallSupervisor,
        Stopped
    };

    enum class PrivilegeLevel {
        User,
        Supervisor,
        Hypervisor,
        Machine
    };

    using Register = RegisterBase<std::uint32_t>;

    class Core : public emu::Core {
    public:
        Core() = default;
        Core(std::uint8_t hart, AddressSpace<std::uint32_t> *address_space)
            : m_hart(hart), m_address_space(address_space) {
            reset();
        }

        Core(const Core &other) = delete;
        Core(Core &&other) = default;

        Core &operator=(const Core &other) = delete;
        Core &operator=(Core &&other) = default;

        StepResult step();

        [[nodiscard]] constexpr auto get_privilege_level() const -> PrivilegeLevel {
            return m_privilege_level;
        }

        constexpr void set_privilege_level(PrivilegeLevel privilege_level) {
            m_privilege_level = privilege_level;
        }

        constexpr auto x(std::uint8_t number) -> Register& {
            if (number == 0) {
                return m_zeroRegister;
            } else if (number <= 31) {
                return m_registers[number - 1];
            } else {
                std::unreachable();
            }
        }

        constexpr auto csr(std::uint16_t number) -> Register& {
            return m_csrs[number];
        }

        auto pc()   -> auto& { return m_program_counter; }
        auto zero() -> auto& { return x(0);  }
        auto ra()   -> auto& { return x(1);  }
        auto sp()   -> auto& { return x(2);  }
        auto gp()   -> auto& { return x(3);  }
        auto tp()   -> auto& { return x(4);  }
        auto t0()   -> auto& { return x(5);  }
        auto t1()   -> auto& { return x(6);  }
        auto t2()   -> auto& { return x(7);  }
        auto s0()   -> auto& { return x(8);  }
        auto fp()   -> auto& { return x(8);  }
        auto s1()   -> auto& { return x(9);  }
        auto a0()   -> auto& { return x(10); }
        auto a1()   -> auto& { return x(11); }
        auto a2()   -> auto& { return x(12); }
        auto a3()   -> auto& { return x(13); }
        auto a4()   -> auto& { return x(14); }
        auto a5()   -> auto& { return x(15); }
        auto a6()   -> auto& { return x(16); }
        auto a7()   -> auto& { return x(17); }
        auto s2()   -> auto& { return x(18); }
        auto s3()   -> auto& { return x(19); }
        auto s4()   -> auto& { return x(20); }
        auto s5()   -> auto& { return x(21); }
        auto s6()   -> auto& { return x(22); }
        auto s7()   -> auto& { return x(23); }
        auto s8()   -> auto& { return x(24); }
        auto s9()   -> auto& { return x(25); }
        auto s10()  -> auto& { return x(26); }
        auto s11()  -> auto& { return x(27); }
        auto t3()   -> auto& { return x(28); }
        auto t4()   -> auto& { return x(29); }
        auto t5()   -> auto& { return x(30); }
        auto t6()   -> auto& { return x(31); }

        auto sstatus()      -> auto& { return csr(0x100); }
        auto sie()          -> auto& { return csr(0x104); }
        auto stvec()        -> auto& { return csr(0x105); }
        auto scounteren()   -> auto& { return csr(0x106); }

        auto sscratch()     -> auto& { return csr(0x140); }
        auto sepc()         -> auto& { return csr(0x141); }
        auto scause()       -> auto& { return csr(0x142); }
        auto stval()        -> auto& { return csr(0x143); }
        auto sip()          -> auto& { return csr(0x144); }

        auto satp()         -> auto& { return csr(0x180); }


        auto reset() -> void {
            m_registers    = {};
            m_csrs         = {};
            m_program_counter = 0x0000'0000;
            a0() = m_hart;
        }

        template<typename T>
        auto read(std::uint64_t address) -> std::expected<T, AccessResult> {
            if (address % alignof(T) != 0) [[unlikely]] {
                scause() = 2;
                return std::unexpected(AccessResult::UnalignedAccess);
            }

            T data;
            const auto result = m_address_space->read(*this, address, util::to_byte_span(data));
            if (result != AccessResult::Ok) {
                return std::unexpected(result);
            } else {
                return data;
            }
        }

        template<typename T>
        auto read_physical(std::uint64_t address) -> std::expected<T, AccessResult> {
            if (address % alignof(T) != 0) [[unlikely]] {
                scause() = 2;
                return std::unexpected(AccessResult::UnalignedAccess);
            }

            T data;
            const auto result = m_address_space->read_physical(address, util::to_byte_span(data));
            if (result != AccessResult::Ok) {
                return std::unexpected(result);
            } else {
                return data;
            }
        }

        template<typename T>
        auto write(std::uint64_t address, T value) -> AccessResult {
            if (address % alignof(T) != 0) [[unlikely]] {
                scause() = 2;
                return AccessResult::UnalignedAccess;
            }

            return m_address_space->write(*this, address, util::to_byte_span(value));
        }

        template<typename T>
        auto write_physical(std::uint64_t address, T value) -> AccessResult {
            if (address % alignof(T) != 0) [[unlikely]] {
                scause() = 2;
                return AccessResult::UnalignedAccess;
            }

            return m_address_space->write_physical(address, util::to_byte_span(value));
        }

    private:
        auto handle_std_instructions(std::uint32_t instruction) -> StepResult;

        auto handle_unimplemented(std::uint32_t instruction) -> StepResult;
        auto handle_system(const instr::base::type::I &instruction) -> StepResult;
        auto handle_jal(const instr::base::type::J &instruction) -> StepResult;
        auto handle_jalr(const instr::base::type::I &instruction) -> StepResult;
        auto handle_load(const instr::base::type::I &instruction) -> StepResult;
        auto handle_store(const instr::base::type::S &instruction) -> StepResult;
        auto handle_lui(const instr::base::type::U &instruction) -> StepResult;
        auto handle_auipc(const instr::base::type::U &instruction) -> StepResult;
        auto handle_op_imm(const instr::base::type::I &instruction) -> StepResult;
        auto handle_op(const instr::base::type::R &instruction) -> StepResult;
        auto handle_branch(const instr::base::type::B &instruction) -> StepResult;
        auto handle_misc_mem(const instr::base::type::I &instruction) -> StepResult;
        auto handle_amo(const instr::base::type::R &instruction) -> StepResult;

        auto handle_interrupts() -> void;
        auto trap() -> void;

    private:
        using HandlerFunction = StepResult(Core::*)(std::uint32_t instruction);

        template<typename Instr, auto HandlerFunction>
        struct Entry {
            using Instruction = Instr;
            constexpr static auto Handler = HandlerFunction;
        };

        template<typename Entry>
        auto decode_instruction(std::uint32_t instruction) -> StepResult {
            if constexpr (requires { (this->*Entry::Handler)(typename Entry::Instruction::Type(instruction)); }) {
                return (this->*Entry::Handler)(typename Entry::Instruction::Type(instruction));
            } else {
                return (this->*Entry::Handler)(instruction);
            }
        }

        template<typename First, typename ... Rest>
        constexpr static auto jumpTableImpl(auto &table) -> void {
            table[First::Instruction::Value] = &Core::decode_instruction<First>;
            if constexpr (sizeof...(Rest) > 0) {
                return jumpTableImpl<Rest...>(table);
            }
        }

        template<std::size_t From, std::size_t To, typename ... Entries>
        constexpr static auto jumpTable() -> auto {
            constexpr static auto NumBits = (To - From) + 1;
            std::array<HandlerFunction, 1 << NumBits> table = {};
            for (auto &handler : table) {
                handler = &Core::handle_unimplemented;
            }
            jumpTableImpl<Entries...>(table);

            return [table](Core *emulator, std::uint32_t instruction) {
                const auto index = (instruction & (util::mask<NumBits>() << From)) >> From;
                const HandlerFunction handler = table[index];

                return (emulator->*handler)(instruction);
            };
        }

    private:
        std::uint8_t m_hart = 0;
        AddressSpace<std::uint32_t> *m_address_space = nullptr;

        ZeroRegister<std::uint32_t> m_zeroRegister;
        std::array<GeneralPurposeRegister<std::uint32_t>, 31> m_registers = {};
        GeneralPurposeRegister<std::uint32_t> m_program_counter = {};
        std::uint32_t m_lr_reservation = 0x00;

        std::array<GeneralPurposeRegister<std::uint32_t>, 4096> m_csrs;
        PrivilegeLevel m_privilege_level = PrivilegeLevel::Supervisor;
    };

}