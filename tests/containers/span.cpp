#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/array.hpp>
#include <wheels/containers/span.hpp>
#include <wheels/containers/static_array.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

constexpr uint32_t test_constexpr_span()
{
    uint32_t ret = 0;
    {
        uint8_t arr[5] = {0, 1, 2, 3, 4};
        Span<uint8_t> span{(uint8_t *)arr, 5};
        ret += (uint32_t)(span.data() == (uint8_t *)arr);
        ret += (uint32_t)(span.size() == 5);
        ret += (uint32_t)(!span.empty());

        Span<uint8_t> const const_span{(uint8_t *)arr, 5};
        ret += (uint32_t)(const_span.data() == (uint8_t *)arr);
        ret += (uint32_t)(const_span.size() == 5);

        Span<uint8_t> empty_span{(uint8_t *)arr, 0};
        ret += (uint32_t)(empty_span.size() == 0);
        ret += (uint32_t)(empty_span.empty());
    }

    {
        uint8_t arr[5] = {0, 1, 2, 3, 4};
        {
            Span<uint8_t> span{(uint8_t *)arr, 5};
            ret += (uint32_t)(span.data() == (uint8_t *)arr);
            ret += (uint32_t)(span.size() == 5);
            ret += (uint32_t)(!span.empty());
            for (size_t i = 0; i < span.size(); ++i)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                ret += (uint32_t)(span[i] == arr[i]);
            uint8_t sum = 0;
            for (uint8_t e : span)
                sum += e;
            ret += (uint32_t)(sum == 10);
        }

        {
            Span<uint8_t> const const_span{(uint8_t *)arr, 5};
            ret += (uint32_t)(const_span.data() == (uint8_t *)arr);
            ret += (uint32_t)(const_span.size() == 5);
            for (size_t i = 0; i < const_span.size(); ++i)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                ret += (uint32_t)(const_span[i] == arr[i]);
            uint8_t sum = 0;
            for (uint8_t e : const_span)
                sum += e;
            ret += (uint32_t)(sum == 10);
        }

        {
            Span<uint8_t> empty_span{(uint8_t *)arr, 0};
            ret += (uint32_t)(empty_span.size() == 0);
            ret += (uint32_t)(empty_span.empty());
            bool didnt_loop = true;
            for (uint8_t s : empty_span)
            {
                (void)s;
                didnt_loop = false;
            }
            ret += (uint32_t)(didnt_loop);
        }
    }

    {
        uint32_t array[] = {0, 1, 2};
        Span<uint32_t> span{(uint32_t *)array, 3};
        {
            Span<uint32_t const> const_span = span;
            (void)const_span;
        }
        {
            Span<uint32_t const> const_span{span};
            (void)const_span;
        }
    }

    return ret;
}

constexpr uint32_t test_constexpr_strspan()
{
    const char ctest[] = "test";
    uint32_t ret = 0;
    {
        StrSpan span{(char const *)ctest};

        ret += (uint32_t)(span.data() == (char const *)ctest);
        ret += (uint32_t)(span.size() == sizeof(ctest) - 1);
        ret += (uint32_t)(!span.empty());
    }

    {
        StrSpan span{(char const *)ctest, 3};
        ret += (uint32_t)(span.data() == (char const *)ctest);
        ret += (uint32_t)(span.size() == 3);
        ret += (uint32_t)(!span.empty());
    }

    return ret;
}

} // namespace

TEST_CASE("Span::create")
{
    uint8_t arr[5] = {0, 1, 2, 3, 4};
    Span<uint8_t> span{(uint8_t *)arr, 5};
    REQUIRE(span.data() == (uint8_t *)arr);
    REQUIRE(span.size() == 5);
    REQUIRE(!span.empty());

    Span<uint8_t> const const_span{(uint8_t *)arr, 5};
    REQUIRE(const_span.data() == (uint8_t *)arr);
    REQUIRE(const_span.size() == 5);

    Span<uint8_t> empty_span{(uint8_t *)arr, 0};
    REQUIRE(empty_span.size() == 0);
    REQUIRE(empty_span.empty());
}

TEST_CASE("Span::loop")
{
    uint8_t arr[5] = {0, 1, 2, 3, 4};
    {
        Span<uint8_t> span{(uint8_t *)arr, 5};
        REQUIRE(span.data() == (uint8_t *)arr);
        REQUIRE(span.size() == 5);
        REQUIRE(!span.empty());
        for (size_t i = 0; i < span.size(); ++i)
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            REQUIRE(span[i] == arr[i]);
        uint8_t sum = 0;
        for (uint8_t e : span)
            sum += e;
        REQUIRE(sum == 10);
    }

    {
        Span<uint8_t> const const_span{(uint8_t *)arr, 5};
        REQUIRE(const_span.data() == (uint8_t *)arr);
        REQUIRE(const_span.size() == 5);
        for (size_t i = 0; i < const_span.size(); ++i)
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            REQUIRE(const_span[i] == arr[i]);
        uint8_t sum = 0;
        for (uint8_t e : const_span)
            sum += e;
        REQUIRE(sum == 10);
    }

    {
        Span<uint8_t> empty_span{(uint8_t *)arr, 0};
        REQUIRE(empty_span.size() == 0);
        REQUIRE(empty_span.empty());
        bool didnt_loop = true;
        for (uint8_t s : empty_span)
        {
            (void)s;
            didnt_loop = false;
        }
        REQUIRE(didnt_loop);
    }
}

