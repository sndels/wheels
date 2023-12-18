#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/inline_array.hpp>

#include "common.hpp"

using namespace wheels;

// TODO:
// Check that non trivially copyable types go through move ctor on
// reallocations and the likes that actually move the objects in memory.

namespace
{

template <size_t N>
InlineArray<uint32_t, N> init_test_static_arr_u32(size_t initial_size)
{
    REQUIRE(initial_size <= N);

    InlineArray<uint32_t, N> arr;
    for (uint32_t i = 0; i < initial_size; ++i)
        arr.push_back(10 * (i + 1));

    return arr;
}

template <size_t N>
InlineArray<DtorObj, N> init_test_static_arr_dtor(size_t initial_size)
{
    REQUIRE(initial_size <= N);

    InlineArray<DtorObj, N> arr;
    for (uint32_t i = 0; i < initial_size; ++i)
        arr.emplace_back(10 * (i + 1));

    return arr;
}

} // namespace

TEST_CASE("InlineArray::allocate_copy")
{
    InlineArray<uint32_t, 4> arr;
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 4);

    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 3);

    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);

    InlineArray<uint32_t, 4> arr_copy_constructed{arr};
    REQUIRE(arr_copy_constructed[0] == 10);
    REQUIRE(arr_copy_constructed[1] == 20);
    REQUIRE(arr_copy_constructed[2] == 30);
    REQUIRE(arr_copy_constructed.size() == 3);
    REQUIRE(arr_copy_constructed.capacity() == 4);

    InlineArray<uint32_t, 4> arr_copy_assigned;
    arr_copy_assigned = arr;
    arr_copy_assigned = WHEELS_MOV(arr_copy_assigned);
    REQUIRE(arr_copy_assigned[0] == 10);
    REQUIRE(arr_copy_assigned[1] == 20);
    REQUIRE(arr_copy_assigned[2] == 30);
    REQUIRE(arr_copy_assigned.size() == 3);
    REQUIRE(arr_copy_assigned.capacity() == 4);

    InlineArray<uint32_t, 4> arr_move_constructed{WHEELS_MOV(arr)};
    REQUIRE(arr_move_constructed[0] == 10);
    REQUIRE(arr_move_constructed[1] == 20);
    REQUIRE(arr_move_constructed[2] == 30);
    REQUIRE(arr_move_constructed.size() == 3);
    REQUIRE(arr_move_constructed.capacity() == 4);

    InlineArray<uint32_t, 4> arr_move_assigned;
    arr_move_assigned = WHEELS_MOV(arr_move_constructed);
    arr_move_assigned = WHEELS_MOV(arr_move_assigned);
    REQUIRE(arr_move_assigned[0] == 10);
    REQUIRE(arr_move_assigned[1] == 20);
    REQUIRE(arr_move_assigned[2] == 30);
    REQUIRE(arr_move_assigned.size() == 3);
    REQUIRE(arr_move_assigned.capacity() == 4);

    InlineArray<uint32_t, 4> default_arr{0xDEADCAFE};
    REQUIRE(!default_arr.empty());
    REQUIRE(default_arr.size() == 4);
    REQUIRE(default_arr.capacity() == 4);
    for (uint32_t e : default_arr)
        REQUIRE(e == 0xDEADCAFE);
}

TEST_CASE("InlineArray::push_lvalue")
{
    init_dtor_counters();

    InlineArray<DtorObj, 5> arr;
    DtorObj const lvalue = {99};
    arr.push_back(lvalue);
    REQUIRE(DtorObj::s_ctor_counter() == 2);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0] == 99);
    arr[0].data = 11;
}

TEST_CASE("InlineArray::front_back")
{
    InlineArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.front() == 10);
    REQUIRE(arr.back() == 50);

    InlineArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.front() == 10);
    REQUIRE(arr_const.back() == 50);
}

