#pragma once

#include <cstring>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include <emu/riscv/register.hpp>
#include <emu/riscv/instructions.hpp>
#include <emu/utils.hpp>

namespace ds::emu::riscv {

    enum class StepResult {
        Ok,
        Unimplemented
    };

    class Emulator {
    public:
        Emulator(std::size_t ramSize) {
            m_ram.resize(ramSize, 0x00);
        }

        StepResult step();

        auto loadToRam(std::uint64_t loadAddress, std::span<const std::uint8_t> data) -> void;

        constexpr auto x(std::uint8_t number) -> Register& {
            if (number == 0) {
                return m_zeroRegister;
            } else if (number <= 31) {
                return m_registers[number - 1];
            } else {
                std::unreachable();
            }
        }

        template<typename T>
        auto read(std::uint64_t address) -> T {
            if (address % alignof(T) != 0) [[unlikely]] {
                throw std::runtime_error("Invalid read alignment");
            }

            T result;
            std::memcpy(&result, m_ram.data() + address, sizeof(T));
            return result;
        }

        template<typename T>
        auto write(std::uint64_t address, T value) -> void {
            if (address % alignof(T) != 0) [[unlikely]] {
                throw std::runtime_error("Invalid write alignment");
            }

            std::memcpy(m_ram.data() + address, &value, sizeof(T));
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
        StepResult handle_i_instructions(std::uint32_t instruction);

        StepResult handle_unimplemented(std::uint32_t instruction);
        StepResult handle_system(std::uint32_t instruction);
        StepResult handle_jal(std::uint32_t instruction);

    private:
        using HandlerFunction = StepResult(Emulator::*)(std::uint32_t instruction);
        constexpr static auto jumpTableImpl(auto &table, std::uint32_t opcode, HandlerFunction handler, auto ... args) -> void {
            table[opcode] = handler;
            if constexpr (sizeof...(args) > 0) {
                return jumpTableImpl(table, args...);
            }
        }

        template<std::size_t From, std::size_t To>
        constexpr static auto jumpTable(auto ... args) -> auto {
            constexpr static auto NumBits = (To - From) + 1;
            std::array<HandlerFunction, 1 << NumBits> table = {};
            for (auto &handler : table) {
                handler = &Emulator::handle_unimplemented;
            }
            jumpTableImpl(table, args...);

            return [table](Emulator *emulator, std::uint32_t instruction) {
                const HandlerFunction handler = table[(instruction & (util::mask<NumBits>() << From)) >> From];

                return (emulator->*handler)(instruction);
            };
        }

    private:
        ZeroRegister m_zeroRegister;
        std::array<GeneralPurposeRegister, 31> m_registers = {};
        GeneralPurposeRegister m_programCounter = {};
        std::vector<std::uint8_t> m_ram;
    };

}