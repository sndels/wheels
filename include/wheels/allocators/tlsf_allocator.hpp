#ifndef WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP

#include "../assert.hpp"
#include "../containers/array.hpp"
#include "../containers/pair.hpp"
#include "../unnecessary_lock.hpp"
#include "allocator.hpp"
#include "utils.hpp"

#include <climits>
#include <cstddef>
#include <cstdlib>

#ifndef WHEELS_ALLOCATION_DEBUG

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

    // Default constructed allocator needs to be initialized with init()
    TlsfAllocator() noexcept = default;
    // Note that the allocator might not be able allocate a single block with
    // size near or matching the capacity due to how available blocks are
    // searched internally.
    TlsfAllocator(size_t capacity) noexcept;
    ~TlsfAllocator();

    TlsfAllocator(TlsfAllocator const &other) = delete;
    TlsfAllocator(TlsfAllocator &&other) = delete;
    TlsfAllocator &operator=(TlsfAllocator const &other) = delete;
    TlsfAllocator &operator=(TlsfAllocator &&other) = delete;

    void init(size_t capacity);
    // This can be called to clean up the allocator explicitly, making the dtor
    // effectively a NOP.
    void destroy();

    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept override;
    // Input ptr is invalidated if reallocation succeeds. The user needs to free
    // it after a failure.
    [[nodiscard]] void *reallocate(void *ptr, size_t num_bytes) noexcept;
    virtual void deallocate(void *ptr) noexcept override;

    Stats const &stats() const noexcept;

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

    static constexpr size_t s_flag_allocated = (size_t)true;
    static constexpr size_t s_flag_free = (size_t)false;

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
        WHEELS_ASSERT(i < 255);
        WHEELS_ASSERT(j < 255);

        return FreeListIndex{(uint8_t)i, (uint8_t)j};
    }

    [[nodiscard]] static constexpr FreeListIndex mapping_search(size_t r)
    {
        // Round up size to the next range, so that whatever block we find is
        // large enough
        r += ((size_t)1 << (fls(r) - s_J)) - 1;
        return mapping_insert(r);
    }

    static void copy_front_tag_to_back(FreeBlock *block) noexcept
    {
        WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

        uintptr_t const tag_addr =
            (uintptr_t)block + block->tag.byte_count - sizeof(BoundaryTag);
        WHEELS_ASSERT(tag_addr % sizeof(BoundaryTag) == 0);

        BoundaryTag *tag = (BoundaryTag *)tag_addr;
        *tag = block->tag;
    }

    [[nodiscard]] static bool front_and_back_tags_match(
        FreeBlock *block) noexcept
    {
        uintptr_t const tag_addr =
            (uintptr_t)block + block->tag.byte_count - sizeof(BoundaryTag);
        WHEELS_ASSERT(tag_addr % sizeof(BoundaryTag) == 0);
        BoundaryTag *tag = (BoundaryTag *)tag_addr;

        bool const flags_match = tag->allocated == block->tag.allocated;
        bool const sizes_match = tag->byte_count == block->tag.byte_count;

        return flags_match && sizes_match;
    }

    [[nodiscard]] static size_t block_padding_num_bytes(size_t num_bytes)
    {
        // Need alignment and space for the back boundary tag. Could skip
        // alignment if allocation alignment and size are nice, but let's not
        // complicate things for the 8 extra bytes.
        size_t padding = s_pre_alloc_padding + num_bytes;
        padding = aligned_offset(padding, alignof(BoundaryTag));
        padding += sizeof(BoundaryTag);
        padding -= num_bytes;

        return padding;
    }

    [[nodiscard]] static size_t padded_num_bytes(size_t num_bytes)
    {
        size_t const padding = block_padding_num_bytes(num_bytes);
        num_bytes += padding;
        if (num_bytes < s_min_block_size)
            num_bytes = s_min_block_size;

        return num_bytes;
    }

    // Returns a pointer to the head of a freelist whose head is suitable
    [[nodiscard]] FreeListIndex find_suitable_block(
        FreeListIndex start_index) noexcept;

    // Also updates the bitmaps if necessary
    void insert_block(FreeBlock *block) noexcept;
    // Also updates the bitmaps if necessary
    [[nodiscard]] FreeBlock *remove_head(FreeListIndex index) noexcept;
    // Also updates the bitmaps if necessary
    void remove_block(FreeBlock *block) noexcept;

    [[nodiscard]] FreeBlock *split_block(
        FreeBlock *block, size_t first_byte_count) noexcept;

    [[nodiscard]] FreeBlock *merge_previous(FreeBlock *block) noexcept;
    [[nodiscard]] FreeBlock *merge_next(FreeBlock *block) noexcept;

    // Separate methods that don't assert thread safety so that they can be
    // called within reallocate()
    [[nodiscard]] void *allocate_internal(size_t num_bytes) noexcept;
    void deallocate_internal(void *ptr) noexcept;

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
    mutable UnnecessaryLock m_assert_lock;
};

