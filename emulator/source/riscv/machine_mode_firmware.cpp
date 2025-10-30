#include <cstdio>
#include <tuple>
#include <emu/utils.hpp>
#include <emu/riscv/machine_mode_firmware.hpp>

namespace ds::emu::riscv {

    namespace impl {

        template<auto Function>
        constexpr static auto call_sbi_extension_function(
            std::uint32_t arg0, std::uint32_t arg1, std::uint32_t arg2, std::uint32_t arg3, std::uint32_t arg4, std::uint32_t arg5
        ) -> MachineModeFirmware::SBICallResult {
            using Signature = util::FunctionSignature<decltype(Function)>;

            static_assert(
                std::same_as<typename Signature::ReturnType, MachineModeFirmware::SBICallResult>,
                "SBI Extension function must return a SBICallResult!"
            );

            if constexpr (Signature::ArgumentCount == 0)
                return Function();
            else if constexpr (Signature::ArgumentCount == 1)
                return Function(arg0);
            else if constexpr (Signature::ArgumentCount == 2)
                return Function(arg0, arg1);
            else if constexpr (Signature::ArgumentCount == 3)
                return Function(arg0, arg1, arg2);
            else if constexpr (Signature::ArgumentCount == 4)
                return Function(arg0, arg1, arg2, arg3);
            else if constexpr (Signature::ArgumentCount == 5)
                return Function(arg0, arg1, arg2, arg3, arg4);
            else if constexpr (Signature::ArgumentCount == 6)
                return Function(arg0, arg1, arg2, arg3, arg4, arg5);
            else
                static_assert(false, "SBI Extension Function may not have more than 6 parameters!");

            return { MachineModeFirmware::SBICallErrorCode::NotSupported, 0 };
        }

        template<typename Extension, std::size_t Index = 0>
        constexpr static auto dispatch_call_to_function(
            std::uint32_t function_id,
            std::uint32_t arg0, std::uint32_t arg1, std::uint32_t arg2, std::uint32_t arg3, std::uint32_t arg4, std::uint32_t arg5
        ) -> MachineModeFirmware::SBICallResult {
            using Functions = Extension::Functions;
            if constexpr (Index >= std::tuple_size_v<Functions>) {
                return { MachineModeFirmware::SBICallErrorCode::NotSupported, 0 };
            } else {
                using Function = std::tuple_element_t<Index, Functions>;
                if (Function::ID == function_id) {
                    return call_sbi_extension_function<Function::Func>(
                        arg0, arg1, arg2, arg3, arg4, arg5
                    );
                }

                return dispatch_call_to_function<Extension, Index + 1>(
                    function_id,
                    arg0, arg1, arg2, arg3, arg4, arg5
                );
            }
        }

        template<typename Extensions, std::size_t Index = 0>
        constexpr static auto dispatch_call_to_extension(
            std::uint32_t extension_id, std::uint32_t function_id,
            std::uint32_t arg0, std::uint32_t arg1, std::uint32_t arg2, std::uint32_t arg3, std::uint32_t arg4, std::uint32_t arg5
        ) -> MachineModeFirmware::SBICallResult {
            if constexpr (Index >= std::tuple_size_v<Extensions>) {
                return { MachineModeFirmware::SBICallErrorCode::NotSupported, 0 };
            } else {
                using Extension = std::tuple_element_t<Index, Extensions>;

                if (Extension::ID == extension_id) {
                    return dispatch_call_to_function<Extension>(
                        function_id,
                        arg0, arg1, arg2, arg3, arg4, arg5
                    );
                }

                return dispatch_call_to_extension<Extensions, Index + 1>(
                    extension_id, function_id,
                    arg0, arg1, arg2, arg3, arg4, arg5
                );
            }
        }

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

    }

    struct ExtensionBase;
    struct ExtensionTimer;
    struct ExtensionRst;
    struct ExtensionHsm;
    struct ExtensionIpi;
    struct ExtensionRFence;
    using Extensions = std::tuple<
        ExtensionBase,
        ExtensionTimer,
        ExtensionRst,
        ExtensionHsm,
        ExtensionIpi,
        ExtensionRFence
    >;

    struct ExtensionBase : MachineModeFirmware::Extension<0x0000'0010> {
        static auto get_sbi_spec_version() -> MachineModeFirmware::SBICallResult {
            return { MachineModeFirmware::SBICallErrorCode::Success, (2 << 24) | 0 };
        }

        static auto get_sbi_impl_id() -> MachineModeFirmware::SBICallResult {
            return { MachineModeFirmware::SBICallErrorCode::Success, 0x999 };
        }

        static auto get_sbi_impl_version() -> MachineModeFirmware::SBICallResult {
            return { MachineModeFirmware::SBICallErrorCode::Success, 1 };
        }

        static auto probe_extensions(std::uint32_t extension_id) -> MachineModeFirmware::SBICallResult {
            return {
                MachineModeFirmware::SBICallErrorCode::Success,
                impl::extension_available<Extensions>(extension_id)
            };
        }


        using Functions = std::tuple<
            Function<0, get_sbi_spec_version>,
            Function<1, get_sbi_impl_id>,
            Function<2, get_sbi_impl_version>,
            Function<3, probe_extensions>
        >;
    };

    struct ExtensionTimer : MachineModeFirmware::Extension<"TIME"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionRst : MachineModeFirmware::Extension<"SRST"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionHsm : MachineModeFirmware::Extension<"\x00HSM"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionIpi : MachineModeFirmware::Extension<"\x00sPI"> {
        using Functions = std::tuple<>;
    };

    struct ExtensionRFence : MachineModeFirmware::Extension<"RFNC"> {
        using Functions = std::tuple<>;
    };


    auto MachineModeFirmware::sbi_call(
        std::uint32_t extension_id, std::uint32_t function_id,
        std::uint32_t arg0, std::uint32_t arg1, std::uint32_t arg2, std::uint32_t arg3, std::uint32_t arg4, std::uint32_t arg5
    ) -> SBICallResult {


        const auto result = impl::dispatch_call_to_extension<Extensions>(
            extension_id, function_id,
            arg0, arg1, arg2, arg3, arg4, arg5
        );

        if (result.error == SBICallErrorCode::NotSupported)
            printf("Unimplemented SBI Extension Function Call to [0x%08X](0x%08X)\n", extension_id, function_id);

        return result;
    }

}
