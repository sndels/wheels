#ifndef WHEELS_ALLOCATORS_UTILS_HPP
#define WHEELS_ALLOCATORS_UTILS_HPP

#include <cassert>
#include <climits>
#include <cstddef>

namespace wheels
{

[[nodiscard]] constexpr size_t aligned_offset(size_t offset, size_t alignment)
{
    // We could allocate the base pointers with e.g. alignment of 256 to support
    // powers of two up to it, but let's not worry about that until it's needed.
    assert(
        alignment <= alignof(std::max_align_t) &&
        "Alignment over std::max_align_t isn't supported.");
    assert(SIZE_MAX - alignment > offset);
    return (offset + alignment - 1) / alignment * alignment;
}

} // namespace wheels

#endif // WHEELS_ALLOCATORS_UTILS_HPP
