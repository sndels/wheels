#ifndef WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP

#include "../containers/array.hpp"
#include "../containers/pair.hpp"
#include "allocator.hpp"
#include "utils.hpp"

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdlib>

// Based on
// Implementation of a constant-time dynamic storage allocator
// By Masmano et al.

// The implementation is NOT thread-safe

namespace wheels
{

class TlsfAllocator : public Allocator
{
  public:
    struct Stats
    {
        size_t allocation_count{0};
        size_t small_allocation_count{0};
        size_t allocated_byte_count{0};
        size_t allocated_byte_count_high_watermark{0};
        size_t free_byte_count{0};
    };

    // Note that the allocator might not be able allocate a single block with
    // size near or matching the capacity due to how available blocks are
    // searched internally.
    TlsfAllocator(size_t capacity);
    ~TlsfAllocator();

    TlsfAllocator(TlsfAllocator const &other) = delete;
    TlsfAllocator(TlsfAllocator &&other) = delete;
    TlsfAllocator &operator=(TlsfAllocator const &other) = delete;
    TlsfAllocator &operator=(TlsfAllocator &&other) = delete;

    [[nodiscard]] virtual void *allocate(size_t num_bytes) override;
    virtual void deallocate(void *ptr) override;

    Stats const &stats() const;

  private:
    using BitMapT = size_t;

    static constexpr size_t s_J = 5;
    static constexpr size_t s_second_level_range_count = pow2(s_J);
    static_assert(
        sizeof(BitMapT) * CHAR_BIT >= pow2(s_J),
        "Second level ranges have to fit the bitmap type");
    // Smaller sizes would require special handling as second level lists could
    // have extra ranges
    // TODO:
    // Can safely drop down to s_second_level_range_count? Did the paper pick
    // a hard-coded 128 for some other reason?
    static constexpr size_t s_min_block_size = 128;

    // Boundary tags are placed at the front and back of all blocks, marking
    // the size and status of the block for merging operations. This means that
    // the front and back boundaries have to be aligned for BoundaryTag.
    // Boundary tags with allocated=1, size=0 are also placed on either side of
    // the inital block so that merges are skipped correctly at its boundaries

    // Free blocks also have the pointers required for the linked list after the
    // front tag.
    // -------------------
    //    Boundary tag
    //    ptr to prev
    //    ptr to next
    // -------------------
    //
    //     ........
    //
    // -------------------
    //    Boundary tag
    // -------------------

    // Allocated blocks have a pointer to the front of the block right
    // before the address returned from allocate().
    // -------------------
    //    Boundary tag
    // -------------------
    //  0 or more padding
    // -------------------
    //  ptr to front (tag)
    // -------------------
    //     alloc ptr

    //     ........
    //
    // -------------------
    //    Boundary tag
    // -------------------

    struct BoundaryTag
    {
        size_t allocated : 1;
        size_t byte_count : sizeof(size_t) * CHAR_BIT - 1;
    };
    static_assert(sizeof(BoundaryTag) == sizeof(size_t));
    static_assert(alignof(BoundaryTag) == alignof(size_t));
    static_assert(
        alignof(BoundaryTag) == alignof(void *),
        "We should be able to pack the pointer to front right after the front "
        "tag");

    static constexpr size_t s_flag_allocated = (size_t) true;
    static constexpr size_t s_flag_free = (size_t) false;

    // Need space for boundary tag and pointer to the front of the block,
    // and after that the required alignment for the actual allocation
    static constexpr size_t s_pre_alloc_padding =
        sizeof(BoundaryTag) + sizeof(void *) + alignof(std::max_align_t);

    struct FreeBlock
    {
        BoundaryTag tag{0};
        FreeBlock *previous{nullptr};
        FreeBlock *next{nullptr};
    };
    static_assert(
        alignof(BoundaryTag) == alignof(FreeBlock) &&
        "We assume we can place the free block over the boundary tag, aliasing "
        "it with the first field");
    static_assert(
        offsetof(FreeBlock, tag) == 0 &&
        "We assume we can place the free block over the boundary tag, aliasing "
        "it with the first field");
    static_assert(
        s_min_block_size > sizeof(FreeBlock) + sizeof(BoundaryTag),
        "Freed blocks must have space for the linked list element and boundary "
        "tag at the back");

