#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <unordered_map>

constexpr int SCHEDULE_NOW = 0;

/// @brief Types of events that can be scheduled. Order that events are defined affects their priority if events are set to fire at
///        the same time (lower value = higher priority).
enum class EventType
{
    // External
    RefreshScreen,

    // Low power mode
    Halt,

    // Interrupts
    IRQ,

    // PPU
    HBlank,
    VBlank,
    VDraw,

    // Timers
    Timer0Overflow,
    Timer1Overflow,
    Timer2Overflow,
    Timer3Overflow,
};

/// @brief Data needed to execute a scheduled event.
struct Event
{
    /// @brief Callback function for when event fires.
    std::function<void(int)> Callback;

    /// @brief What type of event this is.
    EventType eventType_;

    /// @brief Cycle that this event was added to queue.
    uint64_t cycleQueued_;

    /// @brief Cycle that this event should execute its callback.
    uint64_t cycleToExecute_;

    /// @brief Compare priority of events.
    /// @param rhs Event to compare.
    /// @return True if rhs is a lower priority event.
    bool operator<(Event const& rhs) const;
};

/// @brief Manager for scheduling and executing events.
class EventScheduler
{
public:
    /// @brief Initialize EventScheduler.
    EventScheduler();

    /// @brief Advance the Scheduler by some number of cycles.
    /// @param cycles Number of cycles to advance the Scheduler by.
    void Tick(int cycles);

    /// @brief Register a callback function with a specified event type.
    /// @param eventType Event to register.
    /// @param callback Callback function to use for eventType.
    void RegisterEvent(EventType eventType, std::function<void(int)> callback);

    /// @brief Schedule an event to be executed in some number of cycles from now.
    /// @param event Type of event that should be scheduled.
    /// @param cycles Number of cycles from now that this event should fire.
    void ScheduleEvent(EventType eventType, int cycles);

    /// @brief Remove a schedule event from the event queue.
    /// @param eventType Type of event to unschedule.
    void UnscheduleEvent(EventType eventType);

    /// @brief Check how many cycles have elapsed since a particular event was scheduled.
    /// @param eventType Event type to get information for.
    /// @return How many cycles have elapsed since the event was scheduled, if an event of the specified kind is currently queued.
    std::optional<uint64_t> ElapsedCycles(EventType eventType);

private:
    std::set<Event> queue_;
    std::unordered_map<EventType, std::function<void(int)>> registeredEvents_;
    uint64_t totalCycles_;
};

extern EventScheduler Scheduler;
