#include "FreeListAllocator.h"
#include <algorithm>
#include <cassert>
#include <cstring>

namespace Core {

    const size_t FreeListAllocator::MIN_BLOCK_SIZE = sizeof(FreeBlock);

    FreeListAllocator::FreeListAllocator(size_t bufferSize)
        : m_buffer(new uint8_t[bufferSize]),
        m_bufferSize(bufferSize),
        m_freeList(reinterpret_cast<FreeBlock*>(m_buffer)),
        m_name("FreeListAllocator"),
        m_threadSafe(false),
        m_allocatedSize(0),
        m_peakUsage(0),
        m_allocationCount(0) {
        m_freeList->size = bufferSize;
        m_freeList->next = nullptr;
    }

    FreeListAllocator::~FreeListAllocator() {
        delete[] m_buffer;
    }

    void* FreeListAllocator::Allocate(size_t size, size_t alignment) {
        size_t totalSize = size + sizeof(AllocationHeader);
        size_t alignmentPadding = 0;
        FreeBlock* prev = nullptr;
        FreeBlock* current = m_freeList;

        while (current != nullptr) {
            uintptr_t currentAddress = reinterpret_cast<uintptr_t>(current);
            uintptr_t headerAddress = currentAddress + sizeof(FreeBlock);
            uintptr_t alignedAddress = (headerAddress + sizeof(AllocationHeader) + alignment - 1) & ~(alignment - 1);
            alignmentPadding = alignedAddress - headerAddress;

            size_t requiredSize = totalSize + alignmentPadding;

            if (current->size >= requiredSize) {
                if (current->size - requiredSize <= MIN_BLOCK_SIZE) {
                    requiredSize = current->size;
                    if (prev)
                        prev->next = current->next;
                    else
                        m_freeList = current->next;
                }
                else {
                    FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(reinterpret_cast<uint8_t*>(current) + requiredSize);
                    newBlock->size = current->size - requiredSize;
                    newBlock->next = current->next;
                    if (prev)
                        prev->next = newBlock;
                    else
                        m_freeList = newBlock;
                }

                AllocationHeader* header = reinterpret_cast<AllocationHeader*>(alignedAddress - sizeof(AllocationHeader));
                header->size = requiredSize;
                header->padding = static_cast<uint8_t>(alignmentPadding);

                size_t allocatedSize = requiredSize - sizeof(FreeBlock);
                m_allocatedSize.fetch_add(allocatedSize, std::memory_order_relaxed);
                m_allocationCount.fetch_add(1, std::memory_order_relaxed);
                m_peakUsage.store(std::max(m_peakUsage.load(std::memory_order_relaxed), m_allocatedSize.load(std::memory_order_relaxed)), std::memory_order_relaxed);

                return reinterpret_cast<void*>(alignedAddress);
            }

            prev = current;
            current = current->next;
        }

        return nullptr;
    }

    void FreeListAllocator::Free(void* ptr) {
        if (!ptr) return;

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));
        uintptr_t blockStart = reinterpret_cast<uintptr_t>(ptr) - sizeof(AllocationHeader) - header->padding;
        FreeBlock* blockToFree = reinterpret_cast<FreeBlock*>(blockStart);

        blockToFree->size = header->size;

        FreeBlock* prev = nullptr;
        FreeBlock* current = m_freeList;

        while (current != nullptr && current < blockToFree) {
            prev = current;
            current = current->next;
        }

        if (prev) {
            prev->next = blockToFree;
        }
        else {
            m_freeList = blockToFree;
        }
        blockToFree->next = current;

        if (reinterpret_cast<uint8_t*>(blockToFree) + blockToFree->size == reinterpret_cast<uint8_t*>(current)) {
            blockToFree->size += current->size;
            blockToFree->next = current->next;
        }

        if (prev && reinterpret_cast<uint8_t*>(prev) + prev->size == reinterpret_cast<uint8_t*>(blockToFree)) {
            prev->size += blockToFree->size;
            prev->next = blockToFree->next;
        }

        size_t freedSize = header->size - sizeof(FreeBlock);
        m_allocatedSize.fetch_sub(freedSize, std::memory_order_relaxed);
        m_allocationCount.fetch_sub(1, std::memory_order_relaxed);
    }

    void* FreeListAllocator::Reallocate(void* ptr, size_t newSize, size_t alignment) {
        if (!ptr) return Allocate(newSize, alignment);
        if (newSize == 0) {
            Free(ptr);
            return nullptr;
        }

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));
        size_t oldSize = header->size - sizeof(AllocationHeader) - header->padding;

        if (newSize <= oldSize) {
            return ptr;
        }

        void* newPtr = Allocate(newSize, alignment);
        if (newPtr) {
            std::memcpy(newPtr, ptr, std::min(oldSize, newSize));
            Free(ptr);
        }
        return newPtr;
    }

    size_t FreeListAllocator::GetAllocationSize(const void* ptr) const {
        if (!ptr) return 0;
        const AllocationHeader* header = reinterpret_cast<const AllocationHeader*>(static_cast<const uint8_t*>(ptr) - sizeof(AllocationHeader));
        return header->size - sizeof(AllocationHeader) - header->padding;
    }

    size_t FreeListAllocator::GetTotalAllocated() const {
        return m_allocatedSize.load(std::memory_order_relaxed);
    }

    size_t FreeListAllocator::GetPeakUsage() const {
        return m_peakUsage.load(std::memory_order_relaxed);
    }

    void FreeListAllocator::Reset() {
        m_freeList = reinterpret_cast<FreeBlock*>(m_buffer);
        m_freeList->size = m_bufferSize;
        m_freeList->next = nullptr;
        m_allocatedSize.store(0, std::memory_order_relaxed);
        m_allocationCount.store(0, std::memory_order_relaxed);
        m_peakUsage.store(0, std::memory_order_relaxed);
    }

    bool FreeListAllocator::Owns(const void* ptr) const {
        return ptr >= m_buffer && ptr < m_buffer + m_bufferSize;
    }

    size_t FreeListAllocator::GetAllocationCount() const {
        return m_allocationCount.load(std::memory_order_relaxed);
    }

    float FreeListAllocator::GetFragmentationPercentage() const {
        size_t freeMemory = 0;
        size_t largestFreeBlock = 0;
        for (FreeBlock* block = m_freeList; block != nullptr; block = block->next) {
            freeMemory += block->size;
            largestFreeBlock = std::max(largestFreeBlock, block->size);
        }
        return freeMemory > 0 ? (1.0f - static_cast<float>(largestFreeBlock) / freeMemory) * 100.0f : 0.0f;
    }

    void FreeListAllocator::SetName(const char* name) {
        m_name = name;
    }

    const char* FreeListAllocator::GetName() const {
        return m_name;
    }

    void FreeListAllocator::SetThreadSafe(bool threadSafe) {
        m_threadSafe = threadSafe;
    }

    bool FreeListAllocator::IsThreadSafe() const {
        return m_threadSafe;
    }

    bool FreeListAllocator::ValidateInternalState() const {
        size_t totalFreeSize = 0;
        for (FreeBlock* block = m_freeList; block != nullptr; block = block->next) {
            totalFreeSize += block->size;
            if (block->next && reinterpret_cast<uint8_t*>(block) + block->size > reinterpret_cast<uint8_t*>(block->next)) {
                return false;
            }
        }
        return totalFreeSize + m_allocatedSize.load(std::memory_order_relaxed) == m_bufferSize;
    }

    const char* FreeListAllocator::GetDetailedStats() const {
        static char stats[512];
        size_t freeMemory = 0;
        size_t largestFreeBlock = 0;
        size_t freeBlockCount = 0;
        for (FreeBlock* block = m_freeList; block != nullptr; block = block->next) {
            freeMemory += block->size;
            largestFreeBlock = std::max(largestFreeBlock, block->size);
            ++freeBlockCount;
        }
        snprintf(stats, sizeof(stats),
            "FreeListAllocator Stats:\n"
            "Total Size: %zu\n"
            "Allocated: %zu\n"
            "Free: %zu\n"
            "Peak Usage: %zu\n"
            "Allocation Count: %zu\n"
            "Free Block Count: %zu\n"
            "Largest Free Block: %zu\n"
            "Fragmentation: %.2f%%\n",
            m_bufferSize,
            m_allocatedSize.load(std::memory_order_relaxed),
            freeMemory,
            GetPeakUsage(),
            GetAllocationCount(),
            freeBlockCount,
            largestFreeBlock,
            GetFragmentationPercentage());
        return stats;
    }

} // namespace Core
