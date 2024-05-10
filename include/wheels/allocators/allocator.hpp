#ifndef WHEELS_ALLOCATORS_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_ALLOCATOR_HPP

#include <cstddef>

namespace wheels
{

class Allocator
{
  public:
    virtual ~Allocator() { }
    [[nodiscard]] virtual void *allocate(size_t num_bytes) noexcept = 0;
    // No reallocate because it would require extra bookkeeping from bump
    // allocators.
    virtual void deallocate(void *ptr) noexcept = 0;
};

} // namespace wheels

#endif // WHEELS_ALLOCATORS_ALLOCATOR_HPP
