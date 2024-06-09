#pragma once

#include <array>
#include <atomic>
#include <type_traits>

template <typename T, size_t size>
class RingBuffer
{
    static_assert(std::is_trivial<T>::value);
    static_assert(size > 2);

public:
    /// @brief Create a lockless thread-safe ring buffer.
    RingBuffer();

    /// @brief Write data into ring buffer. Should only be called from producer thread.
    /// @param data Pointer to buffer of data to store.
    /// @param cnt Number of items to write into ring buffer.
    /// @return Whether there was adequate space in the buffer to write the requested number of items.
    bool Write(T* data, size_t cnt);

    /// @brief Read data from ring buffer. Should only be called from consumer thread.
    /// @param data Buffer to write data into from ring buffer.
    /// @param cnt Number of items to read from ring buffer.
    /// @return Whether there were enough items in the ring buffer to satisfy the read request.
    bool Read(T* data, size_t cnt);

    /// @brief Get the number of items that can be written into the ring buffer. Should only be called from producer thread.
    /// @return Max number of items that can currently be written to ring buffer.
    size_t GetFree() const;

    /// @brief Get the number of items that are currently in the ring buffer and available to read. Should only be called from
    ///        consumer thread.
    /// @return Max number of items that can currently be read from the ring buffer.
    size_t GetAvailable() const;

private:
    /// @brief Calculate free space to write items to ring buffer.
    /// @param head Current head index.
    /// @param tail Current tail index.
    /// @return Number of free spots in buffer.
    size_t CalculateFree(size_t head, size_t tail) const;

    /// @brief Calculate number of items available to read currently stored in ring buffer.
    /// @param head Current head index.
    /// @param tail Current tail index.
    /// @return Number of available items in buffer.
    size_t CalculateAvailable(size_t head, size_t tail) const;

    std::atomic_size_t head_;
    std::atomic_size_t tail_;
    std::array<T, size> buffer_;
};

#include <Utilities/RingBuffer.tpp>
