#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/array.hpp>
#include <wheels/containers/span.hpp>
#include <wheels/containers/static_array.hpp>

#include "common.hpp"

using namespace wheels;

TEST_CASE("Span::create")
{
    uint8_t arr[5] = {0, 1, 2, 3, 4};
    Span<uint8_t> span{arr, 5};
    REQUIRE(span.data() == arr);
    REQUIRE(span.size() == 5);
    REQUIRE(!span.empty());

    Span<uint8_t> const const_span{arr, 5};
    REQUIRE(const_span.data() == arr);
    REQUIRE(const_span.size() == 5);

    Span<uint8_t> empty_span{arr, 0};
    REQUIRE(empty_span.size() == 0);
    REQUIRE(empty_span.empty());
}

TEST_CASE("Span::loop")
{
    uint8_t arr[5] = {0, 1, 2, 3, 4};
    {
        Span<uint8_t> span{arr, 5};
        REQUIRE(span.data() == arr);
        REQUIRE(span.size() == 5);
        REQUIRE(!span.empty());
        for (size_t i = 0; i < span.size(); ++i)
            REQUIRE(span[i] == arr[i]);
        uint8_t sum = 0;
        for (uint8_t e : span)
            sum += e;
        REQUIRE(sum == 10);
    }

    {
        Span<uint8_t> const const_span{arr, 5};
        REQUIRE(const_span.data() == arr);
        REQUIRE(const_span.size() == 5);
        for (size_t i = 0; i < const_span.size(); ++i)
            REQUIRE(const_span[i] == arr[i]);
        uint8_t sum = 0;
        for (uint8_t e : const_span)
            sum += e;
        REQUIRE(sum == 10);
    }

    {
        Span<uint8_t> empty_span{arr, 0};
        REQUIRE(empty_span.size() == 0);
        REQUIRE(empty_span.empty());
        bool didnt_loop = true;
        for (uint8_t e : empty_span)
            didnt_loop = false;
        REQUIRE(didnt_loop);
    }
}
TEST_CASE("Span::const_conversion")
{
    uint32_t array[] = {0, 1, 2};
    Span<uint32_t> span{array, 3};
    {
        Span<uint32_t const> const_span = span;
        (void)const_span;
    }
    {
        Span<uint32_t const> const_span{span};
        (void)const_span;
    }
}

TEST_CASE("Span::comparisons")
{
    {
        uint32_t array[] = {0, 1, 2, 3};
        uint32_t const array2[] = {0, 1, 3};

        // No inner const to make sure that works
        Span<uint32_t> const span_partial{
            array, sizeof(array) / sizeof(uint32_t) - 1};
        Span<uint32_t const> const span_full{
            array, sizeof(array) / sizeof(uint32_t)};
        Span<uint32_t const> const span_full2{
            array2, sizeof(array2) / sizeof(uint32_t)};
        Span<uint32_t const> const span_empty{array, 0};

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
            array, sizeof(array) / sizeof(DtorObj) - 1};
        Span<DtorObj const> const span_full{
            array, sizeof(array) / sizeof(DtorObj)};
        Span<DtorObj const> const span_full2{
            array2, sizeof(array2) / sizeof(DtorObj)};
        Span<DtorObj const> const span_empty{array, 0};

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

TEST_CASE("Span::compare_str")
{
    // Behavior should be equal to c-string comparisons with the added flavor
    // that a truncating view is treated like the next character was \0.

    char cempty[] = "";
    char const ctest[] = "test";
    char const ctester[] = "tester";
    char const ctett[] = "tett";
    char const ctestnull[] = "test\0\0\0";

    Span<char> empty{cempty, 0};
    Span<char const> empty2{cempty, 0};
    Span<char const> test{ctest, sizeof(ctest) - 1};
    Span<char const> test2{ctester, sizeof(ctester) - 3};
    Span<char const> tett{ctett, sizeof(ctett) - 1};
    Span<char const> tester{ctester, sizeof(ctester) - 1};
    Span<char const> testnull{ctestnull, sizeof(ctestnull) - 1};

    REQUIRE(compare_str(empty, empty));
    REQUIRE(compare_str(empty, empty2));
    REQUIRE(compare_str(empty2, empty));
    REQUIRE(!compare_str(empty, test));
    REQUIRE(!compare_str(test, empty));

    REQUIRE(compare_str(test, test));
    REQUIRE(compare_str(test, test2));
    REQUIRE(compare_str(test2, test2));
    REQUIRE(compare_str(test2, test2));
    REQUIRE(!compare_str(test, tett));
    REQUIRE(!compare_str(tett, test));
    REQUIRE(!compare_str(test, tester));
    REQUIRE(!compare_str(tester, test));
    REQUIRE(compare_str(test, testnull));
    REQUIRE(compare_str(testnull, test));
}
