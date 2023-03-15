#ifndef WHEELS_TESTS_ALLOCATORS_COMMON_HPP
#define WHEELS_TESTS_ALLOCATORS_COMMON_HPP

#include <cstddef>
#include <cstdint>

struct alignas(std::max_align_t) AlignedObj
{
    uint32_t value{0};
    uint8_t _padding[alignof(std::max_align_t) - sizeof(uint32_t)];
};
static_assert(alignof(AlignedObj) > alignof(uint32_t));

#endif // WHEELS_TESTS_ALLOCATORS_COMMON_HPP