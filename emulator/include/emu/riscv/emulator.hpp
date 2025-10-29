#pragma once

#include <array>
#include <cstdint>

#include "core.hpp"

namespace ds::emu::riscv {

    template<std::size_t NumCores>
    class Emulator {
    public:
        Emulator() {
            for (std::size_t i = 0; i < NumCores; i += 1) {
                m_cores[i] = Core(i, &m_address_space);
            }
        }

        auto step() -> StepResult {
            auto &core = m_cores[m_current_core];
            const auto result = core.step();

            if (result != StepResult::Ok) {
                printf("CORE %lu encountered error %d at PC:0x%08X\n", m_current_core, (uint32_t)result, (uint32_t)core.pc());
            }

            m_current_core = (m_current_core + 1) % NumCores;

            return result;
        }

        auto address_space() -> AddressSpace<std::uint32_t>& {
            return m_address_space;
        }

        auto cores() -> std::array<Core, NumCores> {
            return m_cores;
        }

    private:
        AddressSpace<std::uint32_t> m_address_space;
        std::array<Core, NumCores> m_cores;
        std::size_t m_current_core = 0;
    };

}
