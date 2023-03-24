#ifndef WHEELS_ALLOCATORS_ALLOCATOR_HPP
#define WHEELS_ALLOCATORS_ALLOCATOR_HPP

namespace wheels
{

class Allocator
{
  public:
    virtual ~Allocator() { }
    virtual void *allocate(size_t num_bytes) = 0;
    virtual void deallocate(void *ptr) = 0;
};

} // namespace wheels

#endif // WHEELS_ALLOCATORS_ALLOCATOR_HPP
