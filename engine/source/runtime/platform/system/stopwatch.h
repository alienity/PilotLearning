#pragma once
#include "system_core.h"

class Stopwatch
{
public:
    static std::int64_t GetTimestamp();

    Stopwatch() noexcept;

    [[nodiscard]] double GetDeltaTime() const noexcept;
    [[nodiscard]] double GetTotalTime() const noexcept;
    [[nodiscard]] double GetTotalTimePrecise() const noexcept;
    [[nodiscard]] bool   IsPaused() const noexcept;

    void Resume() noexcept;
    void Pause() noexcept;
    void Signal() noexcept;
    void Restart() noexcept;

    static std::int64_t Frequency; // Ticks per second

private:
    std::int64_t StartTime    = {};
    std::int64_t TotalTime    = {};
    std::int64_t PausedTime   = {};
    std::int64_t StopTime     = {};
    std::int64_t PreviousTime = {};
    std::int64_t CurrentTime  = {};
    double       Period       = 0.0;
    double       DeltaTime    = 0.0;

    bool Paused = false;
};

template<typename TCallback>
class ScopedTimer
{
public:
    ScopedTimer(TCallback&& Callback) : Callback(std::move(Callback)), Start(Stopwatch::GetTimestamp()) {}
    ~ScopedTimer()
    {
        std::int64_t End                 = Stopwatch::GetTimestamp();
        std::int64_t Elapsed             = End - Start;
        std::int64_t ElapsedMilliseconds = Elapsed * 1000 / Stopwatch::Frequency;
        Callback(ElapsedMilliseconds);
    }

private:
    TCallback    Callback;
    std::int64_t Start;
};
