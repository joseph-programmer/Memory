#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "IAllocator.h"
#include <cstring>
#include <algorithm>
#include <atomic>
#include <vector>

namespace Core {

    /**
     * @class StackAllocator
     * @brief A stack-based memory allocator that allocates memory in a LIFO (Last In, First Out) manner.
     *
     * The StackAllocator class allocates memory in a stack-like fashion, where each allocation is
     * pushed onto the stack and deallocations are expected to happen in reverse order.
     */
    class StackAllocator : public IAllocator {

    public:
        /**
         * @brief Constructs a StackAllocator with a specified buffer size.
         * @param bufferSize The size of the buffer to allocate.
         */
        StackAllocator(size_t bufferSize);

        /**
         * @brief Destructor that frees the allocated buffer.
         */
        ~StackAllocator() override;

        /**
         * @brief Allocates memory with the specified size and alignment.
         * @param size The size of the memory to allocate.
         * @param alignment The alignment of the memory to allocate.
         * @return Pointer to the allocated memory, or nullptr if allocation fails.
         */
        void* Allocate(size_t size, size_t alignment) override;

        /**
         * @brief Frees the most recently allocated memory block.
         * @param ptr Pointer to the memory block to free.
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
         * @brief Gets the fragmentation percentage (always returns 0.0 for StackAllocator).
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

        /**
         * @brief Gets the current marker, representing the current offset in the buffer.
         * @return The current marker.
         */
        size_t GetMarker() const;

        /**
         * @brief Frees all allocations back to a specific marker.
         * @param marker The marker to free to.
         */
        void FreeToMarker(size_t marker);

        /**
         * @brief Pushes the current marker onto the marker stack.
         */
        void PushMarker();

        /**
         * @brief Pops the last marker from the marker stack and frees to it.
         */
        void PopMarker();

    private:
        struct AllocationHeader {
            size_t size;       /**< Size of the allocated block. */
            size_t adjustment; /**< Adjustment made to satisfy alignment. */
        };

        uint8_t* m_buffer;               /**< Pointer to the start of the buffer. */
        size_t m_bufferSize;             /**< Total size of the buffer. */
        size_t m_offset;                 /**< Current offset in the buffer. */
        const char* m_name;              /**< Name of the allocator. */
        bool m_threadSafe;               /**< Indicates if the allocator is thread-safe. */
        std::atomic<size_t> m_peakUsage; /**< Peak memory usage tracked by the allocator. */
        std::atomic<size_t> m_allocationCount; /**< Number of allocations made. */
        std::vector<size_t> m_markers;    /**< Stack of markers for managing allocations. */

    };

} // namespace Core

#endif // STACK_ALLOCATOR_H
