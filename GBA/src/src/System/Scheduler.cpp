#include <System/Scheduler.hpp>
#include <cstdint>
#include <format>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>

// Global Scheduler instance
EventScheduler Scheduler;

// Event struct defintions

bool Event::operator<(Event const& rhs) const
{
    if (eventType_ == rhs.eventType_)
    {
        return false;
    }

    if (cycleToExecute_ == rhs.cycleToExecute_)
    {
        return eventType_ < rhs.eventType_;
    }

    return cycleToExecute_ < rhs.cycleToExecute_;
}

// EventScheduler definitions

EventScheduler::EventScheduler()
{
    queue_ = std::set<Event>();
    totalCycles_ = 0;
}

void EventScheduler::Tick(int cycles)
{
    totalCycles_ += static_cast<uint64_t>(cycles);
    auto front = queue_.begin();

    while (totalCycles_ >= (*front).cycleToExecute_)
    {
        front->Callback(totalCycles_ - front->cycleToExecute_);
        queue_.erase(front);
        front = queue_.begin();
    }
}

void EventScheduler::SkipToNextEvent()
{
    auto front = queue_.begin();
    totalCycles_ = front->cycleToExecute_;

    while (totalCycles_ == front->cycleToExecute_)
    {
        front->Callback(0);
        queue_.erase(front);
        front = queue_.begin();
    }
}

void EventScheduler::RegisterEvent(EventType eventType, std::function<void(int)> callback)
{
    if (registeredEvents_.contains(eventType))
    {
        throw std::runtime_error(std::format("Registered duplicate event: {}", static_cast<int>(eventType)));
    }

    registeredEvents_.insert({eventType, callback});
}

void EventScheduler::ScheduleEvent(EventType eventType, int cycles)
{
    uint64_t cycleToExecute = static_cast<uint64_t>(cycles) + totalCycles_;
    queue_.insert({registeredEvents_[eventType], eventType, totalCycles_, cycleToExecute});
}

void EventScheduler::UnscheduleEvent(EventType eventType)
{
    auto it = queue_.begin();

    while (it != queue_.end())
    {
        if (it->eventType_ == eventType)
        {
            queue_.erase(it);
            break;
        }

        ++it;
    }
}

std::optional<int> EventScheduler::ElapsedCycles(EventType eventType) const
{
    for (Event const& event : queue_)
    {
        if (event.eventType_ == eventType)
        {
            return totalCycles_ - event.cycleQueued_;
        }
    }

    return {};
}

std::optional<int> EventScheduler::CyclesRemaining(EventType eventType) const
{
    for (Event const& event : queue_)
    {
        if (event.eventType_ == eventType)
        {
            return event.cycleToExecute_ - totalCycles_;
        }
    }

    return {};
}

std::optional<int> EventScheduler::EventLength(EventType eventType) const
{
    for (Event const& event : queue_)
    {
        if (event.eventType_ == eventType)
        {
            return event.cycleToExecute_ - event.cycleQueued_;
        }
    }

    return {};
}

bool EventScheduler::EventScheduled(EventType eventType) const
{
    for (Event const& event : queue_)
    {
        if (event.eventType_ == eventType)
        {
            return true;
        }
    }

    return false;
}
