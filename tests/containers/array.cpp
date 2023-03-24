#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/array.hpp>

#include "common.hpp"

using namespace wheels;

// TODO:
// Check that non trivially copyable types go through move ctor on
// reallocations and the likes that actually move the objects in memory.

namespace
{

Array<uint32_t> init_test_arr_u32(Allocator &allocator, size_t size)
{
    Array<uint32_t> arr{allocator, size};
    for (uint32_t i = 0; i < size; ++i)
        arr.push_back(10 * (i + 1));

    return arr;
}

Array<DtorObj> init_test_arr_dtor(Allocator &allocator, size_t size)
{
    Array<DtorObj> arr{allocator, size};
    for (uint32_t i = 0; i < size; ++i)
        arr.emplace_back(10 * (i + 1));

    return arr;
}

} // namespace

TEST_CASE("Array::allocate_copy")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 0};
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() > 0);

    arr.push_back(10u);
    arr.push_back(20u);
    arr.push_back(30u);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 3);

    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);

    size_t const arr_capacity = arr.capacity();
    Array<uint32_t> arr_move_constructed{WHEELS_MOV(arr)};
    REQUIRE(arr_move_constructed[0] == 10);
    REQUIRE(arr_move_constructed[1] == 20);
    REQUIRE(arr_move_constructed[2] == 30);
    REQUIRE(arr_move_constructed.size() == 3);
    REQUIRE(arr_move_constructed.capacity() == arr_capacity);

    Array<uint32_t> arr_move_assigned{allocator, 1};
    arr_move_assigned = WHEELS_MOV(arr_move_constructed);
    arr_move_assigned = WHEELS_MOV(arr_move_assigned);
    REQUIRE(arr_move_assigned[0] == 10);
    REQUIRE(arr_move_assigned[1] == 20);
    REQUIRE(arr_move_assigned[2] == 30);
    REQUIRE(arr_move_assigned.size() == 3);
    REQUIRE(arr_move_assigned.capacity() == arr_capacity);
}

TEST_CASE("Array::push_lvalue")
{
    CstdlibAllocator allocator;

    Array<DtorObj> arr{allocator, 0};
    init_dtor_counters();
    DtorObj const lvalue = {99};
    arr.push_back(lvalue);
    REQUIRE(DtorObj::s_ctor_counter() == 2);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0] == 99);
}

TEST_CASE("Array::reserve")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 1};
    arr.push_back(10u);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 1);
    REQUIRE(arr[0] == 10);
    uint32_t *initial_data = arr.data();

    arr.reserve(10);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 10);
    REQUIRE(arr[0] == 10);
    REQUIRE(initial_data != arr.data());
}

TEST_CASE("Array::front_back")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr = init_test_arr_u32(allocator, 5);
    REQUIRE(arr.front() == 10);
    REQUIRE(arr.back() == 50);

    Array<uint32_t> const &arr_const = arr;
    REQUIRE(arr_const.front() == 10);
    REQUIRE(arr_const.back() == 50);
}

TEST_CASE("Array::begin_end")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr = init_test_arr_u32(allocator, 5);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    Array<uint32_t> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("Array::clear")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
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
    REQUIRE(DtorObj::s_dtor_counter() == DtorObj::s_ctor_counter());
}

TEST_CASE("Array::emplace")
{
    class Obj
    {
      public:
        Obj(uint32_t value) { m_data = value; };

        Obj(Obj const &) = delete;
        Obj(Obj &&) = default;
        Obj &operator=(Obj const &) = delete;
        Obj &operator=(Obj &&) = default;

        uint32_t m_data{0};
    };

    CstdlibAllocator allocator;

    Array<Obj> arr{allocator, 1};
    arr.emplace_back(10u);
    arr.emplace_back(20u);
    arr.emplace_back(30u);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr[1].m_data == 20);
    REQUIRE(arr[2].m_data == 30);
    REQUIRE(arr.size() == 3);
}

TEST_CASE("Array::pop_back")
{
    class Obj
    {
      public:
        Obj(uint32_t value) { m_data = value; };

        Obj(Obj const &) = delete;
        Obj(Obj &&other)
        : m_data{other.m_data} {};
        Obj &operator=(Obj const &) = delete;

        uint32_t m_data{0};
    };

    CstdlibAllocator allocator;

    Array<Obj> arr{allocator, 1};
    arr.emplace_back(10u);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr.pop_back().m_data == 10);
    REQUIRE(arr.size() == 0);
}

TEST_CASE("Array::resize")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_value_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(6);
    REQUIRE(DtorObj::s_ctor_counter() == 11);
    REQUIRE(DtorObj::s_default_ctor_counter() == 1);
    REQUIRE(DtorObj::s_move_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter() == 11);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter() == 15);
    REQUIRE(DtorObj::s_value_ctor_counter() == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 3);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter() == 16);
    REQUIRE(DtorObj::s_value_ctor_counter() == 7);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter() == 16);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(
        DtorObj::s_dtor_counter() ==
        DtorObj::s_ctor_counter() - DtorObj::s_move_ctor_counter());
}

TEST_CASE("Array::range_for")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 1};
    // Make sure this skips
    for (auto &v : arr)
        v++;

    arr.push_back(10u);
    arr.push_back(20u);
    arr.push_back(30u);

    for (auto &v : arr)
        v++;

    REQUIRE(arr[0] == 11);
    REQUIRE(arr[1] == 21);
    REQUIRE(arr[2] == 31);

    Array<uint32_t> const &arr_const = arr;
    uint32_t sum = 0;
    for (auto const &v : arr_const)
        sum += v;
    REQUIRE(sum == 63);
}

TEST_CASE("Array::aligned")
{
    CstdlibAllocator allocator;

    Array<AlignedObj> arr{allocator, 0};

    arr.push_back(AlignedObj{10});
    arr.push_back(AlignedObj{20});

    REQUIRE((std::uintptr_t)&arr[0].value % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)&arr[1].value % alignof(AlignedObj) == 0);
    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
}