#pragma once
#include "systemtime.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

SystemTime g_SystemTime;

// The amount of time that elapses between ticks of the performance counter
static double sm_CpuTickDelta;

void SystemTime::Initialize()
{
    LARGE_INTEGER frequency;
    ASSERT(TRUE == QueryPerformanceFrequency(&frequency));
    sm_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
}

std::int64_t SystemTime::GetCurrentTick()
{
    LARGE_INTEGER currentTick;
    ASSERT(TRUE == QueryPerformanceCounter(&currentTick));
    return static_cast<int64_t>(currentTick.QuadPart);
}

void SystemTime::BusyLoopSleep(float SleepTime)
{
    int64_t finalTick = (int64_t)((double)SleepTime / sm_CpuTickDelta) + GetCurrentTick();
    while (GetCurrentTick() < finalTick)
        ;
}

double SystemTime::TicksToSeconds(int64_t TickCount)
{
    return TickCount * sm_CpuTickDelta;
}

double SystemTime::TicksToMillisecs(int64_t TickCount)
{
    return TickCount * sm_CpuTickDelta * 1000.0;
}

double SystemTime::TimeBetweenTicks(int64_t tick1, int64_t tick2)
{
    return TicksToSeconds(tick2 - tick1);
}

SystemTime::SystemTime()
{
    SystemTime::Initialize();

    m_StartTick   = SystemTime::GetCurrentTick();
    m_LastFrameTick = m_StartTick;
    
    m_DeltaTicks   = 0;
    m_DeltaMilisecs = 0;
}

double SystemTime::Tick()
{
    int64_t currentTick = SystemTime::GetCurrentTick();
    m_DeltaTicks = currentTick - m_LastFrameTick;
    m_LastFrameTick = currentTick;

    m_DeltaSecs = SystemTime::TicksToSeconds(m_DeltaTicks);
    m_DeltaMilisecs = SystemTime::TicksToMillisecs(m_DeltaTicks);

    return m_DeltaMilisecs;
}

int64_t SystemTime::GetTicks()
{
    return m_LastFrameTick;
}

double SystemTime::GetTimeSecs()
{
    return SystemTime::TicksToSeconds(m_LastFrameTick);
}

double SystemTime::GetTimeMillisecs()
{
    return SystemTime::TicksToMillisecs(m_LastFrameTick);
}

double SystemTime::GetDeltaTimeSecs()
{
    return m_DeltaSecs;
}

double SystemTime::GetDeltaTimeMillisecs()
{
    return m_DeltaMilisecs;
}