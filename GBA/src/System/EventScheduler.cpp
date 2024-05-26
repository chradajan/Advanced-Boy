#include <System/EventScheduler.hpp>
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

void EventScheduler::Step(uint64_t cycles)
{
    totalCycles_ += cycles;
    Event nextEvent = queue_.front();

    while (totalCycles_ >= nextEvent.cycleToExecute_)
    {
        RegisteredEvent& registrationData = registeredEvents_[nextEvent.eventType_];

        if (registrationData.second)
        {
            std::pop_heap(queue_.begin(), queue_.end(), std::greater<>{});
            queue_.pop_back();
            registrationData.first(totalCycles_ - nextEvent.cycleToExecute_);
            nextEvent = queue_.front();
        }
        else
        {
            break;
        }
    }
}

void EventScheduler::CheckEventQueue()
{
    while (totalCycles_ >= queue_.front().cycleToExecute_)
    {
        Event nextEvent = queue_.front();
        std::pop_heap(queue_.begin(), queue_.end(), std::greater<>{});
        queue_.pop_back();
        RegisteredEvent& registrationData = registeredEvents_[nextEvent.eventType_];
        registrationData.first(totalCycles_ - nextEvent.cycleToExecute_);
    }
}

void EventScheduler::SkipToNextEvent()
{
    totalCycles_ = queue_.front().cycleToExecute_;

    while (totalCycles_ == queue_.front().cycleToExecute_)
    {
        Event nextEvent = queue_.front();
        std::pop_heap(queue_.begin(), queue_.end(), std::greater<>{});
        queue_.pop_back();
        RegisteredEvent& registrationData = registeredEvents_[nextEvent.eventType_];
        registrationData.first(0);
    }
}

void EventScheduler::RegisterEvent(EventType eventType, std::function<void(int)> callback, bool fireMidInstruction)
{
    registeredEvents_.insert({eventType, {callback, fireMidInstruction}});
}

void EventScheduler::ScheduleEvent(EventType eventType, uint64_t cycles)
{
    uint64_t cycleToExecute = cycles + totalCycles_;
    queue_.push_back({eventType, totalCycles_, cycleToExecute});
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
            std::make_heap(queue_.begin(), queue_.end(), std::greater<>{});
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
