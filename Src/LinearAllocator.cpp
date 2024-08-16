#include "LinearAllocator.h"

namespace Core {

    LinearAllocator::LinearAllocator(size_t bufferSize)
        : m_buffer(new uint8_t[bufferSize])
        , m_bufferSize(bufferSize)
        , m_offset(0)
        , m_name("LinearAllocator")
        , m_threadSafe(false)
        , m_peakUsage(0)
        , m_allocationCount(0) {}

    LinearAllocator::~LinearAllocator() {
        delete[] m_buffer;
    }

    void* LinearAllocator::Allocate(size_t size, size_t alignment) {
        size_t padding = (alignment - (m_offset % alignment)) % alignment;
        size_t newOffset = m_offset + padding + size;

        if (newOffset > m_bufferSize) {
            return nullptr; // Out of memory
        }

        void* ptr = m_buffer + m_offset + padding;
        m_offset = newOffset;

        m_allocationCount.fetch_add(1, std::memory_order_relaxed);
        m_peakUsage.store(std::max(m_peakUsage.load(std::memory_order_relaxed), m_offset), std::memory_order_relaxed);

        return ptr;
    }

    void LinearAllocator::Free(void* ptr) {
        // Linear allocator doesn't support individual deallocation
        (void)ptr;
    }

    void* LinearAllocator::Reallocate(void* ptr, size_t newSize, size_t alignment) {
        void* newPtr = Allocate(newSize, alignment);
        if (newPtr && ptr) {
            std::memcpy(newPtr, ptr, std::min(newSize, GetAllocationSize(ptr)));
        }
        return newPtr;
    }

    size_t LinearAllocator::GetAllocationSize(const void* ptr) const {
        if (ptr < m_buffer || ptr >= m_buffer + m_offset) {
            return 0;
        }
        return m_buffer + m_offset - static_cast<const uint8_t*>(ptr);
    }

    size_t LinearAllocator::GetTotalAllocated() const {
        return m_offset;
    }

    size_t LinearAllocator::GetPeakUsage() const {
        return m_peakUsage.load(std::memory_order_relaxed);
    }

    void LinearAllocator::Reset() {
        m_offset = 0;
        m_allocationCount.store(0, std::memory_order_relaxed);
    }

    bool LinearAllocator::Owns(const void* ptr) const {
        return ptr >= m_buffer && ptr < m_buffer + m_offset;
    }

    size_t LinearAllocator::GetAllocationCount() const {
        return m_allocationCount.load(std::memory_order_relaxed);
    }

    float LinearAllocator::GetFragmentationPercentage() const {
        return 0.0f;
    }

    void LinearAllocator::SetName(const char* name) {
        m_name = name;
    }

    const char* LinearAllocator::GetName() const {
        return m_name;
    }

    void LinearAllocator::SetThreadSafe(bool threadSafe) {
        m_threadSafe = threadSafe;
    }

    bool LinearAllocator::IsThreadSafe() const {
        return m_threadSafe;
    }

    bool LinearAllocator::ValidateInternalState() const {
        return m_offset <= m_bufferSize;
    }

    const char* LinearAllocator::GetDetailedStats() const {
        static char stats[256];
        snprintf(stats, sizeof(stats),
            "LinearAllocator Stats:\n"
            "Total Size: %zu\n"
            "Used: %zu\n"
            "Peak Usage: %zu\n"
            "Allocation Count: %zu\n",
            m_bufferSize, m_offset, GetPeakUsage(), GetAllocationCount());
        return stats;
    }

} // namespace Core
