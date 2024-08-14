#ifndef IALLOCATOR_H
#define IALLOCATOR_H

#include <cstddef>
#include <cstdint>

namespace Core {

/**
 * @class IAllocator
 * @brief Interface for memory allocation management.
 *
 * Provides a standard interface for allocating, freeing, and managing memory. 
 * This class is intended to be implemented by various types of memory allocators.
 */
class IAllocator {
public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~IAllocator() = default;

    /**
     * @brief Allocates memory of a specified size and alignment.
     * @param size The size of the memory block to allocate.
     * @param alignment The alignment requirement for the memory block.
     * @return A pointer to the allocated memory block, or nullptr if allocation fails.
     */
    virtual void* Allocate(size_t size, size_t alignment) = 0;

    /**
     * @brief Frees previously allocated memory.
     * @param ptr A pointer to the memory block to free.
     */
    virtual void Free(void* ptr) = 0;

    /**
     * @brief Reallocates memory, potentially moving it to a new location.
     * @param ptr A pointer to the memory block to reallocate.
     * @param newSize The new size of the memory block.
     * @param alignment The alignment requirement for the new memory block.
     * @return A pointer to the reallocated memory block, or nullptr if reallocation fails.
     */
    virtual void* Reallocate(void* ptr, size_t newSize, size_t alignment) = 0;

    /**
     * @brief Gets the size of an allocated memory block.
     * @param ptr A pointer to the memory block.
     * @return The size of the allocated memory block.
     */
    virtual size_t GetAllocationSize(const void* ptr) const = 0;

    /**
     * @brief Gets the total amount of memory currently allocated.
     * @return The total amount of memory currently allocated.
     */
    virtual size_t GetTotalAllocated() const = 0;

    /**
     * @brief Gets the peak memory usage.
     * @return The peak memory usage.
     */
    virtual size_t GetPeakUsage() const = 0;

    /**
     * @brief Resets the allocator, freeing all allocations.
     */
    virtual void Reset() = 0;

    /**
     * @brief Checks if the allocator owns a specific memory block.
     * @param ptr A pointer to the memory block.
     * @return True if the allocator owns the memory block, false otherwise.
     */
    virtual bool Owns(const void* ptr) const = 0;

    /**
     * @brief Gets the number of active allocations.
     * @return The number of active allocations.
     */
    virtual size_t GetAllocationCount() const = 0;

    /**
     * @brief Gets the fragmentation percentage.
     * @return The fragmentation percentage.
     */
    virtual float GetFragmentationPercentage() const = 0;

    /**
     * @brief Sets a name for this allocator (useful for debugging).
     * @param name The name to set.
     */
    virtual void SetName(const char* name) = 0;

    /**
     * @brief Gets the name of this allocator.
     * @return The name of the allocator.
     */
    virtual const char* GetName() const = 0;

    /**
     * @brief Enables or disables thread safety (if supported).
     * @param threadSafe True to enable thread safety, false to disable.
     */
    virtual void SetThreadSafe(bool threadSafe) = 0;

    /**
     * @brief Checks if the allocator is thread-safe.
     * @return True if the allocator is thread-safe, false otherwise.
     */
    virtual bool IsThreadSafe() const = 0;

    /**
     * @brief Performs an internal consistency check (for debugging).
     * @return True if the internal state is consistent, false otherwise.
     */
    virtual bool ValidateInternalState() const = 0;

    /**
     * @brief Gets a detailed memory usage report.
     * @return A string in an implementation-defined format containing detailed memory usage information.
     */
    virtual const char* GetDetailedStats() const = 0;
};

/**
 * @brief Helper template for aligned allocation.
 * @tparam T The type of the object to allocate.
 * @tparam Args The types of the arguments to forward to the constructor.
 * @param allocator The allocator to use for memory allocation.
 * @param args The arguments to forward to the constructor.
 * @return A pointer to the newly allocated and constructed object, or nullptr if allocation fails.
 */
template<typename T, typename... Args>
T* AllocateAligned(IAllocator& allocator, Args&&... args) {
    void* ptr = allocator.Allocate(sizeof(T), alignof(T));
    return ptr ? new(ptr) T(std::forward<Args>(args)...) : nullptr;
}

/**
 * @brief Helper template for aligned deallocation.
 * @tparam T The type of the object to deallocate.
 * @param allocator The allocator to use for memory deallocation.
 * @param ptr A pointer to the object to deallocate.
 */
template<typename T>
void DeallocateAligned(IAllocator& allocator, T* ptr) {
    if (ptr) {
        ptr->~T();
        allocator.Free(ptr);
    }
}

} // namespace Core

#endif // IALLOCATOR_H
