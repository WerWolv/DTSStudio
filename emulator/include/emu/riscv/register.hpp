#pragma once

#include <cstdint>
#include <concepts>
#include <tuple>
#include <array>
#include <utility>

namespace ds::emu::riscv {

    template<std::integral T>
    class RegisterBase {
    public:
        using Type = T;

        constexpr virtual ~RegisterBase() = default;

        constexpr auto operator+=(Type type) -> RegisterBase& {
            return this->operator=(*this + type);
        }
        constexpr auto operator-=(Type type) -> RegisterBase& {
            return this->operator=(*this - type);
        }
        constexpr auto operator|=(Type type) -> RegisterBase& {
            return this->operator=(*this | type);
        }
        constexpr auto operator&=(Type type) -> RegisterBase& {
            return this->operator=(*this & type);
        }

        constexpr auto get() const -> Type { return operator Type(); }

        constexpr auto operator=(std::derived_from<RegisterBase> auto &other) -> RegisterBase& {
            return operator=(static_cast<T>(other));
        }

        constexpr auto set_bit(std::uint8_t index, bool value) -> void {
            const auto bit = 1U << index;

            if (value)
                (*this) |= bit;
            else
                (*this) &= ~bit;
        }

        constexpr auto get_bit(std::uint8_t index) -> bool {
            const auto bit = 1ULL << index;

            return (*this) & bit;
        }

        constexpr virtual auto operator=(Type type) -> RegisterBase& = 0;
        constexpr virtual operator Type() const = 0;
    };
    using Register = RegisterBase<uint32_t>;

    class GeneralPurposeRegister : public Register {
    public:
        constexpr auto operator=(Type type) -> GeneralPurposeRegister& final {
            m_value = type;
            return *this;
        }

        constexpr operator Type() const final {
            return m_value;
        }

    private:
        Type m_value = 0x00;
    };

    class ReadOnlyRegister : public Register {
    public:
        explicit ReadOnlyRegister(Type value) : m_value(value) {}
        constexpr auto operator=(Type type) -> ReadOnlyRegister& final {
            std::ignore = type;
            return *this;
        }

        constexpr operator Type() const final {
            return m_value;
        }

    private:
        Type m_value = 0x00;
    };

    class ZeroRegister : public Register {
    public:
        constexpr auto operator=(Type type) -> ZeroRegister& final {
            std::ignore = type;
            return *this;
        }

        constexpr operator Type() const final {
            return 0x00;
        }
    };

}