TEST_CASE("InlineArray::begin_end")
{
    InlineArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    InlineArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("InlineArray::clear")
{
    init_dtor_counters();
    InlineArray<DtorObj, 5> arr = init_test_static_arr_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_value_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    arr.clear();
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(
        DtorObj::s_dtor_counter() ==
        DtorObj::s_ctor_counter() - DtorObj::s_move_ctor_counter());
}

TEST_CASE("InlineArray::emplace")
{
    class Obj
    {
      public:
        Obj(uint32_t value)
        : m_data{value}
        {
        }
        ~Obj() = default;

        Obj(Obj const &) = delete;
        Obj(Obj &&) = delete;
        Obj &operator=(Obj const &) = delete;
        Obj &operator=(Obj &&) = delete;

        uint32_t m_data{0};
    };

    InlineArray<Obj, 3> arr;
    arr.emplace_back(10u);
    arr.emplace_back(20u);
    arr.emplace_back(30u);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr[1].m_data == 20);
    REQUIRE(arr[2].m_data == 30);
    REQUIRE(arr.size() == 3);
}

TEST_CASE("InlineArray::pop_back")
{
    class Obj
    {
      public:
        Obj(uint32_t value)
        : m_data{value}
        {
        }
        ~Obj() = default;

        Obj(Obj const &) = delete;
        Obj(Obj &&other) noexcept
        : m_data{other.m_data} {};
        Obj &operator=(Obj const &) = delete;
        Obj &operator=(Obj &&other) noexcept
        {
            if (this != &other)
            {
                m_data = other.m_data;
            }
            return *this;
        };

        uint32_t m_data{0};
    };

    InlineArray<Obj, 1> arr;
    arr.emplace_back(10u);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr.pop_back().m_data == 10);
    REQUIRE(arr.size() == 0);
}

TEST_CASE("InlineArray::resize")
{
    init_dtor_counters();

    InlineArray<DtorObj, 6> arr = init_test_static_arr_dtor<6>(5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_value_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(6);
    REQUIRE(DtorObj::s_ctor_counter() == 6);
    REQUIRE(DtorObj::s_default_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter() == 6);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter() == 10);
    REQUIRE(DtorObj::s_value_ctor_counter() == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 3);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter() == 11);
    REQUIRE(DtorObj::s_value_ctor_counter() == 7);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter() == 11);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(
        DtorObj::s_dtor_counter() ==
        DtorObj::s_ctor_counter() - DtorObj::s_move_ctor_counter());
}

TEST_CASE("InlineArray::range_for")
{
    InlineArray<uint32_t, 5> arr;
    // Make sure this skips
    for (auto &v : arr)
        v++;

    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);

    for (auto &v : arr)
        v++;

    REQUIRE(arr[0] == 11);
    REQUIRE(arr[1] == 21);
    REQUIRE(arr[2] == 31);

    InlineArray<uint32_t, 5> const &arr_const = arr;
    uint32_t sum = 0;
    for (auto const &v : arr_const)
        sum += v;
    REQUIRE(sum == 63);
}

TEST_CASE("InlineArray::aligned")
{
    InlineArray<AlignedObj, 2> arr;

    arr.push_back({10});
    arr.push_back({20});

    REQUIRE((std::uintptr_t)&arr[0].value % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)&arr[1].value % alignof(AlignedObj) == 0);
    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
}

TEST_CASE("InlineArray::span_conversions")
{
    {
        InlineArray<uint8_t, 32> arr;
        for (uint8_t i = 0; i < 10; ++i)
            arr.push_back(i);

        {
            Span<uint8_t> span = arr;
            REQUIRE(span.data() == arr.data());
            REQUIRE(span.size() == arr.size());

            Span<uint8_t const> const_span = span;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
        {
            InlineArray<uint8_t, 32> const &const_arr = arr;
            Span<uint8_t const> const_span = const_arr;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
    }
    {
        InlineArray<DtorObj, 32> arr;
        for (uint8_t i = 0; i < 10; ++i)
            arr.emplace_back(i);

        {
            Span<DtorObj> span = arr;
            REQUIRE(span.data() == arr.data());
            REQUIRE(span.size() == arr.size());

            Span<DtorObj const> const_span = span;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }

        {
            InlineArray<DtorObj, 32> const &const_arr = arr;
            Span<DtorObj const> const_span = const_arr;
            REQUIRE(const_span.data() == arr.data());
            REQUIRE(const_span.size() == arr.size());
        }
    }
}

TEST_CASE("InlineArray::ctor_deduction")
{
    {
        InlineArray arr{1u, 2u, 3u};
        REQUIRE(arr.size() == 3);
        REQUIRE(arr.capacity() == 3);
    }

    {
        InlineArray arr{{1u, 2u, 3u}};
        REQUIRE(arr.size() == 3);
        REQUIRE(arr.capacity() == 3);
    }
}
