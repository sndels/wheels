#include <catch2/catch_test_macros.hpp>

#include "allocators/cstdlib_allocator.hpp"
#include "array.hpp"

using namespace wheels;

namespace
{

class DtorObj
{
  public:
    static uint64_t const s_null_value = (uint64_t)-1;
    static uint64_t s_ctor_counter;
    static uint64_t s_dtor_counter;

    DtorObj() { s_ctor_counter++; };

    DtorObj(uint32_t data)
    : data{data}
    {
        s_ctor_counter++;
    };

    ~DtorObj()
    {
        if (data < s_null_value)
        {
            s_dtor_counter++;
            data = s_null_value;
        }
    };

    DtorObj(DtorObj const &other)
    : data{other.data}
    {
        s_ctor_counter++;
    }

    DtorObj(DtorObj &&other)
    : data{other.data}
    {
        s_ctor_counter++;
        other.data = s_null_value;
    }

    DtorObj &operator=(DtorObj const &other)
    {
        if (this != &other)
            data = other.data;
        return *this;
    }

    DtorObj &operator=(DtorObj &&other)
    {
        if (this != &other)
        {
            data = other.data;
            other.data = s_null_value;
        }
        return *this;
    }

    // -1 means the value has been moved or destroyed, which skips dtor_counter
    uint64_t data{0};
};
uint64_t DtorObj::s_dtor_counter = 0;
uint64_t DtorObj::s_ctor_counter = 0;

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

TEST_CASE("Array::allocate_copy", "[test]")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 0};
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() > 0);

    arr.push_back(10);
    arr.push_back(20);
    arr.push_back(30);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 3);

    REQUIRE(arr[0] == 10);
    REQUIRE(arr[1] == 20);
    REQUIRE(arr[2] == 30);

    Array<uint32_t> arr_move_constructed{std::move(arr)};
    REQUIRE(arr_move_constructed[0] == 10);
    REQUIRE(arr_move_constructed[2] == 30);

    Array<uint32_t> arr_move_assigned{allocator, 1};
    arr_move_assigned = std::move(arr_move_constructed);
    arr_move_assigned = std::move(arr_move_assigned);
    REQUIRE(arr_move_assigned[0] == 10);
    REQUIRE(arr_move_assigned[2] == 30);
}

TEST_CASE("Array::reserve", "[test]")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 1};
    arr.push_back(10);
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

TEST_CASE("Array::front_back", "[test]")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr = init_test_arr_u32(allocator, 5);
    REQUIRE(arr.front() == 10);
    REQUIRE(arr.back() == 50);

    Array<uint32_t> const &arr_const = arr;
    REQUIRE(arr_const.front() == 10);
    REQUIRE(arr_const.back() == 50);
}

TEST_CASE("Array::begin_end", "[test]")
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

TEST_CASE("Array::clear", "[test]")
{
    CstdlibAllocator allocator;

    DtorObj::s_ctor_counter = 0;
    DtorObj::s_dtor_counter = 0;
    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    arr.clear();
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_dtor_counter == 5);
}

TEST_CASE("Array::emplace", "[test]")
{
    class Obj
    {
      public:
        Obj(uint32_t value) { m_data = value; };

        Obj(Obj const &) = delete;
        Obj(Obj &&) = delete;
        Obj &operator=(Obj const &) = delete;
        Obj &operator=(Obj &&) = delete;

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

TEST_CASE("Array::pop_back", "[test]")
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

TEST_CASE("Array::resize", "[test]")
{
    CstdlibAllocator allocator;

    DtorObj::s_ctor_counter = 0;
    DtorObj::s_dtor_counter = 0;
    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    arr.resize(6);
    // Resize(size) shouldn't do copies internally
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);
    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_dtor_counter == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    arr.resize(4, DtorObj{11});
    // Default value ctor
    REQUIRE(DtorObj::s_ctor_counter == 7);
    // Default value dtor
    REQUIRE(DtorObj::s_dtor_counter == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);
    arr.resize(2, DtorObj{15});
    // Default value ctor
    REQUIRE(DtorObj::s_ctor_counter == 8);
    // Default value and two tail values dtors
    REQUIRE(DtorObj::s_dtor_counter == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);
}

TEST_CASE("Array::range_for", "[test]")
{
    CstdlibAllocator allocator;

    Array<uint32_t> arr{allocator, 1};
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

    Array<uint32_t> const &arr_const = arr;
    uint32_t sum = 0;
    for (auto const &v : arr_const)
        sum += v;
    REQUIRE(sum == 63);
}
