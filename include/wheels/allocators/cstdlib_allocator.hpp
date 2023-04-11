#ifndef WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP

#include "allocator.hpp"

#include <cassert>
#include <cstdlib>

namespace wheels
{

class CstdlibAllocator : public Allocator
{
  public:
    CstdlibAllocator() { }

    [[nodiscard]] virtual void *allocate(size_t num_bytes) override
    {
        return std::malloc(num_bytes);
    }

    virtual void deallocate(void *ptr) override { std::free(ptr); }
};

} // namespace wheels

#endif // WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP
