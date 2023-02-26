#ifndef WHEELS_LINEAR_ALLOCATOR_HPP
#define WHEELS_LINEAR_ALLOCATOR_HPP

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
    LinearAllocator(size_t capacity)
    : m_capacity{capacity}
    {
        m_memory = new uint8_t[m_capacity];
    }

    virtual ~LinearAllocator() { delete[] m_memory; };

    LinearAllocator(LinearAllocator const &) = delete;
    LinearAllocator(LinearAllocator &&) = delete;
    LinearAllocator &operator=(LinearAllocator const &) = delete;
    LinearAllocator &operator=(LinearAllocator &&) = delete;

    virtual void *allocate(size_t num_bytes) override
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

    virtual void deallocate(void * /*ptr*/) override { }

    void reset() { m_offset = 0; }

    void rewind(void *ptr)
    {
        assert(
            ptr >= m_memory && ptr < m_memory + m_capacity &&
            "Tried to rewind to a pointer that doesn't belong to this "
            "allocator");
        m_offset = (uint8_t *)ptr - m_memory;
    }

    friend class ScopedScratch;

  protected:
    void *peek() const { return m_memory; };

  private:
    uint8_t *m_memory{nullptr};
    size_t m_offset{0};
    size_t m_capacity{0};
};

} // namespace wheels

#endif // WHEELS_LINEAR_ALLOCATOR_HPP