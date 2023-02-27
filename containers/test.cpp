#include <catch2/catch_test_macros.hpp>

#include "allocators/cstdlib_allocator.hpp"
#include "array.hpp"
#include "pair.hpp"
#include "small_set.hpp"
#include "static_array.hpp"

using namespace wheels;

namespace
{

struct alignas(std::max_align_t) AlignedObj
{
    uint32_t value{0};
    uint8_t _padding[alignof(std::max_align_t) - sizeof(uint32_t)];
};
static_assert(alignof(AlignedObj) > alignof(uint32_t));

bool operator==(AlignedObj const &lhs, AlignedObj const &rhs)
{
    return lhs.value == rhs.value;
}

class DtorObj
{
  public:
    static uint64_t const s_null_value = (uint64_t)-1;
    static uint64_t s_ctor_counter;
    static uint64_t s_default_ctor_counter;
    static uint64_t s_value_ctor_counter;
    static uint64_t s_copy_ctor_counter;
    static uint64_t s_move_ctor_counter;
    static uint64_t s_assign_counter;
    static uint64_t s_copy_assign_counter;
    static uint64_t s_move_assign_counter;
    static uint64_t s_dtor_counter;

    DtorObj()
    {
        s_ctor_counter++;
        s_default_ctor_counter++;
    };

    DtorObj(uint32_t data)
    : data{data}
    {
        s_ctor_counter++;
        s_value_ctor_counter++;
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
        s_copy_ctor_counter++;
    }

    DtorObj(DtorObj &&other)
    : data{other.data}
    {
        s_ctor_counter++;
        s_move_ctor_counter++;
        other.data = s_null_value;
    }

    DtorObj &operator=(DtorObj const &other)
    {
        if (this != &other)
        {
            data = other.data;
            s_assign_counter++;
            s_copy_assign_counter++;
        }
        return *this;
    }

    DtorObj &operator=(DtorObj &&other)
    {
        if (this != &other)
        {
            data = other.data;
            other.data = s_null_value;
            s_assign_counter++;
            s_move_assign_counter++;
        }
        return *this;
    }

    // -1 means the value has been moved or destroyed, which skips dtor_counter
    uint64_t data{0};
};
uint64_t DtorObj::s_dtor_counter = 0;
uint64_t DtorObj::s_ctor_counter = 0;
uint64_t DtorObj::s_default_ctor_counter = 0;
uint64_t DtorObj::s_value_ctor_counter = 0;
uint64_t DtorObj::s_copy_ctor_counter = 0;
uint64_t DtorObj::s_move_ctor_counter = 0;
uint64_t DtorObj::s_assign_counter = 0;
uint64_t DtorObj::s_copy_assign_counter = 0;
uint64_t DtorObj::s_move_assign_counter = 0;

void init_dtor_counters()
{
    DtorObj::s_dtor_counter = 0;
    DtorObj::s_ctor_counter = 0;
    DtorObj::s_default_ctor_counter = 0;
    DtorObj::s_value_ctor_counter = 0;
    DtorObj::s_copy_ctor_counter = 0;
    DtorObj::s_move_ctor_counter = 0;
    DtorObj::s_assign_counter = 0;
    DtorObj::s_copy_assign_counter = 0;
    DtorObj::s_move_assign_counter = 0;
}

bool operator==(DtorObj const &lhs, DtorObj const &rhs)
{
    return lhs.data == rhs.data;
}

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

template <size_t N>
StaticArray<uint32_t, N> init_test_static_arr_u32(size_t initial_size)
{
    assert(initial_size <= N);

    StaticArray<uint32_t, N> arr;
    for (uint32_t i = 0; i < initial_size; ++i)
        arr.push_back(10 * (i + 1));

    return arr;
}

template <size_t N>
StaticArray<DtorObj, N> init_test_static_arr_dtor(size_t initial_size)
{
    assert(initial_size <= N);

    StaticArray<DtorObj, N> arr;
    for (uint32_t i = 0; i < initial_size; ++i)
        arr.emplace_back(10 * (i + 1));

    return arr;
}

template <size_t N>
SmallSet<uint32_t, N> init_test_small_set_u32(size_t initial_size)
{
    assert(initial_size <= N);

    SmallSet<uint32_t, N> set;
    for (uint32_t i = 0; i < initial_size; ++i)
        set.insert(10 * (i + 1));

    return set;
}

template <size_t N>
SmallSet<DtorObj, N> init_test_small_set_dtor(size_t initial_size)
{
    assert(initial_size <= N);

    SmallSet<DtorObj, N> set;
    for (uint32_t i = 0; i < initial_size; ++i)
        set.insert(DtorObj{10 * (i + 1)});

    return set;
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

    init_dtor_counters();

    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);

    arr.clear();
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == DtorObj::s_ctor_counter);
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

    init_dtor_counters();

    Array<DtorObj> arr = init_test_arr_dtor(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(6);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_default_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter == 3);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_value_ctor_counter == 7);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == DtorObj::s_ctor_counter);
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

