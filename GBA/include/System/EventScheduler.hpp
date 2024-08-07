#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

constexpr int SCHEDULE_NOW = 0;

/// @brief Types of events that can be scheduled. Order that events are defined affects their priority if events are set to fire at
///        the same time (lower value = higher priority).
enum class EventType
{
    // APU
    Channel1Clock,
    Channel1Envelope,
    Channel1LengthTimer,
    Channel1FrequencySweep,

    Channel2Clock,
    Channel2Envelope,
    Channel2LengthTimer,

    Channel4Clock,
    Channel4Envelope,
    Channel4LengthTimer,

    // Timers
    Timer0Overflow,
    Timer1Overflow,
    Timer2Overflow,
    Timer3Overflow,

    // DMA
    DmaComplete,

    // PPU
    HBlank,
    VBlank,
    VDraw,

    // Audio Sampling
    SampleAPU,

    // Number of unique events that can be scheduled. Do not schedule this, and do not place events below it.
    COUNT
};

/// @brief Data needed to execute a scheduled event.
struct Event
{
    /// @brief What type of event this is.
    EventType eventType_;

    /// @brief Cycle that this event was added to queue.
    uint64_t cycleQueued_;

    /// @brief Cycle that this event should execute its callback.
    uint64_t cycleToExecute_;

    /// @brief Compare priority of events. If A > B, then B should execute first.
    /// @param rhs Event to compare.
    /// @return True if rhs is a lower priority event.
    bool operator>(Event const& rhs) const;
};

/// @brief Manager for scheduling and executing events.
class EventScheduler
{
public:
    /// @brief Initialize EventScheduler.
    EventScheduler();

    void Reset();

    /// @brief Advance the scheduler during a CPU instruction.
    /// @param cycles Number of cycles to advance.
    void Step(uint64_t cycles);

    /// @brief Check for any events to fire after a CPU instruction completes.
    void CheckEventQueue();

    /// @brief If the CPU is stopped due to a halt or DMA transfer, skip straight to next event.
    void SkipToNextEvent();

    /// @brief Register a callback function with a specified event type.
    /// @param eventType Event to register.
    /// @param callback Callback function to use for eventType.
    void RegisterEvent(EventType eventType, std::function<void(int)> callback);

    /// @brief Schedule an event to be executed in some number of cycles from now.
    /// @param event Type of event that should be scheduled.
    /// @param cycles Number of cycles from now that this event should fire.
    void ScheduleEvent(EventType eventType, int cycles);

    /// @brief Set the status of the IRQ line.
    /// @param status True if IRQ should trigger, false otherwise.
    void SetPendingIRQ(bool status) { irqPending_ = status; }

    /// @brief Check the IRQ line.
    /// @return True if IRQ should trigger, false otherwise.
    bool GetPendingIRQ() const { return irqPending_; }

    /// @brief Remove a schedule event from the event queue.
    /// @param eventType Type of event to unschedule.
    void UnscheduleEvent(EventType eventType);

    /// @brief Check how many cycles have elapsed since a particular event was scheduled.
    /// @param eventType Event type to get information for.
    /// @return How many cycles have elapsed since the event was scheduled, if an event of the specified kind is currently queued.
    std::optional<int> ElapsedCycles(EventType eventType) const;

    /// @brief Check how many cycles until a particular event will be executed.
    /// @param eventType Event type to get information for.
    /// @return How many cycles until the event will be executed, if an event of the specified kind is currently queued.
    std::optional<int> CyclesRemaining(EventType eventType) const;

    /// @brief Check how many cycles in the future an event was queued for.
    /// @param eventType Event type to get information for.
    /// @return How many cycles in the future the event was scheduled for, if an event of the specified kind is currently queued.
    std::optional<int> EventLength(EventType eventType) const;

    /// @brief Check if an event type is currently scheduled to be executed.
    /// @param eventType Event type to check.
    /// @return True if specified event is in queue, false otherwise.
    bool EventScheduled(EventType eventType) const;

    /// @brief Return the total number of cycles that have elapsed while the emulator has been running.
    /// @return Total number of cycles.
    uint64_t TotalCycles() const { return totalCycles_; }

private:
    std::vector<Event> queue_;
    std::unordered_map<EventType, std::function<void(int)>> registeredEvents_;
    uint64_t totalCycles_;
    bool irqPending_;
};

extern EventScheduler Scheduler;
