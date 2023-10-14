#ifndef WHEELS_CONTAINERS_UTILS_HPP
#define WHEELS_CONTAINERS_UTILS_HPP

#include "../assert.hpp"

namespace wheels
{

[[nodiscard]] inline size_t round_up_power_of_two(size_t value)
{
    WHEELS_ASSERT(value < 0xFFFFFFFF);
    // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
    // TODO: This would be much cleaner with clz
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    return value;
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_UTILS_HPP
