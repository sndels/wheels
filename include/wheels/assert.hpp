#ifndef WHEELS_ASSERT_HPP
#define WHEELS_ASSERT_HPP

// Adapted from Game Engine Architecture 3rd ed. by Jason Gregory

// Enable by default, these should be cheap checks
#ifdef DISABLE_WHEELS_ASSERT

#define WHEELS_ASSERT(expr)

#else // !DISABLE_WHEELS_ASSERT

#include "cstdio"

namespace wheels
{

inline void report_assertion_failure(
    const char *expr, const char *file, int line)
{
    fprintf(stderr, "Assert failed: %s\n%s:%d\n", expr, file, line);
}

} // namespace wheels

#ifdef _WIN32

// for __debug_break()
#include <intrin.h>

// Assumes MSVC
#define WHEELS_ASSERT(expr)                                                    \
    do                                                                         \
    {                                                                          \
        if (expr) [[likely]]                                                   \
        {                                                                      \
        }                                                                      \
        else [[unlikely]]                                                      \
        {                                                                      \
            wheels::report_assertion_failure(#expr, __FILE__, __LINE__);       \
            __debugbreak();                                                    \
        }                                                                      \
    } while (false)

#else // !__WIN32

// Assumes gcc or a new enough clang
#define WHEELS_ASSERT(expr)                                                    \
    do                                                                         \
    {                                                                          \
        if (expr) [[likely]]                                                   \
        {                                                                      \
        }                                                                      \
        else [[unlikely]]                                                      \
        {                                                                      \
            wheels::report_assertion_failure(#expr, __FILE__, __LINE__);       \
            __builtin_trap();                                                  \
        }                                                                      \
    } while (false)

#endif // __WIN32

#endif // DISABLE_WHEELS_ASSERT

#endif // WHEELS_ASSERT_HPP
