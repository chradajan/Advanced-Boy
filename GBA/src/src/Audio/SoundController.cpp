#include <Audio/SoundController.hpp>
#include <functional>
#include <ARM7TDMI/CpuTypes.hpp>
#include <System/Scheduler.hpp>

namespace Audio
{
SoundController::SoundController()
{
    Scheduler.RegisterEvent(EventType::SampleAPU, std::bind(&CollectSample, this, std::placeholders::_1));
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE);
}

void SoundController::DrainInternalBuffer(float* externalBuffer)
{
    size_t index = 0;

    while (!internalBuffer_.Empty())
    {
        auto [left, right] = internalBuffer_.Pop();
        externalBuffer[index++] = left;
        externalBuffer[index++] = right;
    }
}

void SoundController::CollectSample(int extraCycles)
{
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE - extraCycles);

    if (InternalBufferFull())
    {
        return;
    }

    float leftSample = 0.0;
    float rightSample = 0.0;

    internalBuffer_.Push({leftSample, rightSample});
}
}
