#pragma once

#include "runtime/core/log/log_system.h"

#include "runtime/function/global/global_context.h"

#include <chrono>
#include <thread>

#define LOG_HELPER(LOG_LEVEL, ...) \
    MoYu::g_runtime_global_context.m_logger_system->log(LOG_LEVEL, __VA_ARGS__)

#define LOG_TRACE(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Trace, __VA_ARGS__);

#define LOG_DEBUG(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Debug, __VA_ARGS__);

#define LOG_INFO(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Info, __VA_ARGS__);

#define LOG_WARN(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Warn, __VA_ARGS__);

#define LOG_ERROR(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Error, __VA_ARGS__);

#define LOG_FATAL(...) LOG_HELPER(MoYu::LogSystem::LogLevel::Fatal, __VA_ARGS__);

#define MoYuSleep(_ms) std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

#define DEFINE_MOYU_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline constexpr ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline constexpr ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline constexpr ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline constexpr ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline constexpr ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline constexpr ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline constexpr ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}

#ifdef NDEBUG
#define ASSERT(statement, log)
#else
#define ASSERT(statement) assert(statement)
//#define ASSERT(...)
#endif
