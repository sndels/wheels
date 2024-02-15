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
    { // Initial capacity should be exact
        size_t const cap = 2;
        Array<uint32_t> arr{allocator, cap};
        REQUIRE(arr.empty());
        REQUIRE(arr.size() == 0);
        REQUIRE(arr.capacity() == cap);
        REQUIRE(arr.data() != nullptr);
    }

    Array<uint32_t> arr{allocator};
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 0);
    REQUIRE(arr.data() == nullptr);

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

    Array<uint32_t> empty_arr{allocator};
    REQUIRE(empty_arr.begin() == empty_arr.data());
    REQUIRE(empty_arr.end() == empty_arr.begin());

    Array<uint32_t> arr = init_test_arr_u32(allocator, 5);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    Array<uint32_t> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("Array::span")
{
    CstdlibAllocator allocator;

    Array<uint8_t> arr{allocator, 32};
    for (uint8_t i = 0; i < 10; ++i)
        arr.push_back(i);

    Span<uint8_t> span = arr.span(3, 6);
    REQUIRE(span.data() == arr.data() + 3);
    REQUIRE(span.size() == 3);
    Span<uint8_t> full_span = arr.span();
    REQUIRE(full_span.data() == arr.data());
    REQUIRE(full_span.size() == arr.size());

    Array<uint8_t> const &const_arr = arr;
    Span<uint8_t const> const_span = const_arr.span(3, 6);
    REQUIRE(const_span.data() == arr.data() + 3);
    REQUIRE(const_span.size() == 3);
    Span<uint8_t const> const_full_span = const_arr.span();
    REQUIRE(const_full_span.data() == const_arr.data());
    REQUIRE(const_full_span.size() == const_arr.size());
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

    Array<uint32_t> empty_arr{allocator};
    empty_arr.clear();
    REQUIRE(empty_arr.empty());
    REQUIRE(empty_arr.size() == 0);
    REQUIRE(empty_arr.capacity() == 0);
}

TEST_CASE("Array::emplace")
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

TEST_CASE("Array::extend")
{
    CstdlibAllocator allocator;

    {
        Array<uint32_t> arr{allocator, 0};
        uint32_t const lvalue[3] = {21, 22, 23};
        arr.extend(Span{&lvalue[0], 3});
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 21);
        REQUIRE(arr[1] == 22);
        REQUIRE(arr[2] == 23);
    }

    {
        Array<DtorObj> arr{allocator, 0};
        init_dtor_counters();
        DtorObj const lvalue[3] = {{11}, {12}, {13}};
        arr.extend(Span{&lvalue[0], 3});
        REQUIRE(DtorObj::s_ctor_counter() == 6);
        REQUIRE(DtorObj::s_value_ctor_counter() == 3);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 3);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 11);
        REQUIRE(arr[1] == 12);
        REQUIRE(arr[2] == 13);
    }
}

TEST_CASE("Array::pop_back")
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
        Obj(Obj &&) = default;
        Obj &operator=(Obj const &) = delete;
        Obj &operator=(Obj &&) = default;

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

TEST_CASE("Array::erase")
{
    {
        CstdlibAllocator allocator;

        Array<uint32_t> arr{allocator, 1};
        arr.emplace_back(10u);
        arr.emplace_back(20u);
        arr.emplace_back(30u);
        arr.emplace_back(40u);
        REQUIRE(arr.size() == 4);
        arr.erase(1);
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 10u);
        REQUIRE(arr[1] == 30u);
        REQUIRE(arr[2] == 40u);
    }
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
            Obj(Obj &&other) = default;
            Obj &operator=(Obj const &) = delete;
            Obj &operator=(Obj &&other) = default;

            uint32_t m_data{0};
        };

        CstdlibAllocator allocator;

        Array<Obj> arr{allocator, 1};
        arr.emplace_back(10u);
        arr.emplace_back(20u);
        arr.emplace_back(30u);
        arr.emplace_back(40u);
        REQUIRE(arr.size() == 4);
        arr.erase(1);
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0].m_data == 10u);
        REQUIRE(arr[1].m_data == 30u);
        REQUIRE(arr[2].m_data == 40u);
    }
}

TEST_CASE("Array::erase_swap_last")
{
    {
        CstdlibAllocator allocator;

        Array<uint32_t> arr{allocator, 1};
        arr.emplace_back(10u);
        arr.emplace_back(20u);
        arr.emplace_back(30u);
        arr.emplace_back(40u);
        REQUIRE(arr.size() == 4);
        arr.erase_swap_last(1);
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0] == 10u);
        REQUIRE(arr[1] == 40u);
        REQUIRE(arr[2] == 30u);
    }
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
            Obj(Obj &&other) = default;
            Obj &operator=(Obj const &) = delete;
            Obj &operator=(Obj &&other) = default;

            uint32_t m_data{0};
        };

        CstdlibAllocator allocator;

        Array<Obj> arr{allocator, 1};
        arr.emplace_back(10u);
        arr.emplace_back(20u);
        arr.emplace_back(30u);
        arr.emplace_back(40u);
        REQUIRE(arr.size() == 4);
        arr.erase_swap_last(1);
        REQUIRE(arr.size() == 3);
        REQUIRE(arr[0].m_data == 10u);
        REQUIRE(arr[1].m_data == 40u);
        REQUIRE(arr[2].m_data == 30u);
    }
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

TEST_CASE("Array::span_conversion")
{
    CstdlibAllocator allocator;

    Array<uint8_t> arr{allocator, 32};
    for (uint8_t i = 0; i < 10; ++i)
        arr.push_back(i);

    Span<uint8_t const> const_span = arr;
    REQUIRE(const_span.data() == arr.data());
    REQUIRE(const_span.size() == arr.size());
}
