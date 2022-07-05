#pragma once

#include "runtime/core/log/log_system.h"

#include "runtime/function/global/global_context.h"

#include <chrono>
#include <thread>

#define LOG_HELPER(LOG_LEVEL, ...) \
    Pilot::g_runtime_global_context.m_logger_system->log(LOG_LEVEL, __VA_ARGS__)

#define LOG_TRACE(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Trace, __VA_ARGS__);

#define LOG_DEBUG(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Debug, __VA_ARGS__);

#define LOG_INFO(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Info, __VA_ARGS__);

#define LOG_WARN(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Warn, __VA_ARGS__);

#define LOG_ERROR(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Error, __VA_ARGS__);

#define LOG_FATAL(...) LOG_HELPER(Pilot::LogSystem::LogLevel::Fatal, __VA_ARGS__);

#define PolitSleep(_ms) std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

#define PolitNameOf(name) #name

#ifdef NDEBUG
#define ASSERT(statement)
#else
#define ASSERT(statement) assert(statement)
#endif

template<typename T, typename U>
constexpr T AlignUp(T Size, U Alignment)
{
    return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
}

template<typename T, typename U>
constexpr T AlignDown(T Size, U Alignment)
{
    return (T)((size_t)Size & ~((size_t)Alignment - 1));
}

template<typename T>
constexpr T RoundUpAndDivide(T Value, size_t Alignment)
{
    return (T)((Value + Alignment - 1) / Alignment);
}

template<typename T>
constexpr bool IsPowerOfTwo(T Value)
{
    return 0 == (Value & (Value - 1));
}

template<typename T>
constexpr bool IsDivisible(T Value, T Divisor)
{
    return (Value / Divisor) * Divisor == Value;
}