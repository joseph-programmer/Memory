#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#include "IAllocator.h"
#include <cstring>
#include <algorithm>
#include <atomic>

namespace Core {

    /**
     * @class LinearAllocator
     * @brief A linear memory allocator that allocates memory from a pre-allocated buffer.
     *
     * The LinearAllocator class allocates memory in a linear fashion from a pre-allocated buffer.
     * It does not support deallocation or reallocation of individual memory blocks, and it is
     * designed for scenarios where memory is allocated once and freed all at once.
     */
    class LinearAllocator : public IAllocator {

    public:
        /**
         * @brief Constructs a LinearAllocator with a specified buffer size.
         * @param bufferSize The size of the buffer to allocate.
         */
        LinearAllocator(size_t bufferSize);

        /**
         * @brief Destructor that frees the allocated buffer.
         */
        ~LinearAllocator() override;

        /**
         * @brief Allocates memory with the specified size and alignment.
         * @param size The size of the memory to allocate.
         * @param alignment The alignment of the memory to allocate.
         * @return Pointer to the allocated memory, or nullptr if allocation fails.
         */
        void* Allocate(size_t size, size_t alignment) override;

        /**
         * @brief Deallocation is not supported by LinearAllocator.
         * @param ptr Pointer to the memory to free (ignored).
         */
        void Free(void* ptr) override;

        /**
         * @brief Reallocates memory with a new size and alignment.
         * @param ptr Pointer to the previously allocated memory.
         * @param newSize The new size of the memory.
         * @param alignment The alignment of the memory to allocate.
         * @return Pointer to the newly allocated memory, or nullptr if reallocation fails.
         */
        void* Reallocate(void* ptr, size_t newSize, size_t alignment) override;

        /**
         * @brief Gets the size of the allocated memory block.
         * @param ptr Pointer to the memory block.
         * @return Size of the allocated memory block.
         */
        size_t GetAllocationSize(const void* ptr) const override;

        /**
         * @brief Gets the total allocated memory.
         * @return Total allocated memory.
         */
        size_t GetTotalAllocated() const override;

        /**
         * @brief Gets the peak memory usage.
         * @return Peak memory usage.
         */
        size_t GetPeakUsage() const override;

        /**
         * @brief Resets the allocator, allowing memory to be reused.
         */
        void Reset() override;

        /**
         * @brief Checks if the allocator owns the memory block.
         * @param ptr Pointer to the memory block.
         * @return True if the allocator owns the memory block, false otherwise.
         */
        bool Owns(const void* ptr) const override;

        /**
         * @brief Gets the number of allocations made by the allocator.
         * @return Number of allocations.
         */
        size_t GetAllocationCount() const override;

        /**
         * @brief Gets the fragmentation percentage (always returns 0.0 for LinearAllocator).
         * @return Fragmentation percentage.
         */
        float GetFragmentationPercentage() const override;

        /**
         * @brief Sets the name of the allocator.
         * @param name The name of the allocator.
         */
        void SetName(const char* name) override;

        /**
         * @brief Gets the name of the allocator.
         * @return Name of the allocator.
         */
        const char* GetName() const override;

        /**
         * @brief Sets whether the allocator is thread-safe.
         * @param threadSafe True to make the allocator thread-safe, false otherwise.
         */
        void SetThreadSafe(bool threadSafe) override;

        /**
         * @brief Checks if the allocator is thread-safe.
         * @return True if the allocator is thread-safe, false otherwise.
         */
        bool IsThreadSafe() const override;

        /**
         * @brief Validates the internal state of the allocator.
         * @return True if the internal state is valid, false otherwise.
         */
        bool ValidateInternalState() const override;

        /**
         * @brief Gets detailed statistics about the allocator.
         * @return A string containing detailed statistics.
         */
        const char* GetDetailedStats() const override;

    private:
        uint8_t* m_buffer;             /**< Pointer to the start of the buffer. */
        size_t m_bufferSize;           /**< Total size of the buffer. */
        size_t m_offset;               /**< Current offset in the buffer. */
        const char* m_name;            /**< Name of the allocator. */
        bool m_threadSafe;             /**< Indicates if the allocator is thread-safe. */
        std::atomic<size_t> m_peakUsage; /**< Peak memory usage tracked by the allocator. */
        std::atomic<size_t> m_allocationCount; /**< Number of allocations made. */

    };

} // namespace Core

#endif // LINEAR_ALLOCATOR_H