TEST_CASE("Span::const_conversion")
{
    uint32_t array[] = {0, 1, 2};
    Span<uint32_t> span{(uint32_t *)array, 3};
    {
        Span<uint32_t const> const_span = span;
        (void)const_span;
    }
    {
        Span<uint32_t const> const_span{span};
        (void)const_span;
    }
}

TEST_CASE("Span::constexpr")
{
    constexpr uint32_t ret = test_constexpr_span();
    REQUIRE(ret == 27);
}

TEST_CASE("Span::comparisons")
{
    {
        uint32_t array[] = {0, 1, 2, 3};
        uint32_t const array2[] = {0, 1, 3};

        // No inner const to make sure that works
        Span<uint32_t> const span_partial{
            (uint32_t *)array, sizeof(array) / sizeof(uint32_t) - 1};
        Span<uint32_t const> const span_full{
            (uint32_t const *)array, sizeof(array) / sizeof(uint32_t)};
        Span<uint32_t const> const span_full2{
            (uint32_t const *)array2, sizeof(array2) / sizeof(uint32_t)};
        Span<uint32_t const> const span_empty{(uint32_t *)array, 0};

        REQUIRE(span_partial == span_partial);
        REQUIRE(!(span_partial == span_full));
        REQUIRE(!(span_partial == span_full2));
        REQUIRE(!(span_full == span_partial));
        REQUIRE(!(span_empty == span_partial));
        REQUIRE(!(span_empty == span_partial));

        REQUIRE(span_full != span_partial);
        REQUIRE(span_partial != span_full);
        REQUIRE(span_partial != span_full2);
        REQUIRE(span_empty != span_partial);
        REQUIRE(span_partial != span_empty);
        REQUIRE(!(span_partial != span_partial));
    }

    {
        DtorObj const array[] = {{0}, {1}, {2}, {3}};
        DtorObj const array2[] = {{0}, {1}, {3}};

        Span<DtorObj const> const span_partial{
            (DtorObj const *)array, sizeof(array) / sizeof(DtorObj) - 1};
        Span<DtorObj const> const span_full{
            (DtorObj const *)array, sizeof(array) / sizeof(DtorObj)};
        Span<DtorObj const> const span_full2{
            (DtorObj const *)array2, sizeof(array2) / sizeof(DtorObj)};
        Span<DtorObj const> const span_empty{(DtorObj *)array, 0};

        REQUIRE(span_partial == span_partial);
        REQUIRE(!(span_partial == span_full));
        REQUIRE(!(span_partial == span_full2));
        REQUIRE(!(span_full == span_partial));
        REQUIRE(!(span_empty == span_partial));
        REQUIRE(!(span_empty == span_partial));

        REQUIRE(span_full != span_partial);
        REQUIRE(span_partial != span_full);
        REQUIRE(span_partial != span_full2);
        REQUIRE(span_empty != span_partial);
        REQUIRE(span_partial != span_empty);
        REQUIRE(!(span_partial != span_partial));
    }
}

TEST_CASE("StrSpan::create")
{
    const char ctest[] = "test";
    {
        StrSpan span{(char const *)ctest};
        REQUIRE(span.data() == (char const *)ctest);
        REQUIRE(span.size() == sizeof(ctest) - 1);
        REQUIRE(!span.empty());
    }

    {
        StrSpan span{(char const *)ctest, 3};
        REQUIRE(span.data() == (char const *)ctest);
        REQUIRE(span.size() == 3);
        REQUIRE(!span.empty());
    }
}

TEST_CASE("StrSpan::constexpr")
{
    constexpr uint32_t ret = test_constexpr_strspan();
    REQUIRE(ret == 6);
}

TEST_CASE("StrSpan::comparisons")
{
    // Behavior should be equal to c-string comparisons with the added flavor
    // that a truncating view is treated like the next character was \0.

    char cempty[] = "";
    char const ctest[] = "test";
    char const ctester[] = "tester";
    char const ctett[] = "tett";
    char const ctestnull[] = "test\0\0\0";

    StrSpan empty{(char const *)cempty, 0};
    StrSpan empty2{(char const *)cempty, 0};
    StrSpan test{(char const *)ctest, sizeof(ctest) - 1};
    StrSpan test2{(char const *)ctester, sizeof(ctester) - 3};
    StrSpan tett{(char const *)ctett, sizeof(ctett) - 1};
    StrSpan tester{(char const *)ctester, sizeof(ctester) - 1};
    StrSpan testnull{(char const *)ctestnull, sizeof(ctestnull) - 1};

    REQUIRE(empty == empty);
    REQUIRE(empty == empty2);
    REQUIRE(empty2 == empty);
    REQUIRE(!(empty == test));
    REQUIRE(!(test == empty));

    REQUIRE(test == test);
    REQUIRE(test == test2);
    REQUIRE(test2 == test2);
    REQUIRE(test2 == test2);
    REQUIRE(!(test == tett));
    REQUIRE(!(tett == test));
    REQUIRE(!(test == tester));
    REQUIRE(!(tester == test));
    REQUIRE(test == testnull);
    REQUIRE(testnull == test);

    REQUIRE(!(empty != empty));
    REQUIRE(!(empty != empty2));
    REQUIRE(!(empty2 != empty));
    REQUIRE(empty != test);
    REQUIRE(test != empty);

    REQUIRE(!(test != test));
    REQUIRE(!(test != test2));
    REQUIRE(!(test2 != test2));
    REQUIRE(!(test2 != test2));
    REQUIRE(test != tett);
    REQUIRE(tett != test);
    REQUIRE(test != tester);
    REQUIRE(tester != test);
    REQUIRE(!(test != testnull));
    REQUIRE(!(testnull != test));
}
