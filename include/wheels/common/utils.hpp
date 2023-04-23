
#ifndef WHEELS_COMMON_UTILS_HPP
#define WHEELS_COMMON_UTILS_HPP

// https://www.foonathan.net/2020/09/move-forward/
// static_cast to rvalue reference
#define WHEELS_MOV(...)                                                        \
    static_cast<std::remove_reference_t<decltype(__VA_ARGS__)> &&>(__VA_ARGS__)
// The extra && aren't necessary, but make it more robust in case it's used with
// a non-reference.
#define WHEELS_FWD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

#endif // WHEELS_COMMON_UTILS_HPP
