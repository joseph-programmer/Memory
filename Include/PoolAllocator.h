#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#include "IAllocator.h"
#include <vector>
#include <cstring>
#include <mutex>

namespace Core {

    /**
     * @class PoolAllocator
     * @brief A memory pool allocator for efficient memory management.
     */
    class PoolAllocator : public IAllocator {
    
    public:
        /**
         * @brief Constructor to initialize the PoolAllocator with pool configurations.
         * @param poolConfigs A list of pairs indicating block size and count for each pool.
         */
        PoolAllocator(std::initializer_list<std::pair<size_t, size_t>> poolConfigs);

        /**
         * @brief Destructor to free all allocated memory pools.
         */
        ~PoolAllocator() override;

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
        void SetName(const char* allocatorName) override;
        const char* GetName() const override;
        void SetThreadSafe(bool isThreadSafe) override;
        bool IsThreadSafe() const override;
        bool ValidateInternalState() const override;
        const char* GetDetailedStats() const override;

    private:
        struct Block {
            Block* next; /**< Pointer to the next block in the free list. */
        };

        struct Pool {
            void* memory; /**< Pointer to the memory allocated for the pool. */
            size_t blockSize; /**< Size of each block in the pool. */
            size_t blockCount; /**< Number of blocks in the pool. */
            Block* freeList; /**< Pointer to the first free block in the pool. */
        };

        std::vector<Pool> pools; /**< Vector containing all memory pools. */
        size_t totalAllocated; /**< Total memory allocated. */
        size_t peakUsage; /**< Peak memory usage recorded. */
        size_t allocationCount; /**< Number of allocations made. */
        const char* name; /**< Name of the allocator. */
        bool threadSafe; /**< Flag indicating if the allocator is thread-safe. */
        mutable std::mutex mutex; /**< Mutex for thread-safety. */

        Pool& findPool(size_t size);
        void lock() const;
        void unlock() const;

    };

} // namespace Core

#endif // POOL_ALLOCATOR_H
