#pragma once
#include "system_core.h"

class SystemTime
{
public:
    // Query the performance counter frequency
    static void Initialize();
    // Query the current value of the performance counter
    static std::int64_t GetCurrentTick();

    static void BusyLoopSleep(float SleepTime);

    static double TicksToSeconds(int64_t TickCount);
    static double TicksToMillisecs(int64_t TickCount);
    static double TimeBetweenTicks(int64_t tick1, int64_t tick2);

    SystemTime();

    double Tick();
    
    int64_t GetTicks();
    double  GetTimeSecs();
    double  GetTimeMillisecs();
    double  GetDeltaTimeSecs();
    double  GetDeltaTimeMillisecs();

private:
    int64_t m_StartTick;
    int64_t m_LastFrameTick;
    int64_t m_DeltaTicks;

    double m_DeltaSecs;
    double m_DeltaMilisecs;
};

extern SystemTime g_SystemTime;