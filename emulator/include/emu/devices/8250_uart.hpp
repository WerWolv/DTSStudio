#pragma once

#include <emu/address_space.hpp>

namespace ds::emu::dev {

    class UART8250 : public MemoryMappedPeripheral<std::uint32_t> {
    public:
        explicit UART8250() : MemoryMappedPeripheral(0x100000) { }

        auto read(Offset offset, std::span<std::uint8_t> buffer) -> AccessResult final {
            std::memset(buffer.data(), 0, buffer.size());
            return AccessResult::Ok;
        }

        auto write(Offset offset, std::span<const std::uint8_t> buffer) -> AccessResult final {
            switch (offset) {
                case 1:
                    putchar(buffer[0]);
                    break;
            }
            return AccessResult::Ok;
        }
    };

}