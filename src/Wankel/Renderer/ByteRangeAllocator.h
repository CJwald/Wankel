#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Wankel {

// First-fit free-list allocator over a fixed-size byte range, with coalescing on free - generic
// enough for sub-allocating any combined GPU buffer (vertex/index data, an SSBO array, ...), not
// tied to any one caller. Not thread-safe; not designed for a hot per-frame path (linear scan of
// the free list) - meant for the "changes on generate/evict" cadence a streaming chunk pool has,
// not something called every frame for every visible chunk.
class ByteRangeAllocator {
public:
    static constexpr size_t kFailed = SIZE_MAX;

    explicit ByteRangeAllocator(size_t capacity) : m_Capacity(capacity) {
        if (capacity > 0)
            m_FreeList.push_back({0, capacity});
    }

    // Returns the byte offset of a `size`-byte range, or kFailed if no single free block is large
    // enough (caller should fall back rather than trying to defragment - see ChunkGeometryPool).
    size_t Alloc(size_t size) {
        if (size == 0)
            return 0;

        for (size_t i = 0; i < m_FreeList.size(); i++) {
            if (m_FreeList[i].Size < size)
                continue;

            size_t offset = m_FreeList[i].Offset;
            if (m_FreeList[i].Size == size) {
                m_FreeList.erase(m_FreeList.begin() + (long)i);
            } else {
                m_FreeList[i].Offset += size;
                m_FreeList[i].Size -= size;
            }
            return offset;
        }
        return kFailed;
    }

    // Returns a previously-Alloc'd range. offset/size must exactly match what Alloc returned -
    // this allocator has no bookkeeping of live allocations, that's the caller's responsibility
    // (see ChunkGeometryPool's own allocation records).
    void Free(size_t offset, size_t size) {
        if (size == 0)
            return;

        // Insert in offset order, then coalesce with an adjacent neighbor on either side - keeps
        // the free list from fragmenting into many small blocks under sustained alloc/free churn
        // (exactly the streaming pattern chunks generate/evict under).
        auto it = m_FreeList.begin();
        while (it != m_FreeList.end() && it->Offset < offset)
            ++it;

        it = m_FreeList.insert(it, {offset, size});

        if (it + 1 != m_FreeList.end() && it->Offset + it->Size == (it + 1)->Offset) {
            it->Size += (it + 1)->Size;
            m_FreeList.erase(it + 1);
        }
        if (it != m_FreeList.begin() && (it - 1)->Offset + (it - 1)->Size == it->Offset) {
            (it - 1)->Size += it->Size;
            m_FreeList.erase(it);
        }
    }

    size_t GetCapacity() const { return m_Capacity; }

private:
    struct Block {
        size_t Offset;
        size_t Size;
    };

    size_t m_Capacity;
    std::vector<Block> m_FreeList;
};

} // namespace Wankel
