#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/span.hpp>

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