inline TlsfAllocator::TlsfAllocator(size_t capacity) noexcept
{
    init(capacity);
};

inline void TlsfAllocator::init(size_t capacity)
{
    // Let's assume we have a few first-level buckets, first one will be 128
    WHEELS_ASSERT(capacity >= kilobytes(2));
    WHEELS_ASSERT(m_data == nullptr && "init() already called");

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
    memset(
        m_second_level_bitmaps.begin(), 0,
        m_second_level_bitmaps.size() * sizeof(BitMapT));
    m_segregated_lists = Span{
        (SecondLevelRangesLists *)m_second_level_bitmaps.end(),
        first_level_bucket_count};
    memset(
        m_segregated_lists.begin(), 0,
        m_segregated_lists.size() * sizeof(SecondLevelRangesLists));

    uintptr_t front_tag_addr = (uintptr_t)m_segregated_lists.end();
    WHEELS_ASSERT(front_tag_addr % alignof(BoundaryTag) == 0);
    BoundaryTag *front_tag = (BoundaryTag *)front_tag_addr;
    front_tag->allocated = s_flag_allocated;
    front_tag->byte_count = 0;

    uintptr_t back_tag_addr = front_tag_addr + sizeof(BoundaryTag) + block_size;
    WHEELS_ASSERT(back_tag_addr % alignof(BoundaryTag) == 0);
    BoundaryTag *back_tag = (BoundaryTag *)back_tag_addr;
    back_tag->allocated = s_flag_allocated;
    back_tag->byte_count = 0;

    // Insert the empty block that's after the metadata
    void *first_addr = (void *)(front_tag_addr + sizeof(BoundaryTag));
    first_addr = aligned_ptr<FreeBlock>(first_addr);
    WHEELS_ASSERT((uintptr_t)first_addr - (uintptr_t)m_data <= metadata_size);
    WHEELS_ASSERT((uintptr_t)first_addr % alignof(BoundaryTag) == 0);
    m_first_block_addr = first_addr;

    FreeBlock *block = (FreeBlock *)first_addr;
    block->tag.allocated = s_flag_free;
    block->tag.byte_count = block_size;
    block->previous = nullptr;
    block->next = nullptr;
    copy_front_tag_to_back(block);

    insert_block(block);
}

inline TlsfAllocator::~TlsfAllocator() { destroy(); }

inline void TlsfAllocator::destroy()
{
    WHEELS_ASSERT(
        (m_data == nullptr || std::popcount(m_first_level_bitmap) == 1) &&
        "Expected one contiguous block remaining. Not all allocations were "
        "deallocated before the allocator was destroyed.");
    WHEELS_ASSERT(m_stats.allocation_count == 0);
    WHEELS_ASSERT(m_stats.small_allocation_count == 0);
    WHEELS_ASSERT(m_stats.allocated_byte_count == 0);

    if (m_data != nullptr)
    {
        std::free(m_data);
        m_data = nullptr;
    }
}

inline void *TlsfAllocator::allocate(size_t num_bytes) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(
        m_data != nullptr && "init() not called or destroy() already called?");

    return allocate_internal(num_bytes);
}

inline void *TlsfAllocator::allocate_internal(size_t num_bytes) noexcept
{
    size_t const internal_byte_count = padded_num_bytes(num_bytes);

    // First list that could have blocks we can use
    FreeListIndex index = mapping_search(internal_byte_count);

    // Actual first list that has blocks we can use
    index = find_suitable_block(index);
    if (index.is_null())
        // No suiltable blocks left
        return nullptr;
    WHEELS_ASSERT(m_segregated_lists[index.fl][index.sl] != nullptr);

    FreeBlock *block = remove_head(index);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= internal_byte_count);

    // Need to split and put potential extra memory back into free blocks
    if (block->tag.byte_count - internal_byte_count > s_min_block_size)
    {
        FreeBlock *remaining_block = split_block(block, internal_byte_count);
        insert_block(remaining_block);
    }

    // Actual memory should be aligned after the front tag and ptr-to-front
    void *alloc_ptr =
        (void *)((uintptr_t)block + sizeof(BoundaryTag) + sizeof(void *));
    alloc_ptr = aligned_ptr<std::max_align_t>(alloc_ptr);
    WHEELS_ASSERT(
        (uintptr_t)block + block->tag.byte_count - (uintptr_t)alloc_ptr >=
            num_bytes + sizeof(BoundaryTag) &&
        "Allocation runs over the back boundary tag");

    // Need to insert pointer to front so deallocate can find the front tag
    uintptr_t const ptr_to_front_addr = (uintptr_t)alloc_ptr - sizeof(void *);
    WHEELS_ASSERT(ptr_to_front_addr % alignof(void *) == 0);
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

inline void *TlsfAllocator::reallocate(void *ptr, size_t num_bytes) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(
        m_data != nullptr && "init() not called or destroy() already called?");

    WHEELS_ASSERT(num_bytes > 0);
    if (ptr == nullptr)
        return allocate_internal(num_bytes);

    WHEELS_ASSERT(
        (uintptr_t)ptr > (uintptr_t)m_data &&
        (uintptr_t)ptr - (uintptr_t)m_data < m_full_size);

    // We stored a pointer to the front of the block just before the allocation
    // pointer
    uintptr_t const ptr_to_front_addr = (uintptr_t)ptr - sizeof(void *);
    WHEELS_ASSERT(ptr_to_front_addr % alignof(void *) == 0);
    void *ptr_to_front = *(void **)ptr_to_front_addr;

    WHEELS_ASSERT((uintptr_t)ptr_to_front % alignof(FreeBlock) == 0);
    // This aliases the front tag that's already there
    FreeBlock *block = (FreeBlock *)ptr_to_front;
    WHEELS_ASSERT(block->tag.allocated == s_flag_allocated);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

    // Just return the same allocation if the requested size would get the same
    // size allocation
    size_t const padded_byte_count = padded_num_bytes(num_bytes);
    if (padded_byte_count == block->tag.byte_count)
        return ptr;

    // TODO:
    // Shrink the block when num_bytes is less than byte_count.

    // TODO:
    // Grow the block the if the one after it is free and the two are big enough
    // when combined.

    void *new_ptr = allocate_internal(num_bytes);
    if (new_ptr == nullptr)
        return nullptr;

    // Copy over existing data, using the smaller of the sizes.
    // Block size includes padding and tags so let's remove them to avoid
    // copying needless bytes and stomping over the end tag / next allocation's
    // front tag. This won't get us the exact data size but it's at least closer
    // than the full block size.
    // std::min() is in <algorithm>, let's not bloat this header
    size_t const smaller_size = block->tag.byte_count < padded_byte_count
                                    ? block->tag.byte_count
                                    : padded_byte_count;
    size_t const padding = block_padding_num_bytes(smaller_size);
    WHEELS_ASSERT(smaller_size > padding);
    std::memcpy(new_ptr, ptr, smaller_size - padding);

    deallocate_internal(ptr);

    return new_ptr;
}

inline void TlsfAllocator::deallocate(void *ptr) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(
        m_data != nullptr && "init() not called or destroy() already called?");

    deallocate_internal(ptr);
}

inline void TlsfAllocator::deallocate_internal(void *ptr) noexcept
{
    if (ptr == nullptr)
        return;

    WHEELS_ASSERT(
        (uintptr_t)ptr > (uintptr_t)m_data &&
        (uintptr_t)ptr - (uintptr_t)m_data < m_full_size);

    // We stored a pointer to the front of the block just before the allocation
    // pointer
    uintptr_t const ptr_to_front_addr = (uintptr_t)ptr - sizeof(void *);
    WHEELS_ASSERT(ptr_to_front_addr % alignof(void *) == 0);
    void *ptr_to_front = *(void **)ptr_to_front_addr;

    WHEELS_ASSERT((uintptr_t)ptr_to_front % alignof(FreeBlock) == 0);
    // This aliases the front tag that's already there
    FreeBlock *block = (FreeBlock *)ptr_to_front;
    WHEELS_ASSERT(block->tag.allocated == s_flag_allocated);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);
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
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);

    // Tag or end tag location might have changed
    copy_front_tag_to_back(block);

    insert_block(block);
}

