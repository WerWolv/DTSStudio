#pragma once

#include <cstdint>
#include <cstring>

namespace ds::emu::riscv {

    class MachineModeFirmware {
    public:
        enum class SBICallErrorCode : std::int32_t {
            Success = 0,
            Failed = -1,
            NotSupported = -2,
            InvalidParam = -3,
            Denied = -4,
            InvalidAddress = -5,
            AlreadyAvailable = -6,
            AlreadyStarted = -7,
            AlreadyStopped = -8,
            NoSharedMemory = -9,
        };

        struct SBICallResult {
            SBICallErrorCode error;
            std::uint32_t return_value;
        };

        template<auto ... Fs>
        struct Functions {

        };

        struct ExtensionId {
            constexpr ExtensionId(std::uint32_t id) : value(id) {}
            constexpr ExtensionId(const char (&id)[5])
                : value((id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3]) { }
            std::uint32_t value;
        };

        template<ExtensionId ExtensionID>
        struct Extension {
            constexpr static auto ID = ExtensionID.value;

            template<std::uint32_t FunctionID, auto F>
            struct Function {
                constexpr static auto ID = FunctionID;
                constexpr static auto Func = F;
            };
        };

        auto sbi_call(
            std::uint32_t extension_id, std::uint32_t function_id,
            std::uint32_t arg0,
            std::uint32_t arg1,
            std::uint32_t arg2,
            std::uint32_t arg3,
            std::uint32_t arg4,
            std::uint32_t arg5
        ) -> SBICallResult;
    };

}
