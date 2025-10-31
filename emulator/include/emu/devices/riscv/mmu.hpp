#pragma once

#include <emu/address_space.hpp>
#include <emu/riscv/core.hpp>

namespace ds::emu::dev::riscv {

    using namespace literals;

    template<typename T>
    class MMU : public AddressTranslator<T> {
    public:
        constexpr static auto PageSize = 4_KiB;

        constexpr auto translate(emu::Core &core, T virtual_address) -> std::expected<T, AccessResult> final {
            auto &riscv_core = static_cast<emu::riscv::Core &>(core);

            const auto mode = riscv_core.satp().get_bit(31);

            // If MMU is not enabled, virtual address = physical address
            if (mode == 0)
                return virtual_address;

            const auto root_page_table_page_number = util::extract_bits<0, 21>(
                static_cast<std::uint32_t>(riscv_core.satp())
            );

            const auto vpn0 = util::extract_bits<12, 21>(virtual_address);
            const auto vpn1 = util::extract_bits<22, 31>(virtual_address);

            return get_physical_address(riscv_core, virtual_address, { vpn0, vpn1 }, root_page_table_page_number * PageSize);
        }

        constexpr auto get_physical_address(emu::riscv::Core &core, T virtual_address, std::array<T, 2> virtual_page_numbers, T page_table_address, std::uint8_t level = 1) const -> std::expected<T, AccessResult> {
            const auto index = virtual_page_numbers[level];
            const auto entry_addr = page_table_address + index * sizeof(T);
            const auto &entry = core.read_physical<T>(entry_addr);
            if (!entry.has_value())
                return entry;

            constexpr static auto V = util::bit<0>();
            constexpr static auto R = util::bit<1>();
            constexpr static auto W = util::bit<2>();
            constexpr static auto X = util::bit<3>();

            const auto value = entry.value();

            // V bit is not set, entry is invalid
            if (!(value & (V)))
                return std::unexpected(AccessResult::Unmapped);


            // Only Valid bit is set, entry is a pointer to the next translation layer
            if ((value & (X | W | R | V)) == (V)) {
                const auto physical_page_number = util::extract_bits<10, 31>(value);
                return get_physical_address(core, virtual_address, virtual_page_numbers, physical_page_number * PageSize, 0);
            }

            // Neither readable nor executable, entry is invalid
            if (!(value & (X | R)))
                return std::unexpected(AccessResult::Unmapped);

            // Entry is a leaf entry, construct the final physical address with it
            const auto ppn0 = util::extract_bits<10, 19>(value);
            const auto ppn1 = util::extract_bits<20, 31>(value);
            const auto offset = virtual_address & (PageSize - 1);

            T physical_address;
            if (level == 1) {
                // Super Page
                physical_address = (ppn1 << 22) | (virtual_page_numbers[0] << 12) | offset;
            } else {
                physical_address = (ppn1 << 22) | (ppn0 << 12) | offset;
            }

            return physical_address;
        }

        constexpr auto invalidate() -> void final {

        }
    };

}