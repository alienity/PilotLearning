#pragma once
#include <type_traits>
#include <windows.h>
#include <strsafe.h>
#include <string>

#include <filesystem>
#include <wil/resource.h>

#include <cmath>
#include <cassert>

#include <mutex>
#include <shared_mutex>

#include "robin_hood.h"

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

// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code?redirectedfrom=MSDN
inline void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD  dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpMsgBuf,
                  0,
                  NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(
        LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
                    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    TEXT("%s failed with error %d: %s"),
                    lpszFunction,
                    dw,
                    lpMsgBuf);
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}