    // We'll have at most size_t bits indices so 255 is plenty
    struct FreeListIndex
    {
        uint8_t fl;
        uint8_t sl;

        bool is_null() const { return fl == (uint8_t)255; }

        static constexpr FreeListIndex null()
        {
            return FreeListIndex{255, 255};
        }
    };

    [[nodiscard]] static constexpr size_t fls(size_t v)
    {
        return std::countr_zero(std::bit_floor(v));
    }

    [[nodiscard]] static constexpr size_t ffs(size_t v)
    {
        return std::countr_zero(v);
    }

    [[nodiscard]] static constexpr size_t first_level_bitmap_element(
        size_t block_size)
    {
        return fls(block_size) / (sizeof(BitMapT) * CHAR_BIT);
    }

    [[nodiscard]] static constexpr size_t first_level_bitmap_bit(
        size_t block_size)
    {
        return fls(block_size) % (sizeof(BitMapT) * CHAR_BIT);
    }

    [[nodiscard]] static constexpr FreeListIndex mapping_insert(size_t r)
    {
        size_t const i = fls(r);
        size_t const j = (r >> (i - s_J)) - pow2(s_J);
        assert(i < 255);
        assert(j < 255);

        return FreeListIndex{(uint8_t)i, (uint8_t)j};
    }

    [[nodiscard]] static constexpr FreeListIndex mapping_search(size_t r)
    {
        // Round up size to the next range, so that whatever block we find is
        // large enough
        r += (1 << (fls(r) - s_J)) - 1;
        return mapping_insert(r);
    }

    static void copy_front_tag_to_back(FreeBlock *block)
    {
        assert(block->tag.byte_count >= s_min_block_size);

        uintptr_t const tag_addr =
            (uintptr_t)block + block->tag.byte_count - sizeof(BoundaryTag);
        assert(tag_addr % sizeof(BoundaryTag) == 0);

        BoundaryTag *tag = (BoundaryTag *)tag_addr;
        *tag = block->tag;
    }

    [[nodiscard]] static bool front_and_back_tags_match(FreeBlock *block)
    {
        uintptr_t const tag_addr =
            (uintptr_t)block + block->tag.byte_count - sizeof(BoundaryTag);
        assert(tag_addr % sizeof(BoundaryTag) == 0);
        BoundaryTag *tag = (BoundaryTag *)tag_addr;

        bool const flags_match = tag->allocated == block->tag.allocated;
        bool const sizes_match = tag->byte_count == block->tag.byte_count;

        return flags_match && sizes_match;
    }

    // Returns a pointer to the head of a freelist whose head is suitable
    [[nodiscard]] FreeListIndex find_suitable_block(FreeListIndex start_index);

    // Also updates the bitmaps if necessary
    void insert_block(FreeBlock *block);
    // Also updates the bitmaps if necessary
    [[nodiscard]] FreeBlock *remove_head(FreeListIndex index);
    // Also updates the bitmaps if necessary
    void remove_block(FreeBlock *block);

    [[nodiscard]] FreeBlock *split_block(
        FreeBlock *block, size_t first_byte_count);

    [[nodiscard]] FreeBlock *merge_previous(FreeBlock *block);
    [[nodiscard]] FreeBlock *merge_next(FreeBlock *block);

    using SecondLevelRangesLists = FreeBlock *[s_second_level_range_count];

