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
    // Default constructed allocator needs to be initialized with init()
    LinearAllocator() noexcept = default;
    LinearAllocator(size_t capacity) noexcept;
    // alloc has to live at least as long as this allocator
    LinearAllocator(Allocator &allocactor, size_t capacity) noexcept;

    virtual ~LinearAllocator();

    void init(size_t capacity);
    // alloc has to live at least as long as this allocator (or until destroy()
    // is called)
    void init(Allocator &allocactor, size_t capacity) noexcept;
    void destroy();

    LinearAllocator(LinearAllocator const &) = delete;
    LinearAllocator(LinearAllocator &&) = delete;
    LinearAllocator &operator=(LinearAllocator const &) = delete;
    LinearAllocator &operator=(LinearAllocator &&) = delete;

    // User should not depend on the addresses themselves being linear to
    // support the debug version of this allocator.
    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept override;

    virtual void deallocate(void *ptr) noexcept override;

    void reset() noexcept;

    void rewind(void *ptr) noexcept;

    size_t allocated_byte_count_high_watermark() const noexcept;

    friend class ScopedScratch;

  protected:
    // User should not depend on the allocated addresses themselves being linear
    // after this pointer to support the debug version of this allocator.
    [[nodiscard]] void *peek() const noexcept;

  private:
    Allocator *m_allocator{nullptr};
    uint8_t *m_memory{nullptr};
    size_t m_offset{0};
    size_t m_capacity{0};
    size_t m_allocated_byte_count_high_watermark{0};
    mutable UnnecessaryLock m_assert_lock;
};

inline LinearAllocator::LinearAllocator(size_t capacity) noexcept
{
    init(capacity);
}

inline LinearAllocator::LinearAllocator(
    Allocator &allocator, size_t capacity) noexcept
{
    init(allocator, capacity);
}

inline LinearAllocator::~LinearAllocator() { destroy(); }

inline void LinearAllocator::init(size_t capacity)
{
    WHEELS_ASSERT(m_allocator == nullptr && "init() already called");
    WHEELS_ASSERT(m_memory == nullptr && "init() already called");

    m_capacity = capacity;
    m_memory = new uint8_t[m_capacity];
}

inline void LinearAllocator::init(
    Allocator &allocator, size_t capacity) noexcept
{
    WHEELS_ASSERT(m_allocator == nullptr && "init() already called");
    WHEELS_ASSERT(m_memory == nullptr && "init() already called");

    m_allocator = &allocator;
    m_capacity = capacity;
    m_memory = reinterpret_cast<uint8_t *>(m_allocator->allocate(m_capacity));
}

inline void LinearAllocator::destroy()
{
    if (m_memory != nullptr)
    {
        if (m_allocator != nullptr)
        {
            m_allocator->deallocate(m_memory);
            m_allocator = nullptr;
        }
        else
            delete[] m_memory;

        m_memory = nullptr;
    }
};

inline void *LinearAllocator::allocate(size_t num_bytes) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");

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

inline void LinearAllocator::deallocate(void * /*ptr*/) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");
}

inline void LinearAllocator::reset() noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");

    m_offset = 0;
}

inline void LinearAllocator::rewind(void *ptr) noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");

    WHEELS_ASSERT(
        ptr >= m_memory && ptr < m_memory + m_capacity &&
        "Tried to rewind to a pointer that doesn't belong to this "
        "allocator");
    m_offset = (uint8_t *)ptr - m_memory;
}

inline size_t LinearAllocator::allocated_byte_count_high_watermark()
    const noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");

    return m_allocated_byte_count_high_watermark;
}

inline void *LinearAllocator::peek() const noexcept
{
    WHEELS_ASSERT_LOCK_NOT_NECESSARY(m_assert_lock);
    WHEELS_ASSERT(m_memory != nullptr && "init() not called");

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
    LinearAllocator() noexcept = default;
    LinearAllocator(size_t capacity) noexcept;
    LinearAllocator(Allocator &allocactor, size_t capacity) noexcept;

    virtual ~LinearAllocator();

    void init(size_t capacity);
    void init(Allocator &allocactor, size_t capacity) noexcept;
    void destroy();

    LinearAllocator(LinearAllocator const &) = delete;
    LinearAllocator(LinearAllocator &&) = delete;
    LinearAllocator &operator=(LinearAllocator const &) = delete;
    LinearAllocator &operator=(LinearAllocator &&) = delete;

    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept override;

    virtual void deallocate(void *ptr) noexcept override;

    void reset() noexcept;

    void rewind(void *ptr) noexcept;

    size_t allocated_byte_count_high_watermark() const noexcept;

    friend class ScopedScratch;

  protected:
    [[nodiscard]] void *peek() const noexcept;

  private:
    mutable std::vector<void *> m_allocations;
    mutable std::vector<size_t> m_allocation_byte_counts;
    size_t m_allocated_byte_count{0};
    size_t m_capacity{0};
    size_t m_allocated_byte_count_high_watermark{0};
};

inline LinearAllocator::LinearAllocator(size_t capacity) noexcept
: m_capacity{capacity}
{
}

inline LinearAllocator::LinearAllocator(
    Allocator & /*alloc*/, size_t capacity) noexcept
: m_capacity{capacity}
{
}

inline LinearAllocator::~LinearAllocator()
{
    // Dropping a full linear allocator is ok by design
    for (void *ptr : m_allocations)
        std::free(ptr);
};

inline void *LinearAllocator::allocate(size_t num_bytes) noexcept
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

inline void LinearAllocator::deallocate(void * /*ptr*/) noexcept
{ // Match nop from actual implementation
}

inline void LinearAllocator::reset() noexcept
{
    m_allocated_byte_count = 0;
    for (void *ptr : m_allocations)
        std::free(ptr);
    m_allocations.clear();
    m_allocation_byte_counts.clear();
}

inline void LinearAllocator::rewind(void *ptr) noexcept
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

inline size_t LinearAllocator::allocated_byte_count_high_watermark()
    const noexcept
{
    return m_allocated_byte_count_high_watermark;
}

inline void *LinearAllocator::peek() const noexcept
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