inline TlsfAllocator::Stats const &TlsfAllocator::stats() const noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(
        m_data != nullptr && "init() not called or destroy() already called?");

    return m_stats;
}

inline TlsfAllocator::FreeListIndex TlsfAllocator::find_suitable_block(
    FreeListIndex start_index) noexcept
{
    WHEELS_ASSERT(start_index.sl < 64);
    BitMapT bitmap_tmp =
        m_second_level_bitmaps[start_index.fl] & ((size_t)-1 << start_index.sl);

    if (bitmap_tmp != 0)
    {
        size_t const non_empty_sl = ffs(bitmap_tmp);
        WHEELS_ASSERT(non_empty_sl < 255);
        return FreeListIndex{start_index.fl, (uint8_t)non_empty_sl};
    }

    WHEELS_ASSERT(start_index.fl < 64);
    bitmap_tmp = m_first_level_bitmap & ((size_t)-1 << (start_index.fl + 1));
    if (bitmap_tmp == 0)
        return FreeListIndex::null();

    size_t const non_empty_fl = ffs(bitmap_tmp);
    size_t const non_empty_sl = ffs(m_second_level_bitmaps[non_empty_fl]);
    WHEELS_ASSERT(non_empty_fl < 255);
    WHEELS_ASSERT(non_empty_sl < 255);

    return FreeListIndex{(uint8_t)non_empty_fl, (uint8_t)non_empty_sl};
}

inline void TlsfAllocator::insert_block(FreeBlock *block) noexcept
{
    WHEELS_ASSERT(block != nullptr);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->previous == nullptr);
    WHEELS_ASSERT(block->next == nullptr);

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

