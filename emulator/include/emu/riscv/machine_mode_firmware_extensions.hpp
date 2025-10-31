#pragma once

#include <emu/riscv/machine_mode_firmware.hpp>

namespace ds::emu::riscv::m_mode {

    struct ExtensionBase;
    struct ExtensionTimer;
    struct ExtensionRst;
    struct ExtensionHsm;
    struct ExtensionIpi;
    struct ExtensionRFence;

    using MachineModeFirmwareExtensions = std::tuple<
        ExtensionBase,
        ExtensionTimer,
        ExtensionRst,
        ExtensionHsm,
        ExtensionIpi,
        ExtensionRFence
    >;

    struct ExtensionBase : Extension<0x0000'0010> {
        constexpr static auto get_sbi_spec_version() -> SBICallResult {
            constexpr static auto SbiSpecVersion = (2 << 24) | 0;
            return { SBICallErrorCode::Success, SbiSpecVersion };
        }

        constexpr static auto get_sbi_impl_id() -> SBICallResult {
            constexpr static auto SbiImplId = 0x999;
            return { SBICallErrorCode::Success, SbiImplId };
        }

        constexpr static auto get_sbi_impl_version() -> SBICallResult {
            constexpr static auto SbiImplVersion = 1;
            return { SBICallErrorCode::Success, SbiImplVersion };
        }

        constexpr static auto probe_extensions(std::uint32_t extension_id) -> SBICallResult {
            return {
                SBICallErrorCode::Success,
                extension_available<MachineModeFirmwareExtensions>(extension_id)
            };
        }

        constexpr static auto get_mvendorid() -> SBICallResult {
            constexpr static auto MVendorId = 0x12345678;
            return { SBICallErrorCode::Success, MVendorId };
        }

        constexpr static auto get_marchid() -> SBICallResult {
            constexpr static auto MArchId = (1ULL << 31) | 1;
            return { SBICallErrorCode::Success, MArchId };
        }

        constexpr static auto get_mimpid() -> SBICallResult {
            constexpr static auto MImpid = 1;
            return { SBICallErrorCode::Success, MImpid };
        }


        using Functions = std::tuple<
            Function<0, &ExtensionBase::get_sbi_spec_version>,
            Function<1, &ExtensionBase::get_sbi_impl_id>,
            Function<2, &ExtensionBase::get_sbi_impl_version>,
            Function<3, &ExtensionBase::probe_extensions>,
            Function<4, &ExtensionBase::get_mvendorid>,
            Function<5, &ExtensionBase::get_marchid>,
            Function<6, &ExtensionBase::get_mimpid>
        >;

    private:
        template<typename Extensions, std::size_t Index = 0>
        constexpr static auto extension_available(std::uint32_t extension_id) -> bool {
            if constexpr (Index >= std::tuple_size_v<Extensions>) {
                return false;
            } else {
                using Extension = std::tuple_element_t<Index, Extensions>;
                if (Extension::ID == extension_id)
                    return true;

                return extension_available<Extensions, Index + 1>(extension_id);
            }
        }
    };

    struct ExtensionTimer : Extension<"TIME"> {
        auto set_timer(Core &core, std::uint32_t low, std::uint32_t high) -> SBICallResult {
            const auto value = (static_cast<std::uint64_t>(high) << 32) | low;
            std::ignore = core;
            std::ignore = value;

            return { SBICallErrorCode::NotSupported, 0 };
        }

        auto update(Core &core) -> void {

        }

        using Functions = std::tuple<
            Function<0, &ExtensionTimer::set_timer>
        >;
    };

    struct ExtensionRst : Extension<"SRST"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionHsm : Extension<"\x00HSM"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionIpi : Extension<"\x00sPI"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionRFence : Extension<"RFNC"> {
        using Functions = std::tuple<>;
    };

}