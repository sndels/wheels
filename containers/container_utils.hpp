#ifndef WHEELS_CONTAINER_UTILS_HPP
#define WHEELS_CONTAINER_UTILS_HPP

// https://www.foonathan.net/2020/09/move-forward/
// static_cast to rvalue reference
#define WHEELS_MOV(...)                                                        \
    static_cast<std::remove_reference_t<decltype(__VA_ARGS__)> &&>(__VA_ARGS__)
// The extra && aren't necessary, but make it more robust in case it's used with
// a non-reference.
#define WHEELS_FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

namespace wheels
{

size_t round_up_power_of_two(size_t value)
{
    assert(value < 0xFFFFFFFF);
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

#endif // WHEELS_CONTAINER_UTILS_HPP