    void *m_data{nullptr};
    void *m_first_block_addr{nullptr};
    size_t m_full_size{0};
    Stats m_stats;
    // Each bit is one first level bucket
    BitMapT m_first_level_bitmap{0};
    // Each element is one first level bucket
    Span<BitMapT> m_second_level_bitmaps;
    Span<SecondLevelRangesLists> m_segregated_lists;
};

inline TlsfAllocator::TlsfAllocator(size_t capacity)
{
    // Let's assume we have a few first-level buckets, first one will be 128
    assert(capacity >= kilobytes(1));

    // Boundary tags will be written after the main block
    capacity = aligned_offset(capacity, alignof(BoundaryTag));
    m_stats.free_byte_count = capacity;

    size_t const first_level_bucket_count = fls(capacity) + 1;

    static_assert(
        sizeof(BitMapT) == alignof(SecondLevelRangesLists),
        "Implementation expects that the metadata can be packed tightly");
    size_t const metadata_size =
        sizeof(BitMapT) + sizeof(BitMapT) * first_level_bucket_count +
        sizeof(SecondLevelRangesLists) * first_level_bucket_count +
        sizeof(BoundaryTag) + alignof(std::max_align_t);

    // Need alignment and space for the back boundary tag. Could skip alignment
    // if allocation alignment and size are nice, but let's not complicate
    // things for the 8 extra bytes.
    size_t block_size = s_pre_alloc_padding + capacity;
    block_size = aligned_offset(block_size, alignof(BoundaryTag));
    block_size += sizeof(BoundaryTag);

    // Metadata and the memory pool live are backed by the same allocation
    m_full_size = metadata_size + block_size + sizeof(BoundaryTag);
    m_data = std::malloc(m_full_size);

    // Set up metadata
    m_second_level_bitmaps =
        Span{(BitMapT *)(m_data), first_level_bucket_count};
    memset(m_second_level_bitmaps.begin(), 0, m_second_level_bitmaps.size() * sizeof(BitMapT));
    m_segregated_lists = Span{
        (SecondLevelRangesLists *)m_second_level_bitmaps.end(),
        first_level_bucket_count};
    memset(m_segregated_lists.begin(), 0, m_segregated_lists.size() * sizeof(SecondLevelRangesLists));

    uintptr_t front_tag_addr = (uintptr_t)m_segregated_lists.end();
    assert(front_tag_addr % alignof(BoundaryTag) == 0);
    BoundaryTag *front_tag = (BoundaryTag *)front_tag_addr;
    front_tag->allocated = s_flag_allocated;
    front_tag->byte_count = 0;

    uintptr_t back_tag_addr = front_tag_addr + sizeof(BoundaryTag) + block_size;
    assert(back_tag_addr % alignof(BoundaryTag) == 0);
    BoundaryTag *back_tag = (BoundaryTag *)back_tag_addr;
    back_tag->allocated = s_flag_allocated;
    back_tag->byte_count = 0;

    // Insert the empty block that's after the metadata
    void *first_addr = (void *)(front_tag_addr + sizeof(BoundaryTag));
    first_addr = aligned_ptr<FreeBlock>(first_addr);
    assert((uintptr_t)first_addr - (uintptr_t)m_data <= metadata_size);
    assert((uintptr_t)first_addr % alignof(BoundaryTag) == 0);
    m_first_block_addr = first_addr;

    FreeBlock *block = (FreeBlock *)first_addr;
    block->tag.allocated = s_flag_free;
    block->tag.byte_count = block_size;
    block->previous = nullptr;
    block->next = nullptr;
    copy_front_tag_to_back(block);

    insert_block(block);
}

inline TlsfAllocator::~TlsfAllocator()
{
    assert(
        std::popcount(m_first_level_bitmap) == 1 &&
        "Expected one contiguous block remaining. Not all allocations were "
        "deallocated before the allocator was destroyed.");
    assert(m_stats.allocation_count == 0);
    assert(m_stats.small_allocation_count == 0);
    assert(m_stats.allocated_byte_count == 0);

    std::free(m_data);
}

inline void *TlsfAllocator::allocate(size_t num_bytes)
{
    // Need alignment and space for the back boundary tag. Could skip alignment
    // if allocation alignment and size are nice, but let's not complicate
    // things for the 8 extra bytes.
    num_bytes = s_pre_alloc_padding + num_bytes;
    num_bytes = aligned_offset(num_bytes, alignof(BoundaryTag));
    num_bytes += sizeof(BoundaryTag);
    if (num_bytes < s_min_block_size)
        num_bytes = s_min_block_size;

    // First list that could have blocks we can use
    FreeListIndex index = mapping_search(num_bytes);

    // Actual first list that has blocks we can use
    index = find_suitable_block(index);
    if (index.is_null())
        // No suiltable blocks left
        return nullptr;
    assert(m_segregated_lists[index.fl][index.sl] != nullptr);

    FreeBlock *block = remove_head(index);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= num_bytes);

    // Need to split and put potential extra memory back into free blocks
    if (block->tag.byte_count - num_bytes > s_min_block_size)
    {
        FreeBlock *remaining_block = split_block(block, num_bytes);
        insert_block(remaining_block);
    }

    // Actual memory should be aligned after the front tag and ptr-to-front
    void *alloc_ptr =
        (void *)((uintptr_t)block + sizeof(BoundaryTag) + sizeof(void *));
    alloc_ptr = aligned_ptr<std::max_align_t>(alloc_ptr);

