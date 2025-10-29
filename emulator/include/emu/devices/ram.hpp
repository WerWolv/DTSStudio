#pragma once

#include <emu/address_space.hpp>

namespace ds::emu::dev {

    class Ram : public MemoryMappedPeripheral {
    public:
        explicit Ram(std::size_t size) : MemoryMappedPeripheral(size) {
            m_data.resize(size);
        }

        auto read(std::uint64_t offset, std::span<std::uint8_t> buffer) -> AccessResult final {
            std::memcpy(buffer.data(), m_data.data() + offset, buffer.size_bytes());
            return AccessResult::Ok;
        }

        auto write(std::uint64_t offset, std::span<const std::uint8_t> buffer) -> AccessResult final {
            std::memcpy(m_data.data() + offset, buffer.data(), buffer.size_bytes());
            return AccessResult::Ok;
        }

    private:
        std::vector<std::uint8_t> m_data;
    };

}