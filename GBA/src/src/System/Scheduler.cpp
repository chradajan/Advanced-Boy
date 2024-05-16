#include <System/Scheduler.hpp>
#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

// Global Scheduler instance
EventScheduler Scheduler;

// Event struct defintions

bool Event::operator>(Event const& rhs) const
{
    if (eventType_ == rhs.eventType_)
    {
        return false;
    }

    if (cycleToExecute_ == rhs.cycleToExecute_)
    {
        return eventType_ > rhs.eventType_;
    }

    return cycleToExecute_ > rhs.cycleToExecute_;
}

// EventScheduler definitions

EventScheduler::EventScheduler()
{
    queue_.reserve(static_cast<size_t>(EventType::COUNT) + 1);
    totalCycles_ = 0;
}

void EventScheduler::Tick(int cycles)
{
    totalCycles_ += static_cast<uint64_t>(cycles);

    while (totalCycles_ >= queue_[0].cycleToExecute_)
    {
        std::pop_heap(queue_.begin(), queue_.end(), std::greater<>{});
        currentEvent_ = std::move(queue_.back());
        queue_.pop_back();
        currentEvent_.Callback(totalCycles_ - currentEvent_.cycleToExecute_);
    }
}

void EventScheduler::SkipToNextEvent()
{
    totalCycles_ = queue_[0].cycleToExecute_;

    while (totalCycles_ == queue_[0].cycleToExecute_)
    {
        std::pop_heap(queue_.begin(), queue_.end(), std::greater<>{});
        currentEvent_ = std::move(queue_.back());
        queue_.pop_back();
        currentEvent_.Callback(0);
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
    queue_.push_back({registeredEvents_[eventType], eventType, totalCycles_, cycleToExecute});
    std::make_heap(queue_.begin(), queue_.end(), std::greater<>{});
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
