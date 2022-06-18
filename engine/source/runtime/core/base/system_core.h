#pragma once
#include <type_traits>
#include <cmath>

// clang-format off
// Decimal SI units
constexpr size_t operator""_KB(size_t x) { return x * 1000; }
constexpr size_t operator""_MB(size_t x) { return x * 1000 * 1000; }
constexpr size_t operator""_GB(size_t x) { return x * 1000 * 1000 * 1000; }

// Binary SI units
constexpr size_t operator""_KiB(size_t x) { return x * 1024; }
constexpr size_t operator""_MiB(size_t x) { return x * 1024 * 1024; }
constexpr size_t operator""_GiB(size_t x) { return x * 1024 * 1024 * 1024; }
// clang-format on

// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

template<typename Enum>
struct EnableBitMaskOperators
{
    static constexpr bool Enable = false;
};

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator|(Enum Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) | static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator&(Enum Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) & static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator^(Enum Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) ^ static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator~(Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    return static_cast<Enum>(~static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator|=(Enum& Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    Lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) | static_cast<UnderlyingType>(Rhs));
    return Lhs;
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator&=(Enum& Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    Lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) & static_cast<UnderlyingType>(Rhs));
    return Lhs;
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator^=(Enum& Lhs, Enum Rhs)
{
    using UnderlyingType = std::underlying_type_t<Enum>;
    Lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) ^ static_cast<UnderlyingType>(Rhs));
    return Lhs;
}

#define ENABLE_BITMASK_OPERATORS(Enum) \
    template<> \
    struct EnableBitMaskOperators<Enum> \
    { \
        static constexpr bool Enable = true; \
    }; \
\
    constexpr bool EnumMaskBitSet(Enum Mask, Enum Component) { return (Mask & Component) == Component; }

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = std::float_t;
using f64 = std::double_t;
