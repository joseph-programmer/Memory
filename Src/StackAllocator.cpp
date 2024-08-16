#include "StackAllocator.h"

namespace Core {

    StackAllocator::StackAllocator(size_t bufferSize)
        : m_buffer(new uint8_t[bufferSize])
        , m_bufferSize(bufferSize)
        , m_offset(0)
        , m_name("StackAllocator")
        , m_threadSafe(false)
        , m_peakUsage(0)
        , m_allocationCount(0) {}

    StackAllocator::~StackAllocator() {
        delete[] m_buffer;
    }

    void* StackAllocator::Allocate(size_t size, size_t alignment) {
        size_t headerSize = sizeof(AllocationHeader);
        size_t totalSize = size + headerSize;
        size_t padding = (alignment - ((m_offset + headerSize) % alignment)) % alignment;
        size_t newOffset = m_offset + totalSize + padding;

        if (newOffset > m_bufferSize) {
            return nullptr; // Out of memory
        }

        void* alignedAddress = m_buffer + m_offset + headerSize + padding;
        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(alignedAddress) - headerSize);
        header->size = size;
        header->adjustment = headerSize + padding;

        m_offset = newOffset;
        m_allocationCount.fetch_add(1, std::memory_order_relaxed);
        m_peakUsage.store(std::max(m_peakUsage.load(std::memory_order_relaxed), m_offset), std::memory_order_relaxed);

        return alignedAddress;
    }

    void StackAllocator::Free(void* ptr) {
        if (!ptr) return;

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));
        m_offset = reinterpret_cast<uint8_t*>(ptr) - m_buffer - header->adjustment;
        m_allocationCount.fetch_sub(1, std::memory_order_relaxed);
    }

    void* StackAllocator::Reallocate(void* ptr, size_t newSize, size_t alignment) {
        if (!ptr) return Allocate(newSize, alignment);

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(static_cast<uint8_t*>(ptr) - sizeof(AllocationHeader));
        size_t oldSize = header->size;
        size_t adjustment = header->adjustment;

        // Check if we can simply expand the current allocation
        if (reinterpret_cast<uint8_t*>(ptr) + oldSize == m_buffer + m_offset) {
            size_t additionalSpace = newSize - oldSize;
            if (m_offset + additionalSpace <= m_bufferSize) {
                m_offset += additionalSpace;
                header->size = newSize;
                m_peakUsage.store(std::max(m_peakUsage.load(std::memory_order_relaxed), m_offset), std::memory_order_relaxed);
                return ptr;
            }
        }

        // Otherwise, allocate new memory and copy
        void* newPtr = Allocate(newSize, alignment);
        if (newPtr) {
            std::memcpy(newPtr, ptr, std::min(oldSize, newSize));
            Free(ptr);
        }
        return newPtr;
    }

    size_t StackAllocator::GetAllocationSize(const void* ptr) const {
        if (!Owns(ptr)) return 0;
        const AllocationHeader* header = reinterpret_cast<const AllocationHeader*>(static_cast<const uint8_t*>(ptr) - sizeof(AllocationHeader));
        return header->size;
    }

    size_t StackAllocator::GetTotalAllocated() const {
        return m_offset;
    }

    size_t StackAllocator::GetPeakUsage() const {
        return m_peakUsage.load(std::memory_order_relaxed);
    }

    void StackAllocator::Reset() {
        m_offset = 0;
        m_allocationCount.store(0, std::memory_order_relaxed);
        m_markers.clear();
    }

    bool StackAllocator::Owns(const void* ptr) const {
        return ptr >= m_buffer && ptr < m_buffer + m_offset;
    }

    size_t StackAllocator::GetAllocationCount() const {
        return m_allocationCount.load(std::memory_order_relaxed);
    }

    float StackAllocator::GetFragmentationPercentage() const {
        return 0.0f;
    }

    void StackAllocator::SetName(const char* name) {
        m_name = name;
    }

    const char* StackAllocator::GetName() const {
        return m_name;
    }

    void StackAllocator::SetThreadSafe(bool threadSafe) {
        m_threadSafe = threadSafe;
    }

    bool StackAllocator::IsThreadSafe() const {
        return m_threadSafe;
    }

    bool StackAllocator::ValidateInternalState() const {
        return m_offset <= m_bufferSize;
    }

    const char* StackAllocator::GetDetailedStats() const {
        static char stats[256];
        snprintf(stats, sizeof(stats),
            "StackAllocator Stats:\n"
            "Total Size: %zu\n"
            "Used: %zu\n"
            "Peak Usage: %zu\n"
            "Allocation Count: %zu\n"
            "Marker Count: %zu\n",
            m_bufferSize, m_offset, GetPeakUsage(), GetAllocationCount(), m_markers.size());
        return stats;
    }

    size_t StackAllocator::GetMarker() const {
        return m_offset;
    }

    void StackAllocator::FreeToMarker(size_t marker) {
        if (marker <= m_offset) {
            m_offset = marker;
        }
    }

    void StackAllocator::PushMarker() {
        m_markers.push_back(m_offset);
    }

    void StackAllocator::PopMarker() {
        if (!m_markers.empty()) {
            FreeToMarker(m_markers.back());
            m_markers.pop_back();
        }
    }

} // namespace Core
