#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <set>

namespace ds::emu {

    enum class AccessResult {
        Ok,
        Unmapped
    };

    class MemoryMappedPeripheral {
    public:
        constexpr explicit MemoryMappedPeripheral(std::size_t size) : m_size(size) {}
        virtual ~MemoryMappedPeripheral() = default;

        virtual auto read(std::uint64_t offset, std::span<std::uint8_t> buffer) -> AccessResult = 0;
        virtual auto write(std::uint64_t offset, std::span<const std::uint8_t> buffer) -> AccessResult = 0;

        [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return m_size; }

    private:
        std::size_t m_size;
    };

    template<typename T>
    class AddressSpace {
    public:
        auto read(std::uint64_t address, std::span<std::uint8_t> buffer) -> AccessResult {
            if (auto entry = get(address); entry != nullptr)
                return entry->peripheral->read(address - entry->base_address, buffer);
            else
                return AccessResult::Unmapped;
        }

        auto write(std::uint64_t address, std::span<const std::uint8_t> buffer) -> AccessResult {
            if (auto entry = get(address); entry != nullptr)
                return entry->peripheral->write(address - entry->base_address, buffer);
            else
                return AccessResult::Unmapped;
        }

        struct Entry {
            T base_address;
            MemoryMappedPeripheral *peripheral;

            auto operator<=>(const Entry &other) const noexcept -> auto {
                return this->base_address <=> other.base_address;
            }
        };

        const Entry* get(std::uint64_t address) {
            auto it = std::find_if(m_peripherals.begin(), m_peripherals.end(),[address](const Entry &peripheral) {
                const T start = peripheral.base_address;
                const T end   = start + peripheral.peripheral->size();

                return address >= start and address < end;
            });

            if (it == m_peripherals.end()) [[unlikely]] {
                return nullptr;
            } else {
                return &*it;
            }
        }

        void map(T base_address, MemoryMappedPeripheral *peripheral) {
            m_peripherals.insert(Entry{ base_address, peripheral });
        }

    private:
        std::set<Entry> m_peripherals;
    };

}