inline TlsfAllocator::FreeBlock *TlsfAllocator::remove_head(
    FreeListIndex index) noexcept
{
    FreeBlock **list_ptr = &m_segregated_lists[index.fl][index.sl];
    WHEELS_ASSERT(*list_ptr != nullptr);

    FreeBlock *block = *list_ptr;
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

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

inline void TlsfAllocator::remove_block(FreeBlock *block) noexcept
{
    WHEELS_ASSERT(block != nullptr);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

    if (block->previous == nullptr)
    {
        FreeListIndex index = mapping_insert(block->tag.byte_count);
        FreeBlock *head = remove_head(index);
        WHEELS_ASSERT(head == block);
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
    FreeBlock *block, size_t first_byte_count) noexcept
{
    WHEELS_ASSERT(block != nullptr);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

    uintptr_t const remaining_block_addr = (uintptr_t)block + first_byte_count;
    WHEELS_ASSERT(remaining_block_addr % alignof(FreeBlock) == 0);

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

inline TlsfAllocator::FreeBlock *TlsfAllocator::merge_previous(
    FreeBlock *block) noexcept
{
    WHEELS_ASSERT(block != nullptr);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

    uintptr_t const prev_tag_addr = (uintptr_t)block - sizeof(BoundaryTag);
    WHEELS_ASSERT(prev_tag_addr % alignof(BoundaryTag) == 0);

    BoundaryTag const prev_tag = *(BoundaryTag *)prev_tag_addr;

    if (prev_tag.allocated == s_flag_allocated)
        return block;

    // Merge block into previous

    uintptr_t const prev_block_addr =
        prev_tag_addr - prev_tag.byte_count + sizeof(BoundaryTag);
    WHEELS_ASSERT(prev_block_addr % alignof(FreeBlock) == 0);
    WHEELS_ASSERT(prev_block_addr >= (uintptr_t)m_first_block_addr);

    FreeBlock *prev_block = (FreeBlock *)prev_block_addr;
    remove_block(prev_block);

    prev_block->tag.byte_count += block->tag.byte_count;
    copy_front_tag_to_back(prev_block);

    return prev_block;
}

inline TlsfAllocator::FreeBlock *TlsfAllocator::merge_next(
    FreeBlock *block) noexcept
{
    WHEELS_ASSERT(block != nullptr);
    WHEELS_ASSERT(front_and_back_tags_match(block));
    WHEELS_ASSERT(block->tag.allocated == s_flag_free);
    WHEELS_ASSERT(block->tag.byte_count >= s_min_block_size);

    uintptr_t const next_tag_addr = (uintptr_t)block + block->tag.byte_count;
    WHEELS_ASSERT(next_tag_addr % alignof(BoundaryTag) == 0);

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

#else // WHEELS_ALLOCATION_DEBUG

#include <unordered_map>

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

    TlsfAllocator() noexcept = default;
    // Note that the allocator might not be able allocate a single block with
    // size near or matching the capacity due to how available blocks are
    // searched internally.
    TlsfAllocator(size_t capacity) noexcept;
    ~TlsfAllocator();

    void init(size_t capacity) noexcept;
    void destroy();

    TlsfAllocator(TlsfAllocator const &other) = delete;
    TlsfAllocator(TlsfAllocator &&other) = delete;
    TlsfAllocator &operator=(TlsfAllocator const &other) = delete;
    TlsfAllocator &operator=(TlsfAllocator &&other) = delete;

    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept override;
    // Input ptr is invalidated if reallocation succeeds. The user needs to free
    // it after a failure.
    [[nodiscard]] void *reallocate(void *ptr, size_t num_bytes) noexcept;
    virtual void deallocate(void *ptr) noexcept override;

    Stats const &stats() const noexcept;

  private:
    Stats m_stats;
    std::unordered_map<void *, size_t> m_allocations;
    mutable UnnecessaryLock m_assert_lock;
};

inline TlsfAllocator::TlsfAllocator(size_t capacity) noexcept
: m_stats{.free_byte_count = capacity}
{
}

inline TlsfAllocator::~TlsfAllocator() { destroy(); }

inline void TlsfAllocator::init(size_t capacity) noexcept
{
    m_stats.free_byte_count = capacity;
}

inline void TlsfAllocator::destroy()
{
    WHEELS_ASSERT(m_stats.allocated_byte_count == 0);
    WHEELS_ASSERT(m_allocations.empty());
    for (auto &ptr_size : m_allocations)
        std::free(ptr_size.first);
}

inline void *TlsfAllocator::allocate(size_t num_bytes) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(num_bytes <= m_stats.free_byte_count);

    void *ptr = std::malloc(num_bytes);
    WHEELS_ASSERT(ptr != nullptr);
    m_stats.allocation_count++;
    m_stats.allocated_byte_count += num_bytes;
    m_stats.free_byte_count -= num_bytes;
    m_stats.allocated_byte_count_high_watermark = std::max(
        m_stats.allocated_byte_count,
        m_stats.allocated_byte_count_high_watermark);

    m_allocations.emplace(ptr, num_bytes);

    return ptr;
}

inline void *TlsfAllocator::reallocate(void *ptr, size_t num_bytes) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(num_bytes <= m_stats.free_byte_count);

    void *new_ptr = std::realloc(ptr, num_bytes);
    WHEELS_ASSERT(new_ptr != nullptr);
    if (new_ptr != ptr)
    {
        if (ptr != nullptr)
        {
            WHEELS_ASSERT(m_allocations.contains(ptr));

            const size_t prev_num_bytes = m_allocations.find(ptr)->second;
            m_stats.allocated_byte_count -= prev_num_bytes;
            m_stats.free_byte_count += prev_num_bytes;

            m_allocations.erase(ptr);
            // realloc already freed the original pointer
        }
        else
            m_stats.allocation_count++;

        m_stats.allocated_byte_count += num_bytes;
        m_stats.free_byte_count -= num_bytes;
        m_stats.allocated_byte_count_high_watermark = std::max(
            m_stats.allocated_byte_count,
            m_stats.allocated_byte_count_high_watermark);

        m_allocations.emplace(new_ptr, num_bytes);
    }

    return new_ptr;
}

inline void TlsfAllocator::deallocate(void *ptr) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    if (ptr == nullptr)
        return;

    std::free(ptr);

    WHEELS_ASSERT(m_allocations.contains(ptr));
    const size_t num_bytes = m_allocations.find(ptr)->second;
    m_allocations.erase(ptr);

    WHEELS_ASSERT(m_stats.allocation_count > 0);
    m_stats.allocation_count--;
    WHEELS_ASSERT(m_stats.allocated_byte_count >= num_bytes);
    m_stats.allocated_byte_count -= num_bytes;
    m_stats.free_byte_count += num_bytes;
}

inline TlsfAllocator::Stats const &TlsfAllocator::stats() const noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    return m_stats;
}

} // namespace wheels

#endif // WHEELS_ALLOCATION_DEBUG

#endif // WHEELS_ALLOCATORS_TLSF_ALLOCATOR_HPP
