#ifndef WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP

#include "../assert.hpp"
#include "../unnecessary_lock.hpp"
#include "allocator.hpp"
#include "utils.hpp"

#include <climits>
#include <cstdint>

#ifndef WHEELS_ALLOCATION_DEBUG

namespace wheels
{

class ScopedScratch;

class LinearAllocator : public Allocator
{
  public:
    LinearAllocator(size_t capacity);

    virtual ~LinearAllocator();

    LinearAllocator(LinearAllocator const &) = delete;
    LinearAllocator(LinearAllocator &&) = delete;
    LinearAllocator &operator=(LinearAllocator const &) = delete;
    LinearAllocator &operator=(LinearAllocator &&) = delete;

    // User should not depend on the addresses themselves being linear to
    // support the debug version of this allocator.
    [[nodiscard]] virtual void *allocate(size_t num_bytes) override;

    virtual void deallocate(void *ptr) override;

    void reset();

    void rewind(void *ptr);

    size_t allocated_byte_count_high_watermark() const;

    friend class ScopedScratch;

  protected:
    // User should not depend on the allocated addresses themselves being linear
    // after this pointer to support the debug version of this allocator.
    [[nodiscard]] void *peek() const;

  private:
    uint8_t *m_memory{nullptr};
    size_t m_offset{0};
    size_t m_capacity{0};
    size_t m_allocated_byte_count_high_watermark{0};
    mutable UnnecessaryLock m_assert_lock;
};

inline LinearAllocator::LinearAllocator(size_t capacity)
: m_capacity{capacity}
{
    m_memory = new uint8_t[m_capacity];
}
inline LinearAllocator::~LinearAllocator() { delete[] m_memory; };

inline void *LinearAllocator::allocate(size_t num_bytes)
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    size_t const ret_offset =
        aligned_offset(m_offset, alignof(std::max_align_t));
    WHEELS_ASSERT(SIZE_MAX - num_bytes > ret_offset);

    size_t const new_offset = ret_offset + num_bytes;
    if (new_offset > m_capacity)
        return nullptr;

    m_offset = new_offset;
    if (m_offset > m_allocated_byte_count_high_watermark)
        m_allocated_byte_count_high_watermark = m_offset;

    return m_memory + ret_offset;
}

inline void LinearAllocator::deallocate(void * /*ptr*/)
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
}

inline void LinearAllocator::reset()
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    m_offset = 0;
}

inline void LinearAllocator::rewind(void *ptr)
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    WHEELS_ASSERT(
        ptr >= m_memory && ptr < m_memory + m_capacity &&
        "Tried to rewind to a pointer that doesn't belong to this "
        "allocator");
    m_offset = (uint8_t *)ptr - m_memory;
}

inline size_t LinearAllocator::allocated_byte_count_high_watermark() const
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    return m_allocated_byte_count_high_watermark;
}

inline void *LinearAllocator::peek() const
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);

    return m_memory + m_offset;
};

} // namespace wheels

#else // !WHEELS_ALLOCATION_DEBUG

#include <vector>

namespace wheels
{

class ScopedScratch;

class LinearAllocator : public Allocator
{
  public:
    LinearAllocator(size_t capacity);

    virtual ~LinearAllocator();

    LinearAllocator(LinearAllocator const &) = delete;
    LinearAllocator(LinearAllocator &&) = delete;
    LinearAllocator &operator=(LinearAllocator const &) = delete;
    LinearAllocator &operator=(LinearAllocator &&) = delete;

    [[nodiscard]] virtual void *allocate(size_t num_bytes) override;

    virtual void deallocate(void *ptr) override;

    void reset();

    void rewind(void *ptr);

    size_t allocated_byte_count_high_watermark() const;

    friend class ScopedScratch;

  protected:
    [[nodiscard]] void *peek() const;

  private:
    mutable std::vector<void *> m_allocations;
    mutable std::vector<size_t> m_allocation_byte_counts;
    size_t m_allocated_byte_count{0};
    size_t m_capacity{0};
    size_t m_allocated_byte_count_high_watermark{0};
};

inline LinearAllocator::LinearAllocator(size_t capacity)
: m_capacity{capacity}
{
}

inline LinearAllocator::~LinearAllocator()
{
    // Dropping a full linear allocator is ok by design
    for (void *ptr : m_allocations)
        std::free(ptr);
};

inline void *LinearAllocator::allocate(size_t num_bytes)
{
    if (m_capacity - num_bytes < m_allocated_byte_count)
        return nullptr;

    void *ptr = std::malloc(num_bytes);
    WHEELS_ASSERT(ptr != nullptr);

    m_allocated_byte_count += num_bytes;
    m_allocated_byte_count_high_watermark =
        std::max(m_allocated_byte_count, m_allocated_byte_count_high_watermark);
    m_allocations.push_back(ptr);
    m_allocation_byte_counts.push_back(num_bytes);

    return ptr;
}

inline void LinearAllocator::deallocate(void * /*ptr*/)
{ // Match nop from actual implementation
}

inline void LinearAllocator::reset()
{
    m_allocated_byte_count = 0;
    for (void *ptr : m_allocations)
        std::free(ptr);
    m_allocations.clear();
    m_allocation_byte_counts.clear();
}

inline void LinearAllocator::rewind(void *ptr)
{
    if (ptr == nullptr)
        return;

    size_t target_i = 0;
    for (void *pptr : m_allocations)
    {
        if (ptr == pptr)
            break;
        target_i++;
    }
    WHEELS_ASSERT(
        target_i < m_allocations.size() &&
        "ptr is not an active allocation in this allocator");

    const size_t allocation_count = m_allocations.size();
    for (size_t i = target_i; i < allocation_count; ++i)
    {
        std::free(m_allocations[i]);
        m_allocated_byte_count -= m_allocation_byte_counts[i];
    }
    m_allocations.resize(target_i);
    m_allocation_byte_counts.resize(target_i);
}

inline size_t LinearAllocator::allocated_byte_count_high_watermark() const
{
    return m_allocated_byte_count_high_watermark;
}

inline void *LinearAllocator::peek() const
{
    // Push a marker into tracked allocations to mimic rewind behavior
    void *marker_ptr = std::malloc(1);
    m_allocations.push_back(marker_ptr);
    m_allocation_byte_counts.push_back(0);

    return marker_ptr;
};

} // namespace wheels

#endif // WHEELS_ALLOCATION_DEBUG

#endif // WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP
