#pragma once

#include <Utilities/CircularBuffer.hpp>
#include <array>
#include <cstdint>
#include <stdexcept>

template <typename T, size_t len>
CircularBuffer<T, len>::CircularBuffer() noexcept
{
    static_assert(len != 0);
    Clear();
}

template <typename T, size_t len>
bool CircularBuffer<T, len>::Full() const noexcept
{
    return count_ == buffer_.size();
}

template <typename T, size_t len>
size_t CircularBuffer<T, len>::Size() const noexcept
{
    return count_;
}

template <typename T, size_t len>
bool CircularBuffer<T, len>::Empty() const noexcept
{
    return count_ == 0;
}

template <typename T, size_t len>
void CircularBuffer<T, len>::Push(T val)
{
    if (Full())
    {
        throw std::out_of_range("CircularBuffer Illegal Push");
    }

    buffer_[head_++] = val;
    head_ %= buffer_.size();
    ++count_;
}

template <typename T, size_t len>
T CircularBuffer<T, len>::Pop()
{
    if (Empty())
    {
        throw std::out_of_range("CircularBuffer Illegal Pop");
    }

    size_t returnIndex = tail_++;
    tail_ %= buffer_.size();
    --count_;
    return buffer_[returnIndex];
}

template <typename T, size_t len>
T const& CircularBuffer<T, len>::Peak() const
{
    if (Empty())
    {
        throw std::out_of_range("CircularBuffer Illegal Peak");
    }

    return buffer_[tail_];
}

template <typename T, size_t len>
void CircularBuffer<T, len>::Clear() noexcept
{
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}