    // Need to insert pointer to front so deallocate can find the front tag
    uintptr_t const ptr_to_front_addr = (uintptr_t)alloc_ptr - sizeof(void *);
    assert(ptr_to_front_addr % alignof(void *) == 0);
    void **ptr_to_front = (void **)ptr_to_front_addr;
    *ptr_to_front = (void *)block;

    block->tag.allocated = s_flag_allocated;
    copy_front_tag_to_back(block);

    m_stats.allocation_count++;
    if (block->tag.byte_count == s_min_block_size)
        m_stats.small_allocation_count++;
    m_stats.free_byte_count -= block->tag.byte_count;
    m_stats.allocated_byte_count += block->tag.byte_count;
    if (m_stats.allocated_byte_count >
        m_stats.allocated_byte_count_high_watermark)
    {
        m_stats.allocated_byte_count_high_watermark =
            m_stats.allocated_byte_count;
    }

    return alloc_ptr;
}

inline void TlsfAllocator::deallocate(void *ptr)
{
    if (ptr == nullptr)
        return;

    assert(
        (uintptr_t)ptr > (uintptr_t)m_data &&
        (uintptr_t)ptr - (uintptr_t)m_data < m_full_size);

    // We stored a pointer to the front of the block just before the allocation
    // pointer
    uintptr_t const ptr_to_front_addr = (uintptr_t)ptr - sizeof(void *);
    assert(ptr_to_front_addr % alignof(void *) == 0);
    void *ptr_to_front = *(void **)ptr_to_front_addr;

    assert((uintptr_t)ptr_to_front % alignof(FreeBlock) == 0);
    // This aliases the front tag that's already there
    FreeBlock *block = (FreeBlock *)ptr_to_front;
    assert(block->tag.allocated == s_flag_allocated);
    assert(block->tag.byte_count >= s_min_block_size);
    block->next = nullptr;
    block->previous = nullptr;

    // Update before merging with the size of the original freed block
    m_stats.allocation_count--;
    if (block->tag.byte_count == s_min_block_size)
        m_stats.small_allocation_count--;
    m_stats.free_byte_count += block->tag.byte_count;
    m_stats.allocated_byte_count -= block->tag.byte_count;

    block->tag.allocated = s_flag_free;
    copy_front_tag_to_back(block);

    // Do merging to avoid needless fragmentation
    block = merge_previous(block);
    block = merge_next(block);
    assert(block->tag.allocated == s_flag_free);

    // Tag or end tag location might have changed 
    copy_front_tag_to_back(block);

    insert_block(block);
}

inline TlsfAllocator::Stats const &TlsfAllocator::stats() const
{
    return m_stats;
}

inline TlsfAllocator::FreeListIndex TlsfAllocator::find_suitable_block(
    FreeListIndex start_index)
{
    BitMapT bitmap_tmp =
        m_second_level_bitmaps[start_index.fl] & ((size_t)-1 << start_index.sl);

    if (bitmap_tmp != 0)
    {
        size_t const non_empty_sl = ffs(bitmap_tmp);
        assert(non_empty_sl < 255);
        return FreeListIndex{start_index.fl, (uint8_t)non_empty_sl};
    }

    bitmap_tmp = m_first_level_bitmap & ((size_t)-1 << (start_index.fl + 1));
    if (bitmap_tmp == 0)
        return FreeListIndex::null();

    size_t const non_empty_fl = ffs(bitmap_tmp);
    size_t const non_empty_sl = ffs(m_second_level_bitmaps[non_empty_fl]);
    assert(non_empty_fl < 255);
    assert(non_empty_sl < 255);

    return FreeListIndex{(uint8_t)non_empty_fl, (uint8_t)non_empty_sl};
}

inline void TlsfAllocator::insert_block(FreeBlock *block)
{
    assert(block != nullptr);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->previous == nullptr);
    assert(block->next == nullptr);

    FreeListIndex const index = mapping_insert(block->tag.byte_count);

    FreeBlock **list_ptr = &m_segregated_lists[index.fl][index.sl];

    if (*list_ptr == nullptr)
    {
        // Add the newly popupated list from bitmaps
        m_first_level_bitmap |= (size_t)1 << index.fl;
        m_second_level_bitmaps[index.fl] |= (size_t)1 << index.sl;
    }
    else
    {
        // Just add the block to the linked list
        (*list_ptr)->previous = block;
        block->next = *list_ptr;
    }

    *list_ptr = block;
}

