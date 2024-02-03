#ifndef WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP

#include "allocator.hpp"

#include <cstdlib>

namespace wheels
{

// No special debug allocator since this is already compatible with valgrind
class CstdlibAllocator : public Allocator
{
  public:
    CstdlibAllocator() noexcept { }

    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept override
    {
        return std::malloc(num_bytes);
    }

    virtual void deallocate(void *ptr) noexcept override { std::free(ptr); }
};

} // namespace wheels

#endif // WHEELS_ALLOCATORS_CSTDLIB_ALLOCATOR_HPP
