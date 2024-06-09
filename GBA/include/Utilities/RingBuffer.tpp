#pragma once

#include <Utilities/RingBuffer.hpp>
#include <array>
#include <atomic>
#include <cstring>

template <typename T, size_t size>
RingBuffer<T, size>::RingBuffer()
{
    head_ = 0;
    tail_ = 0;
}

template <typename T, size_t size>
bool RingBuffer<T, size>::Write(T* data, size_t cnt)
{
    size_t head = head_.load(std::memory_order_relaxed);
    size_t tail = tail_.load(std::memory_order_acquire);

    if (CalculateFree(head, tail) < cnt)
    {
        return false;
    }

    if ((head + cnt) <= size)
    {
        std::memcpy(&buffer_[head], data, cnt * sizeof(T));
        head += cnt;

        if (head == size)
        {
            head = 0;
        }
    }
    else
    {
        size_t firstCopyLen = size - head;
        std::memcpy(&buffer_[head], data, firstCopyLen * sizeof(T));
        size_t remaining = cnt - firstCopyLen;
        std::memcpy(&buffer_[0], &data[firstCopyLen], remaining * sizeof(T));
        head = remaining;
    }

    head_.store(head, std::memory_order_release);
    return true;
}

template <typename T, size_t size>
bool RingBuffer<T, size>::Read(T* data, size_t cnt)
{
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t head = head_.load(std::memory_order_acquire);

    if (CalculateAvailable(head, tail) < cnt)
    {
        return false;
    }

    if ((tail + cnt) <= size)
    {
        std::memcpy(data, &buffer_[tail], cnt * sizeof(T));
        tail += cnt;

        if (tail == size)
        {
            tail = 0;
        }
    }
    else
    {
        size_t firstCopyLen = size - tail;
        std::memcpy(data, &buffer_[tail], firstCopyLen * sizeof(T));
        size_t remaining = cnt - firstCopyLen;
        std::memcpy(&data[firstCopyLen], &buffer_[0], remaining * sizeof(T));
        tail = remaining;
    }

    tail_.store(tail, std::memory_order_release);
    return true;
}

template <typename T, size_t size>
size_t RingBuffer<T, size>::GetFree() const
{
    size_t head = head_.load(std::memory_order_relaxed);
    size_t tail = tail_.load(std::memory_order_acquire);
    return CalculateFree(head, tail);
}

template <typename T, size_t size>
size_t RingBuffer<T, size>::GetAvailable() const
{
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t head = head_.load(std::memory_order_acquire);
    return CalculateAvailable(head, tail);
}

template <typename T, size_t size>
size_t RingBuffer<T, size>::CalculateFree(size_t head, size_t tail) const
{
    if (tail > head)
    {
        return tail - head - 1;
    }
    else
    {
        return size - (head - tail) - 1;
    }
}

template <typename T, size_t size>
size_t RingBuffer<T, size>::CalculateAvailable(size_t head, size_t tail) const
{
    if (head >= tail)
    {
        return head - tail;
    }
    else
    {
        return size - (tail - head);
    }
}
