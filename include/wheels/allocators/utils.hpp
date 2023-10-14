#ifndef WHEELS_ALLOCATORS_UTILS_HPP
#define WHEELS_ALLOCATORS_UTILS_HPP

#include "../assert.hpp"

#include <bit>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>

namespace wheels
{

[[nodiscard]] constexpr size_t aligned_offset(size_t offset, size_t alignment)
{
    // We could allocate the base pointers with e.g. alignment of 256 to support
    // powers of two up to it, but let's not worry about that until it's needed.
    WHEELS_ASSERT(
        alignment <= alignof(std::max_align_t) &&
        "Alignment over std::max_align_t isn't supported.");
    WHEELS_ASSERT(SIZE_MAX - alignment > offset);
    return (offset + alignment - 1) / alignment * alignment;
}

template <typename T> [[nodiscard]] constexpr void *aligned_ptr(void *ptr)
{
    size_t const alignment = alignof(T);
    // This may already be required by spec
    static_assert(
        std::has_single_bit(alignment),
        "Implementation assumes only power of 2 alignments");

    uintptr_t uptr = (uintptr_t)ptr;
    if ((uptr & (alignment - 1)) != 0)
        uptr += alignment - (uptr & (alignment - 1));
    return (void *)uptr;
}

[[nodiscard]] constexpr size_t megabytes(size_t mb) { return mb * 1000 * 1000; }

[[nodiscard]] constexpr size_t kilobytes(size_t kb) { return kb * 1000; }

[[nodiscard]] constexpr size_t pow2(size_t c) { return (size_t)1 << c; }

} // namespace wheels

#endif // WHEELS_ALLOCATORS_UTILS_HPP
