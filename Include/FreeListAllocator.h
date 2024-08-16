#ifndef FREELISTALLOCATOR_H
#define FREELISTALLOCATOR_H

#include "IAllocator.h"
#include <cstddef>
#include <cstdint>
#include <atomic>

namespace Core {

    /**
     * @brief A memory allocator using a free list to manage allocations and deallocations.
     */
    class FreeListAllocator : public IAllocator {

    public:
        /**
         * @brief Constructs a FreeListAllocator with a specified buffer size.
         * @param bufferSize Size of the buffer.
         */
        FreeListAllocator(size_t bufferSize);

        /**
         * @brief Destructor for the FreeListAllocator.
         */
        ~FreeListAllocator() override;

        void* Allocate(size_t size, size_t alignment) override;
        void Free(void* ptr) override;
        void* Reallocate(void* ptr, size_t newSize, size_t alignment) override;
        size_t GetAllocationSize(const void* ptr) const override;
        size_t GetTotalAllocated() const override;
        size_t GetPeakUsage() const override;
        void Reset() override;
        bool Owns(const void* ptr) const override;
        size_t GetAllocationCount() const override;
        float GetFragmentationPercentage() const override;
        void SetName(const char* name) override;
        const char* GetName() const override;
        void SetThreadSafe(bool threadSafe) override;
        bool IsThreadSafe() const override;
        bool ValidateInternalState() const override;
        const char* GetDetailedStats() const override;

    private:
        struct AllocationHeader {
            size_t size;       ///< The size of the allocated block.
            uint8_t padding;   ///< Padding added to satisfy alignment requirements.
        };

        struct FreeBlock {
            size_t size;       ///< The size of the free block.
            FreeBlock* next;   ///< Pointer to the next free block.
        };

        uint8_t* m_buffer;                     ///< The buffer used for memory allocation.
        size_t m_bufferSize;                   ///< The total size of the buffer.
        FreeBlock* m_freeList;                 ///< The free list of memory blocks.
        const char* m_name;                    ///< Name of the allocator.
        bool m_threadSafe;                     ///< Flag indicating if the allocator is thread-safe.
        std::atomic<size_t> m_allocatedSize;   ///< Total allocated size.
        std::atomic<size_t> m_peakUsage;       ///< Peak memory usage.
        std::atomic<size_t> m_allocationCount; ///< Number of allocations.

        static const size_t MIN_BLOCK_SIZE;    ///< Minimum block size.

    };

} // namespace Core

#endif // FREELISTALLOCATOR_H
