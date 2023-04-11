#ifndef WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP

#include "allocator.hpp"
#include "utils.hpp"

#include <cassert>
#include <climits>
#include <cstdint>

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

    friend class ScopedScratch;

  protected:
    [[nodiscard]] void *peek() const;

  private:
    uint8_t *m_memory{nullptr};
    size_t m_offset{0};
    size_t m_capacity{0};
};

inline LinearAllocator::LinearAllocator(size_t capacity)
: m_capacity{capacity}
{
    m_memory = new uint8_t[m_capacity];
}
inline LinearAllocator::~LinearAllocator() { delete[] m_memory; };

inline void *LinearAllocator::allocate(size_t num_bytes)
{
    size_t const ret_offset =
        aligned_offset(m_offset, alignof(std::max_align_t));
    assert(SIZE_MAX - num_bytes > ret_offset);

    size_t const new_offset = ret_offset + num_bytes;
    if (new_offset > m_capacity)
        return nullptr;

    m_offset = new_offset;

    return m_memory + ret_offset;
}

inline void LinearAllocator::deallocate(void * /*ptr*/) { }

inline void LinearAllocator::reset() { m_offset = 0; }

inline void LinearAllocator::rewind(void *ptr)
{
    assert(
        ptr >= m_memory && ptr < m_memory + m_capacity &&
        "Tried to rewind to a pointer that doesn't belong to this "
        "allocator");
    m_offset = (uint8_t *)ptr - m_memory;
}

inline void *LinearAllocator::peek() const { return m_memory; };

} // namespace wheels

#endif // WHEELS_ALLOCATORS_LINEAR_ALLOCATOR_HPP
