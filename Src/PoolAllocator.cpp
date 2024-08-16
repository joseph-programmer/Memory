#include "PoolAllocator.h"
#include <stdexcept>
#include <algorithm>

namespace Core {

    PoolAllocator::PoolAllocator(std::initializer_list<std::pair<size_t, size_t>> poolConfigs)
        : totalAllocated(0), peakUsage(0), allocationCount(0), name(""), threadSafe(false) {
        for (const auto& config : poolConfigs) {
            size_t blockSize = config.first;
            size_t blockCount = config.second;
            size_t poolSize = blockSize * blockCount;

            void* memory = std::malloc(poolSize);
            if (!memory) {
                throw std::bad_alloc();
            }

            Block* freeList = nullptr;
            char* start = static_cast<char*>(memory);
            for (size_t i = 0; i < blockCount; ++i) {
                Block* block = reinterpret_cast<Block*>(start + i * blockSize);
                block->next = freeList;
                freeList = block;
            }

            pools.push_back({ memory, blockSize, blockCount, freeList });
        }
    }

    PoolAllocator::~PoolAllocator() {
        for (const auto& pool : pools) {
            std::free(pool.memory);
        }
    }

    void* PoolAllocator::Allocate(size_t size, size_t alignment) {
        lock();
        Pool& pool = findPool(size);
        if (!pool.freeList) {
            unlock();
            return nullptr;
        }

        Block* block = pool.freeList;
        pool.freeList = block->next;

        totalAllocated += pool.blockSize;
        allocationCount++;
        peakUsage = std::max(peakUsage, totalAllocated);

        unlock();
        return block;
    }

    void PoolAllocator::Free(void* ptr) {
        if (!ptr) return;

        lock();
        for (auto& pool : pools) {
            if (ptr >= pool.memory && ptr < static_cast<char*>(pool.memory) + pool.blockSize * pool.blockCount) {
                Block* block = static_cast<Block*>(ptr);
                block->next = pool.freeList;
                pool.freeList = block;

                totalAllocated -= pool.blockSize;
                allocationCount--;
                break;
            }
        }
        unlock();
    }

    void* PoolAllocator::Reallocate(void* ptr, size_t newSize, size_t alignment) {
        void* newPtr = Allocate(newSize, alignment);
        if (newPtr && ptr) {
            size_t oldSize = GetAllocationSize(ptr);
            std::memcpy(newPtr, ptr, std::min(oldSize, newSize));
            Free(ptr);
        }
        return newPtr;
    }

    size_t PoolAllocator::GetAllocationSize(const void* ptr) const {
        lock();
        for (const auto& pool : pools) {
            if (ptr >= pool.memory && ptr < static_cast<const char*>(pool.memory) + pool.blockSize * pool.blockCount) {
                unlock();
                return pool.blockSize;
            }
        }
        unlock();
        return 0;
    }

    size_t PoolAllocator::GetTotalAllocated() const {
        lock();
        size_t total = totalAllocated;
        unlock();
        return total;
    }

    size_t PoolAllocator::GetPeakUsage() const {
        lock();
        size_t peak = peakUsage;
        unlock();
        return peak;
    }

    void PoolAllocator::Reset() {
        lock();
        for (auto& pool : pools) {
            Block* freeList = nullptr;
            char* start = static_cast<char*>(pool.memory);
            for (size_t i = 0; i < pool.blockCount; ++i) {
                Block* block = reinterpret_cast<Block*>(start + i * pool.blockSize);
                block->next = freeList;
                freeList = block;
            }
            pool.freeList = freeList;
        }
        totalAllocated = 0;
        allocationCount = 0;
        unlock();
    }

    bool PoolAllocator::Owns(const void* ptr) const {
        lock();
        for (const auto& pool : pools) {
            if (ptr >= pool.memory && ptr < static_cast<const char*>(pool.memory) + pool.blockSize * pool.blockCount) {
                unlock();
                return true;
            }
        }
        unlock();
        return false;
    }

    size_t PoolAllocator::GetAllocationCount() const {
        lock();
        size_t count = allocationCount;
        unlock();
        return count;
    }

    float PoolAllocator::GetFragmentationPercentage() const {
        return 0.0f;
    }

    void PoolAllocator::SetName(const char* allocatorName) {
        lock();
        name = allocatorName;
        unlock();
    }

    const char* PoolAllocator::GetName() const {
        lock();
        const char* allocatorName = name;
        unlock();
        return allocatorName;
    }

    void PoolAllocator::SetThreadSafe(bool isThreadSafe) {
        lock();
        threadSafe = isThreadSafe;
        unlock();
    }

    bool PoolAllocator::IsThreadSafe() const {
        lock();
        bool isThreadSafe = threadSafe;
        unlock();
        return isThreadSafe;
    }

    bool PoolAllocator::ValidateInternalState() const {
        lock();
        // Add internal consistency checks here
        unlock();
        return true;
    }

    const char* PoolAllocator::GetDetailedStats() const {
        return "Detailed stats not implemented";
    }

    PoolAllocator::Pool& PoolAllocator::findPool(size_t size) {
        for (auto& pool : pools) {
            if (size <= pool.blockSize) {
                return pool;
            }
        }
        throw std::runtime_error("No suitable pool found");
    }

    void PoolAllocator::lock() const {
        if (threadSafe) {
            mutex.lock();
        }
    }

    void PoolAllocator::unlock() const {
        if (threadSafe) {
            mutex.unlock();
        }
    }

} // namespace Core