TEST_CASE("Array::aligned", "[test]")
{
    CstdlibAllocator allocator;

    Array<AlignedObj> arr{allocator, 0};

    arr.push_back({10});
    arr.push_back({20});

    REQUIRE((std::uintptr_t)&arr[0].value % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)&arr[1].value % alignof(AlignedObj) == 0);
    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
}

TEST_CASE("StaticArray::allocate_copy", "[test]")
{
    StaticArray<uint32_t, 4> arr;
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

    StaticArray<uint32_t, 4> arr_copy_constructed{arr};
    REQUIRE(arr_copy_constructed[0] == 10);
    REQUIRE(arr_copy_constructed[1] == 20);
    REQUIRE(arr_copy_constructed[2] == 30);
    REQUIRE(arr_copy_constructed.size() == 3);
    REQUIRE(arr_copy_constructed.capacity() == 4);

    StaticArray<uint32_t, 4> arr_copy_assigned;
    arr_copy_assigned = arr;
    arr_copy_assigned = WHEELS_MOV(arr_copy_assigned);
    REQUIRE(arr_copy_assigned[0] == 10);
    REQUIRE(arr_copy_assigned[1] == 20);
    REQUIRE(arr_copy_assigned[2] == 30);
    REQUIRE(arr_copy_assigned.size() == 3);
    REQUIRE(arr_copy_assigned.capacity() == 4);

    StaticArray<uint32_t, 4> arr_move_constructed{WHEELS_MOV(arr)};
    REQUIRE(arr_move_constructed[0] == 10);
    REQUIRE(arr_move_constructed[1] == 20);
    REQUIRE(arr_move_constructed[2] == 30);
    REQUIRE(arr_move_constructed.size() == 3);
    REQUIRE(arr_move_constructed.capacity() == 4);

    StaticArray<uint32_t, 4> arr_move_assigned;
    arr_move_assigned = WHEELS_MOV(arr_move_constructed);
    arr_move_assigned = WHEELS_MOV(arr_move_assigned);
    REQUIRE(arr_move_assigned[0] == 10);
    REQUIRE(arr_move_assigned[1] == 20);
    REQUIRE(arr_move_assigned[2] == 30);
    REQUIRE(arr_move_assigned.size() == 3);
    REQUIRE(arr_move_assigned.capacity() == 4);
}

TEST_CASE("StaticArray::front_back", "[test]")
{
    StaticArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.front() == 10);
    REQUIRE(arr.back() == 50);

    StaticArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.front() == 10);
    REQUIRE(arr_const.back() == 50);
}

TEST_CASE("StaticArray::begin_end", "[test]")
{
    StaticArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    StaticArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("StaticArray::clear", "[test]")
{
    init_dtor_counters();
    StaticArray<DtorObj, 5> arr = init_test_static_arr_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    arr.clear();
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == DtorObj::s_ctor_counter);
}

TEST_CASE("StaticArray::emplace", "[test]")
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

    StaticArray<Obj, 3> arr;
    arr.emplace_back(10u);
    arr.emplace_back(20u);
    arr.emplace_back(30u);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr[1].m_data == 20);
    REQUIRE(arr[2].m_data == 30);
    REQUIRE(arr.size() == 3);
}

TEST_CASE("StaticArray::pop_back", "[test]")
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

    StaticArray<Obj, 1> arr;
    arr.emplace_back(10u);
    REQUIRE(arr[0].m_data == 10);
    REQUIRE(arr.pop_back().m_data == 10);
    REQUIRE(arr.size() == 0);
}

TEST_CASE("StaticArray::resize", "[test]")
{
    init_dtor_counters();

    StaticArray<DtorObj, 6> arr = init_test_static_arr_dtor<6>(5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(6);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_default_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter == 3);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_value_ctor_counter == 7);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == DtorObj::s_ctor_counter);
}

TEST_CASE("StaticArray::range_for", "[test]")
{
    StaticArray<uint32_t, 5> arr;
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

    StaticArray<uint32_t, 5> const &arr_const = arr;
    uint32_t sum = 0;
    for (auto const &v : arr_const)
        sum += v;
    REQUIRE(sum == 63);
}

TEST_CASE("StaticArray::aligned", "[test]")
{
    CstdlibAllocator allocator;

    StaticArray<AlignedObj, 2> arr;

    arr.push_back({10});
    arr.push_back({20});

    REQUIRE((std::uintptr_t)&arr[0].value % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)&arr[1].value % alignof(AlignedObj) == 0);
    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
}

