#ifndef WHEELS_CSTDLIB_ALLOCATOR_HPP
#define WHEELS_CSTDLIB_ALLOCATOR_HPP

#include "allocator.hpp"

#include <cassert>
#include <cstdlib>

namespace wheels
{

class CstdlibAllocator : public Allocator
{
  public:
    CstdlibAllocator() { }
    virtual ~CstdlibAllocator()
    {
        assert(m_alloc_count == 0 && "CstdlibAllocator allocations leaked");
        m_alloc_count = 0;
    }

    virtual void *allocate(size_t num_bytes) override
    {
        m_alloc_count++;
        return std::malloc(num_bytes);
    }

    virtual void deallocate(void *ptr)
    {
        if (ptr != nullptr)
        {
            std::free(ptr);
            assert(m_alloc_count > 0);
            m_alloc_count--;
        }
    }

  private:
    size_t m_alloc_count{0};
};

} // namespace wheels

#endif // WHEELS_CSTDLIB_CstdlibAllocator_HPP