#pragma once

#include <array>
#include <cstdint>

#include <emu/riscv/core.hpp>
#include <emu/riscv/machine_mode_firmware.hpp>
#include <emu/riscv/machine_mode_firmware_extensions.hpp>

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
            if (m_in_reset) [[unlikely]]
                return StepResult::Stopped;

            auto &core = m_cores[m_current_core];
            const auto curr_pc = core.pc();

            // Step the core one instruction forward
            const auto result = core.step();
            switch (result) {
                case StepResult::Ok:
                    break;
                default:
                    printf("CORE %lu encountered error %d at PC:0x%08X\n", m_current_core, (uint32_t)result, (uint32_t)curr_pc);
            }

            // Handle SBI calls
            if (core.get_privilege_level() == PrivilegeLevel::Machine) {
                const auto [error, return_value] = m_machine_mode_firmware.sbi_call(
                    core,
                    core.a7(), core.a6(),
                    core.a0(), core.a1(), core.a2(), core.a3(), core.a4(), core.a5()
                );

                core.a0() = static_cast<std::uint32_t>(error);
                core.a1() = return_value;

                core.set_privilege_level(PrivilegeLevel::Supervisor);
            }
            m_machine_mode_firmware.update(core);

            // Go to the next core
            m_current_core = (m_current_core + 1) % NumCores;

            return result;
        }

        auto address_space() -> AddressSpace<std::uint32_t>& {
            return m_address_space;
        }

        auto cores() -> std::array<Core, NumCores>& {
            return m_cores;
        }

        auto reset() -> void {
            for (auto &core : m_cores) {
                core.reset();
            }

            m_address_space.reset();
            m_machine_mode_firmware.reset();

            m_in_reset = true;
        }

        auto power_up() -> void {
            reset();
            m_in_reset = false;
        }

    private:
        bool m_in_reset = true;
        m_mode::MachineModeFirmware<m_mode::MachineModeFirmwareExtensions> m_machine_mode_firmware;

        AddressSpace<std::uint32_t> m_address_space;
        std::array<Core, NumCores> m_cores;
        std::size_t m_current_core = 0;
    };

}
