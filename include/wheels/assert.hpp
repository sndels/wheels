#ifndef WHEELS_ASSERT_HPP
#define WHEELS_ASSERT_HPP

// Adapted from Game Engine Architecture 3rd ed. by Jason Gregory

// Enable by default, these should be cheap checks
#ifdef DISABLE_WHEELS_ASSERT

#define WHEELS_ASSERT(expr)

#else // !DISABLE_WHEELS_ASSERT

#include <cstdio>

// Annotate the report function to have clang-tidy treat the assert macros like
// proper asserts for null checks etc.
// From https://clang-analyzer.llvm.org/annotations.html#custom_assertions
#ifndef CLANG_ANALYZER_NORETURN
#if defined(__clang__)
#if __has_feature(attribute_analyzer_noreturn)
#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#else // !__has_feature(attribute_analyzer_noreturn)
#define CLANG_ANALYZER_NORETURN
#endif // __has_feature(attribute_analyzer_noreturn)
#else  // !__clang__
#define CLANG_ANALYZER_NORETURN
#endif // __clang__
#endif // CLANG_ANALYZER_NORETURN

namespace wheels
{

inline void report_assertion_failure(
    const char *expr, const char *file,
    int line) noexcept CLANG_ANALYZER_NORETURN
{
    fprintf(stderr, "Assert failed: %s\n%s:%d\n", expr, file, line);
}

} // namespace wheels

#ifdef _WIN32

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
