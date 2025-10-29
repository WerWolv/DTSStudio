#pragma once

#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include <emu/address_space.hpp>
#include <emu/riscv/register.hpp>
#include <emu/riscv/instructions.hpp>
#include <emu/utils.hpp>

namespace ds::emu::riscv {

    enum class StepResult {
        Ok,
        Unimplemented,
        InvalidInstruction
    };

    class Core {
    public:
        Core() = default;
        Core(std::uint8_t hart, AddressSpace<std::uint32_t> *address_space)
            : m_hart(hart), m_address_space(address_space) { }

        StepResult step();

        template<typename T>
        auto read(std::uint64_t address) -> T {
            if (address % alignof(T) != 0) [[unlikely]] {
                throw std::runtime_error("Invalid read alignment");
            }

            T result;
            m_address_space->read(address, util::to_byte_span(result));
            return result;
        }

        template<typename T>
        auto write(std::uint64_t address, T value) -> void {
            if (address % alignof(T) != 0) [[unlikely]] {
                throw std::runtime_error("Invalid write alignment");
            }

            m_address_space->write(address, util::to_byte_span(value));
        }

        auto reset() -> void {
            m_registers    = {};
            m_programCounter = 0x0000'0000;
            x(0) = m_hart;
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

        constexpr auto csr(std::uint16_t number) -> GeneralPurposeRegister& {
            return m_csrs[number];
        }

        auto pc()   -> auto& { return m_programCounter; }
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

    private:
        StepResult handle_std_instructions(std::uint32_t instruction);

        StepResult handle_unimplemented(std::uint32_t instruction);
        StepResult handle_system(const instr::base::type::I &instruction);
        StepResult handle_jal(const instr::base::type::J &instruction);
        StepResult handle_jalr(const instr::base::type::I &instruction);
        StepResult handle_load(const instr::base::type::I &instruction);
        StepResult handle_store(const instr::base::type::S &instruction);
        StepResult handle_lui(const instr::base::type::U &instruction);
        StepResult handle_auipc(const instr::base::type::U &instruction);
        StepResult handle_op_imm(const instr::base::type::I &instruction);
        StepResult handle_op(const instr::base::type::R &instruction);
        StepResult handle_branch(const instr::base::type::B &instruction);
    private:
        using HandlerFunction = StepResult(Core::*)(std::uint32_t instruction);

        template<typename Instr, auto HandlerFunction>
        struct Entry {
            using Instruction = Instr;
            constexpr static auto Handler = HandlerFunction;
        };

        template<typename Entry>
        StepResult decode_instruction(std::uint32_t instruction) {
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

        ZeroRegister m_zeroRegister;
        std::array<GeneralPurposeRegister, 31> m_registers = {};
        GeneralPurposeRegister m_programCounter = {};

        std::array<GeneralPurposeRegister, 4096> m_csrs;
    };

}