inline TlsfAllocator::FreeBlock *TlsfAllocator::remove_head(FreeListIndex index)
{
    FreeBlock **list_ptr = &m_segregated_lists[index.fl][index.sl];
    assert(*list_ptr != nullptr);

    FreeBlock *block = *list_ptr;
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= s_min_block_size);

    if (block->next == nullptr)
    {
        *list_ptr = nullptr;

        // Remove the now empty list from bitmaps
        m_first_level_bitmap &= ~(1 << index.fl);
        m_second_level_bitmaps[index.fl] &= ~(1 << index.sl);
    }
    else
    {
        // Just pop the first element from the linked list
        *list_ptr = block->next;
        (*list_ptr)->previous = nullptr;
    }

    block->previous = nullptr;
    block->next = nullptr;

    return block;
}

inline void TlsfAllocator::remove_block(FreeBlock *block)
{
    assert(block != nullptr);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= s_min_block_size);

    if (block->previous == nullptr)
    {
        FreeListIndex index = mapping_insert(block->tag.byte_count);
        FreeBlock *head = remove_head(index);
        assert(head == block);
    }
    else
    {
        if (block->next != nullptr)
            block->next->previous = block->previous;
        block->previous->next = block->next;
    }

    block->previous = nullptr;
    block->next = nullptr;
}

inline TlsfAllocator::FreeBlock *TlsfAllocator::split_block(
    FreeBlock *block, size_t first_byte_count)
{
    assert(block != nullptr);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= s_min_block_size);

    uintptr_t const remaining_block_addr = (uintptr_t)block + first_byte_count;
    assert(remaining_block_addr % alignof(FreeBlock) == 0);

    FreeBlock *remaining_block = (FreeBlock *)remaining_block_addr;
    remaining_block->tag.allocated = s_flag_free;
    remaining_block->tag.byte_count = block->tag.byte_count - first_byte_count;
    remaining_block->previous = nullptr;
    remaining_block->next = nullptr;
    copy_front_tag_to_back(remaining_block);

    block->tag.byte_count = first_byte_count;
    copy_front_tag_to_back(block);

    return remaining_block;
}

inline TlsfAllocator::FreeBlock *TlsfAllocator::merge_previous(FreeBlock *block)
{
    assert(block != nullptr);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= s_min_block_size);

    uintptr_t const prev_tag_addr = (uintptr_t)block - sizeof(BoundaryTag);
    assert(prev_tag_addr % alignof(BoundaryTag) == 0);

    BoundaryTag const prev_tag = *(BoundaryTag *)prev_tag_addr;

    if (prev_tag.allocated == s_flag_allocated)
        return block;

    // Merge block into previous

    uintptr_t const prev_block_addr =
        prev_tag_addr - prev_tag.byte_count + sizeof(BoundaryTag);
    assert(prev_block_addr % alignof(FreeBlock) == 0);
    assert(prev_block_addr >= (uintptr_t)m_first_block_addr);

    FreeBlock *prev_block = (FreeBlock *)prev_block_addr;
    remove_block(prev_block);

    prev_block->tag.byte_count += block->tag.byte_count;
    copy_front_tag_to_back(prev_block);

    return prev_block;
}

inline TlsfAllocator::FreeBlock *TlsfAllocator::merge_next(FreeBlock *block)
{
    assert(block != nullptr);
    assert(front_and_back_tags_match(block));
    assert(block->tag.allocated == s_flag_free);
    assert(block->tag.byte_count >= s_min_block_size);

    uintptr_t const next_tag_addr = (uintptr_t)block + block->tag.byte_count;
    assert(next_tag_addr % alignof(BoundaryTag) == 0);

    BoundaryTag const next_tag = *(BoundaryTag *)next_tag_addr;

    if (next_tag.allocated == s_flag_allocated)
        return block;

    // Merge next into block

    // Front boundary tag of a free block is the first element in its FreeBlock
    FreeBlock *next_block = (FreeBlock *)next_tag_addr;
    remove_block(next_block);

    block->tag.byte_count += next_block->tag.byte_count;
    copy_front_tag_to_back(block);

    return block;
}

} // namespace wheels

#endif // WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP
