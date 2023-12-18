#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/static_array.hpp>

#include "common.hpp"

using namespace wheels;

// TODO:
// Check that non trivially copyable types go through move ctor on
// reallocations and the likes that actually move the objects in memory.

namespace
{

constexpr uint32_t test_constexpr_u32()
{
    StaticArray<uint32_t, 4> arr{{1, 2, 3, 4}};

    uint32_t sum = 0;

    sum += arr[0];
    sum += arr[1];
    sum += arr[2];
    sum += arr[3];

    StaticArray<uint32_t, 4> arr_copy_constructed{arr};
    sum += arr_copy_constructed[0];
    sum += arr_copy_constructed[1];
    sum += arr_copy_constructed[2];
    sum += arr_copy_constructed[3];

    StaticArray<uint32_t, 4> arr_copy_assigned;
    arr_copy_assigned = arr;
    sum += arr_copy_assigned[0];
    sum += arr_copy_assigned[1];
    sum += arr_copy_assigned[2];
    sum += arr_copy_assigned[3];

    StaticArray<uint32_t, 4> arr_move_constructed{WHEELS_MOV(arr)};
    sum += arr_move_constructed[0];
    sum += arr_move_constructed[1];
    sum += arr_move_constructed[2];
    sum += arr_move_constructed[3];

    StaticArray<uint32_t, 4> arr_move_assigned;
    arr_move_assigned = WHEELS_MOV(arr_move_constructed);
    sum += arr_move_assigned[0];
    sum += arr_move_assigned[1];
    sum += arr_move_assigned[2];
    sum += arr_move_assigned[3];

    StaticArray<uint32_t, 4> default_arr{10};
    sum += default_arr[0];
    sum += default_arr[1];
    sum += default_arr[2];
    sum += default_arr[3];

    const StaticArray<uint32_t, 4> const_default_arr{10};
    sum += const_default_arr.size();
    sum += const_default_arr.capacity();
    sum += const_default_arr.data()[0];
    sum += const_default_arr.begin()[0];
    sum += (const_default_arr.end() - 1)[0];

    return sum;
}

} // namespace

TEST_CASE("StaticArray::allocate_copy")
{
    StaticArray<uint32_t, 4> arr;
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 4);

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);

    StaticArray<uint32_t, 4> arr_copy_constructed{arr};
    REQUIRE(arr_copy_constructed[0] == 10);
    REQUIRE(arr_copy_constructed[1] == 20);
    REQUIRE(arr_copy_constructed[2] == 30);
    REQUIRE(arr_copy_constructed.size() == 4);
    REQUIRE(arr_copy_constructed.capacity() == 4);

    StaticArray<uint32_t, 4> arr_copy_assigned;
    arr_copy_assigned = arr;
    arr_copy_assigned = WHEELS_MOV(arr_copy_assigned);
    REQUIRE(arr_copy_assigned[0] == 10);
    REQUIRE(arr_copy_assigned[1] == 20);
    REQUIRE(arr_copy_assigned[2] == 30);
    REQUIRE(arr_copy_assigned.size() == 4);
    REQUIRE(arr_copy_assigned.capacity() == 4);

    StaticArray<uint32_t, 4> arr_move_constructed{WHEELS_MOV(arr)};
    REQUIRE(arr_move_constructed[0] == 10);
    REQUIRE(arr_move_constructed[1] == 20);
    REQUIRE(arr_move_constructed[2] == 30);
    REQUIRE(arr_move_constructed.size() == 4);
    REQUIRE(arr_move_constructed.capacity() == 4);

    StaticArray<uint32_t, 4> arr_move_assigned;
    arr_move_assigned = WHEELS_MOV(arr_move_constructed);
    arr_move_assigned = WHEELS_MOV(arr_move_assigned);
    REQUIRE(arr_move_assigned[0] == 10);
    REQUIRE(arr_move_assigned[1] == 20);
    REQUIRE(arr_move_assigned[2] == 30);
    REQUIRE(arr_move_assigned.size() == 4);
    REQUIRE(arr_move_assigned.capacity() == 4);

    StaticArray<uint32_t, 4> default_arr{0xDEADCAFE};
    REQUIRE(default_arr.size() == 4);
    REQUIRE(default_arr.capacity() == 4);
    for (uint32_t e : default_arr)
        REQUIRE(e == 0xDEADCAFE);
}

TEST_CASE("StaticArray::begin_end")
{
    StaticArray<uint32_t, 5> arr;
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    StaticArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("StaticArray::range_for")
{
    StaticArray<uint32_t, 3> arr;
    // Make sure this skips
    for (auto &v : arr)
        v++;

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    for (auto &v : arr)
        v++;

    REQUIRE(arr[0] == 11);
    REQUIRE(arr[1] == 21);
    REQUIRE(arr[2] == 31);

    StaticArray<uint32_t, 3> const &arr_const = arr;
    uint32_t sum = 0;
    for (auto const &v : arr_const)
        sum += v;
    REQUIRE(sum == 63);
}

TEST_CASE("StaticArray::aligned")
{
    StaticArray<AlignedObj, 2> arr;

    arr[0] = {10};
    arr[1] = {20};

    REQUIRE((std::uintptr_t)&arr[0].value % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)&arr[1].value % alignof(AlignedObj) == 0);
    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
}

TEST_CASE("StaticArray::span_conversions")
{
    {
        StaticArray<uint8_t, 32> arr;
        {
            Span<uint8_t> span = arr;
            REQUIRE(span.data() == arr.data());
            REQUIRE(span.size() == arr.size());

            Span<uint8_t const> const_span = span;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
        {
            StaticArray<uint8_t, 32> const &const_arr = arr;
            Span<uint8_t const> const_span = const_arr;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
    }
    {
        StaticArray<DtorObj, 32> arr;
        {
            Span<DtorObj> span = arr;
            REQUIRE(span.data() == arr.data());
            REQUIRE(span.size() == arr.size());

            Span<DtorObj const> const_span = span;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }

        {
            StaticArray<DtorObj, 32> const &const_arr = arr;
            Span<DtorObj const> const_span = const_arr;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
    }
}

TEST_CASE("StaticArray::ctor_deduction")
{
    {
        StaticArray arr{{1u, 2u, 3u}};
        REQUIRE(arr.size() == 3);
        REQUIRE(arr.capacity() == 3);
    }
}

TEST_CASE("StaticArray::constexpr")
{
    constexpr uint32_t v = test_constexpr_u32();
    REQUIRE(v == 128);
}
