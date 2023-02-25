#ifndef ALLOCATORS_ALLOCATOR_HPP
#define ALLOCATORS_ALLOCATOR_HPP

class Allocator
{
  public:
    virtual ~Allocator() { }
    virtual void *allocate(size_t nBytes) = 0;
    virtual void deallocate(void *ptr) = 0;
};

#endif // ALLOCATORS_ALLOCATOR_HPP