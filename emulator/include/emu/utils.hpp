#pragma once

#include <cstdint>
#include <limits>

namespace ds::emu::util {

    template<std::size_t N>
    struct SizedType;;
    template<> struct SizedType<1> { using Signed = std::int8_t;  using Unsigned = std::uint8_t;  };
    template<> struct SizedType<2> { using Signed = std::int16_t; using Unsigned = std::uint16_t; };
    template<> struct SizedType<4> { using Signed = std::int32_t; using Unsigned = std::uint32_t; };
    template<> struct SizedType<8> { using Signed = std::int64_t; using Unsigned = std::uint64_t; };

    template<std::uint8_t Size, typename T = std::uint32_t>
    constexpr auto mask() -> T {
        return (T(1) << Size) - 1;
    }

    template<std::uint8_t From, std::uint8_t To>
    constexpr auto extract_bits(auto value) -> std::remove_cvref_t<decltype(value)> {
        static_assert(From <= To, "To > From");

        constexpr static auto Mask = mask<(To - From) + 1>() << From;
        return (value & Mask) >> From;
    }

    template<std::integral auto ... Widths>
    constexpr auto combine_bits(std::integral auto ... values) -> auto {
        static_assert(sizeof...(values) == sizeof...(Widths), "Provided value count must match the number of bit widths");

        SizedType<((Widths + ...) + 7) / 8> result = 0;
        (
            (result <<= Widths, result |= (values & mask<Widths>())),
            ...
        );

        return result;
    }

    template<std::unsigned_integral T, std::size_t N>
    constexpr auto sign_extend(T value) -> std::make_signed_t<T> {
        static_assert(N > 0, "Bit width N must be greater than 0");
        static_assert(N <= std::numeric_limits<T>::digits, "Bit width N exceeds type width");

        using SignedT = std::make_signed_t<T>;
        constexpr auto total_bits = std::numeric_limits<T>::digits;

        constexpr T sign_bit = T(1) << (N - 1);

        if (value & sign_bit) {
            constexpr T mask = (~T(0)) << N;
            return static_cast<SignedT>(value | mask);
        } else {
            return static_cast<SignedT>(value);
        }
    }

}