TEST_CASE("SmallSet::allocate_copy", "[test]")
{
    SmallSet<uint32_t, 4> set;
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 4);

    set.insert(10);
    set.insert(20);
    set.insert(30);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 3);

    REQUIRE(set.contains(10));
    REQUIRE(set.contains(20));
    REQUIRE(set.contains(30));
    REQUIRE(!set.contains(40));

    SmallSet<uint32_t, 4> set_copy_constructed{set};
    REQUIRE(set_copy_constructed.contains(10));
    REQUIRE(set_copy_constructed.contains(20));
    REQUIRE(set_copy_constructed.contains(30));
    REQUIRE(set_copy_constructed.size() == 3);
    REQUIRE(set_copy_constructed.capacity() == 4);

    SmallSet<uint32_t, 4> set_copy_assigned;
    set_copy_assigned = set_copy_constructed;
    set_copy_assigned = set_copy_assigned;
    REQUIRE(set_copy_assigned.contains(10));
    REQUIRE(set_copy_assigned.contains(20));
    REQUIRE(set_copy_assigned.contains(30));
    REQUIRE(set_copy_assigned.size() == 3);
    REQUIRE(set_copy_assigned.capacity() == 4);

    SmallSet<uint32_t, 4> set_move_constructed{WHEELS_MOV(set)};
    REQUIRE(set_move_constructed.contains(10));
    REQUIRE(set_move_constructed.contains(20));
    REQUIRE(set_move_constructed.contains(30));
    REQUIRE(set_move_constructed.size() == 3);
    REQUIRE(set_move_constructed.capacity() == 4);

    SmallSet<uint32_t, 4> set_move_assigned;
    set_move_assigned = WHEELS_MOV(set_move_constructed);
    set_move_assigned = WHEELS_MOV(set_move_assigned);
    REQUIRE(set_move_assigned.contains(10));
    REQUIRE(set_move_assigned.contains(20));
    REQUIRE(set_move_assigned.contains(30));
    REQUIRE(set_move_assigned.size() == 3);
    REQUIRE(set_move_assigned.capacity() == 4);
}

TEST_CASE("SmallSet::begin_end", "[test]")
{
    SmallSet<uint32_t, 3> set = init_test_small_set_u32<3>(3);
    REQUIRE(set.size() == 3);
    REQUIRE(set.begin() == set.end() - 3);

    SmallSet<uint32_t, 3> const &set_const = set;
    REQUIRE(set_const.begin() == set_const.end() - 3);
}

TEST_CASE("SmallSet::clear", "[test]")
{
    init_dtor_counters();

    SmallSet<DtorObj, 5> set = init_test_small_set_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_move_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 5);
    REQUIRE(set.capacity() == 5);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
}

TEST_CASE("SmallSet::remove", "[test]")
{
    SmallSet<uint32_t, 3> set = init_test_small_set_u32<3>(3);
    REQUIRE(set.size() == 3);
    REQUIRE(set.contains(10));
    set.remove(10);
    REQUIRE(set.size() == 2);
    REQUIRE(!set.contains(10));
    REQUIRE(set.contains(20));
    REQUIRE(set.contains(30));
    set.remove(10);
    REQUIRE(set.size() == 2);
    REQUIRE(set.contains(20));
    REQUIRE(set.contains(30));
}

TEST_CASE("SmallSet::range_for", "[test]")
{
    SmallSet<uint32_t, 5> set;
    // Make sure this skips
    for (auto &v : set)
        v++;

    set.insert(10);
    set.insert(20);
    set.insert(30);

    for (auto &v : set)
        v++;

    REQUIRE(set.contains(11));
    REQUIRE(set.contains(21));
    REQUIRE(set.contains(31));

    SmallSet<uint32_t, 5> const &set_const = set;
    uint32_t sum = 0;
    for (auto const &v : set_const)
        sum += v;
    REQUIRE(sum == 63);
}

TEST_CASE("SmallSet::aligned", "[test]")
{
    CstdlibAllocator allocator;

    SmallSet<AlignedObj, 2> set;

    set.insert({10});
    set.insert({20});

    REQUIRE(set.contains({10}));
    REQUIRE(set.contains({20}));

    uint32_t sum = 0;
    for (auto const &v : set)
        sum += v.value;
    REQUIRE(sum == 30);
}

TEST_CASE("Pair", "[test]")
{
    Pair<uint32_t, uint16_t> p0{0xDEADCAFE, 0x1234};
    Pair<uint32_t, uint16_t> p1{0xDEADCAFE, 0x1234};
    Pair<uint32_t, uint16_t> p2{0xDEADCAFE, 0xABCD};
    Pair<uint32_t, uint16_t> p3 =
        make_pair((uint32_t)0xC0FFEEEE, (uint16_t)0x1234);

    REQUIRE(p0.first == 0xDEADCAFE);
    REQUIRE(p0.second == 0x1234);
    REQUIRE(p3.first == 0xC0FFEEEE);
    REQUIRE(p3.second == 0x1234);
    REQUIRE(p0 == p1);
    REQUIRE(p0 != p2);
    REQUIRE(p0 != p3);
}