#include <catch2/catch_test_macros.hpp>

#include <allocators/cstdlib_allocator.hpp>
#include <containers/array.hpp>
#include <containers/hash.hpp>
#include <containers/hash_set.hpp>
#include <containers/optional.hpp>
#include <containers/pair.hpp>
#include <containers/small_map.hpp>
#include <containers/small_set.hpp>
#include <containers/static_array.hpp>

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

struct AlignedHash
{
    /// Delete implementation for types that don't specifically override with a
    /// valid hasher implementation
    uint64_t operator()(AlignedObj const &value) const noexcept
    {
        return wyhash(&value.value, sizeof(value.value), 0, _wyp);
    }
};

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

struct DtorHash
{
    /// Delete implementation for types that don't specifically override with a
    /// valid hasher implementation
    uint64_t operator()(DtorObj const &value) const noexcept
    {
        return wyhash(&value.data, sizeof(value.data), 0, _wyp);
    }
};

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

HashSet<uint32_t, Hash<uint32_t>> init_test_hash_set_u32(
    Allocator &allocator, size_t size)
{
    HashSet<uint32_t, Hash<uint32_t>> set{allocator, size};
    for (uint32_t i = 0; i < size; ++i)
        set.insert(10 * (i + 1));

    return set;
}

HashSet<DtorObj, DtorHash> init_test_hash_set_dtor(
    Allocator &allocator, size_t size)
{
    HashSet<DtorObj, DtorHash> set{allocator, size};
    for (uint32_t i = 0; i < size; ++i)
        set.insert(DtorObj{10 * (i + 1)});

    return set;
}

template <size_t N>
SmallMap<uint32_t, uint32_t, N> init_test_small_map_u32(size_t initial_size)
{
    assert(initial_size <= N);

    SmallMap<uint32_t, uint32_t, N> map;
    for (uint32_t i = 0; i < initial_size; ++i)
        map.insert_or_assign(10 * (i + 1), 10 * (i + 1) + 1);

    return map;
}

template <size_t N>
SmallMap<DtorObj, DtorObj, N> init_test_small_map_dtor(size_t initial_size)
{
    assert(initial_size <= N);

    SmallMap<DtorObj, DtorObj, N> map;
    for (uint32_t i = 0; i < initial_size; ++i)
        map.insert_or_assign(make_pair(DtorObj{10 * i}, DtorObj{10 * (i + 1)}));

    return map;
}

} // namespace

TEST_CASE("Array::allocate_copy")
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

TEST_CASE("Array::push_lvalue")
{
    CstdlibAllocator allocator;

    Array<DtorObj> arr{allocator, 0};
    init_dtor_counters();
    DtorObj const lvalue = {99};
    arr.push_back(lvalue);
    REQUIRE(DtorObj::s_ctor_counter == 2);
    REQUIRE(DtorObj::s_value_ctor_counter == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0] == 99);
}

TEST_CASE("Array::reserve")
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
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_default_ctor_counter == 1);
    REQUIRE(DtorObj::s_move_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter == 15);
    REQUIRE(DtorObj::s_value_ctor_counter == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter == 3);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_value_ctor_counter == 7);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(
        DtorObj::s_dtor_counter ==
        DtorObj::s_ctor_counter - DtorObj::s_move_ctor_counter);
}

TEST_CASE("Array::range_for")
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

TEST_CASE("Array::aligned")
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

TEST_CASE("StaticArray::allocate_copy")
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

TEST_CASE("StaticArray::push_lvalue")
{
    init_dtor_counters();

    StaticArray<DtorObj, 5> arr;
    DtorObj const lvalue = {99};
    arr.push_back(lvalue);
    REQUIRE(DtorObj::s_ctor_counter == 2);
    REQUIRE(DtorObj::s_value_ctor_counter == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr[0] == 99);
    arr[0].data = 11;
}

TEST_CASE("StaticArray::front_back")
{
    StaticArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.front() == 10);
    REQUIRE(arr.back() == 50);

    StaticArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.front() == 10);
    REQUIRE(arr_const.back() == 50);
}

TEST_CASE("StaticArray::begin_end")
{
    StaticArray<uint32_t, 5> arr = init_test_static_arr_u32<5>(5);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.begin() == arr.data());
    REQUIRE(arr.end() == arr.data() + arr.size());

    StaticArray<uint32_t, 5> const &arr_const = arr;
    REQUIRE(arr_const.begin() == arr_const.data());
    REQUIRE(arr_const.end() == arr_const.data() + arr_const.size());
}

TEST_CASE("StaticArray::clear")
{
    init_dtor_counters();
    StaticArray<DtorObj, 5> arr = init_test_static_arr_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_move_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!arr.empty());
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 5);
    arr.clear();
    REQUIRE(arr.empty());
    REQUIRE(arr.size() == 0);
    REQUIRE(arr.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(
        DtorObj::s_dtor_counter ==
        DtorObj::s_ctor_counter - DtorObj::s_move_ctor_counter);
}

TEST_CASE("StaticArray::emplace")
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

TEST_CASE("StaticArray::pop_back")
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

TEST_CASE("StaticArray::resize")
{
    init_dtor_counters();

    StaticArray<DtorObj, 6> arr = init_test_static_arr_dtor<6>(5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_move_ctor_counter == 5);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(5);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 5);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);

    arr.resize(6);
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_default_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(arr.size() == 6);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[4].data == 50);
    REQUIRE(arr[5].data == 0);

    arr.resize(1);
    REQUIRE(DtorObj::s_ctor_counter == 11);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
    REQUIRE(arr.size() == 1);
    REQUIRE(arr.capacity() == 6);
    REQUIRE(arr[0].data == 10);

    arr.resize(4, DtorObj{11});
    REQUIRE(DtorObj::s_ctor_counter == 15);
    REQUIRE(DtorObj::s_value_ctor_counter == 6);
    REQUIRE(DtorObj::s_copy_ctor_counter == 3);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 6);
    REQUIRE(arr.size() == 4);
    REQUIRE(arr.capacity() == 6);
    for (size_t i = 1; i < 4; ++i)
        REQUIRE(arr[i].data == 11);

    arr.resize(2, DtorObj{15});
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_value_ctor_counter == 7);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 9);
    REQUIRE(arr[0].data == 10);
    REQUIRE(arr[1].data == 11);

    arr.clear();
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(
        DtorObj::s_dtor_counter ==
        DtorObj::s_ctor_counter - DtorObj::s_move_ctor_counter);
}

TEST_CASE("StaticArray::range_for")
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

TEST_CASE("StaticArray::aligned")
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

TEST_CASE("SmallSet::allocate_copy")
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

TEST_CASE("SmallSet::insert_lvalue")
{
    init_dtor_counters();

    SmallSet<DtorObj, 5> set;
    DtorObj const lvalue = {99};
    set.insert(lvalue);
    REQUIRE(DtorObj::s_ctor_counter == 2);
    REQUIRE(DtorObj::s_value_ctor_counter == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(set.size() == 1);
    REQUIRE(set.contains(lvalue));
}

TEST_CASE("SmallSet::begin_end")
{
    SmallSet<uint32_t, 3> set = init_test_small_set_u32<3>(3);
    REQUIRE(set.size() == 3);
    REQUIRE(set.begin() == set.end() - 3);

    SmallSet<uint32_t, 3> const &set_const = set;
    REQUIRE(set_const.begin() == set_const.end() - 3);
}

TEST_CASE("SmallSet::clear")
{
    init_dtor_counters();

    SmallSet<DtorObj, 5> set = init_test_small_set_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter == 15);
    REQUIRE(DtorObj::s_value_ctor_counter == 5);
    REQUIRE(DtorObj::s_move_ctor_counter == 10);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 5);
    REQUIRE(set.capacity() == 5);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 15);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 5);
}

TEST_CASE("SmallSet::remove")
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

TEST_CASE("SmallSet::range_for")
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

TEST_CASE("SmallSet::aligned")
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

TEST_CASE("HashSet::allocate_copy")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set{allocator, 8};
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);

    set.insert(10u);
    set.insert(20u);
    set.insert(30u);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 3);

    REQUIRE(set.contains(10));
    REQUIRE(set.contains(20));
    REQUIRE(set.contains(30));
    REQUIRE(!set.contains(40));

    HashSet<uint32_t> set_move_constructed{WHEELS_MOV(set)};
    REQUIRE(set_move_constructed.contains(10));
    REQUIRE(set_move_constructed.contains(20));
    REQUIRE(set_move_constructed.contains(30));
    REQUIRE(set_move_constructed.size() == 3);
    REQUIRE(set_move_constructed.capacity() == 32);

    HashSet<uint32_t> set_move_assigned{allocator, 1};
    set_move_assigned = WHEELS_MOV(set_move_constructed);
    set_move_assigned = WHEELS_MOV(set_move_assigned);
    REQUIRE(set_move_assigned.contains(10));
    REQUIRE(set_move_assigned.contains(20));
    REQUIRE(set_move_assigned.contains(30));
    REQUIRE(set_move_assigned.size() == 3);
    REQUIRE(set_move_assigned.capacity() == 32);
}

TEST_CASE("HashSet::grow")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set{allocator, 4};
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);

    set.insert(10u);
    set.insert(20u);
    REQUIRE(set.size() == 2);
    REQUIRE(set.capacity() == 32);

    // Let's stress to tickle out bugs from un- or wrongly initialized memory
    for (uint32_t i = 3; i <= 8096; ++i)
        set.insert(i * 10);

    REQUIRE(set.size() == 8096);
    REQUIRE(set.capacity() == 16384);
    for (uint32_t i = 1; i <= 8096; ++i)
        REQUIRE(set.contains(i * 10));
}

TEST_CASE("HashSet::reinsert")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set{allocator, 4};
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);
    set.insert(0u);

    // Try to cause a case where all of the values are either Full or Deleted
    for (size_t i = 0; i < 8096; ++i)
    {
        uint32_t const value = rand();
        set.insert(value == 0 ? 1 : value);
        REQUIRE(set.contains(value == 0 ? 1 : value));
        set.remove(value == 0 ? 1 : value);
    }
    REQUIRE(set.size() == 1);
    REQUIRE(set.capacity() == 32);

    set.remove(0);
    REQUIRE(set.size() == 0);
}

TEST_CASE("HashSet::insert_lvalue")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    HashSet<DtorObj, DtorHash> set{allocator};
    DtorObj const lvalue = {99};
    set.insert(lvalue);
    REQUIRE(DtorObj::s_ctor_counter == 2);
    REQUIRE(DtorObj::s_value_ctor_counter == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(set.size() == 1);
    REQUIRE(set.contains(lvalue));
}

TEST_CASE("HashSet::begin_end")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set = init_test_hash_set_u32(allocator, 3);
    REQUIRE(set.size() == 3);
    REQUIRE(set.begin() != set.end());
    HashSet<uint32_t>::ConstIterator iter = set.begin();
    ++iter;
    REQUIRE(iter != set.end());
    ++iter;
    REQUIRE(iter != set.end());
    ++iter;
    REQUIRE(iter == set.end());
}

TEST_CASE("HashSet::clear")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    HashSet<DtorObj, DtorHash> set = init_test_hash_set_dtor(allocator, 8);
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_value_ctor_counter == 8);
    REQUIRE(DtorObj::s_move_ctor_counter == 8);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 8);
    REQUIRE(set.capacity() == 32);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);
    REQUIRE(DtorObj::s_ctor_counter == 16);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 8);
}

TEST_CASE("HashSet::remove")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set = init_test_hash_set_u32(allocator, 3);
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

TEST_CASE("HashSet::range_for")
{
    CstdlibAllocator allocator;

    HashSet<uint32_t> set{allocator, 5};
    // Make sure this skips
    uint32_t sum = 0;
    for (auto v : set)
        sum += v;
    REQUIRE(sum == 0);

    set.insert(10u);
    set.insert(20u);
    set.insert(30u);

    sum = 0;
    for (auto &v : set)
        sum += v;
    REQUIRE(sum == 60);

    HashSet<uint32_t> const &set_const = set;
    sum = 0;
    for (auto const &v : set_const)
        sum += v;
    REQUIRE(sum == 60);
}

TEST_CASE("HashSet::aligned")
{
    CstdlibAllocator allocator;

    HashSet<AlignedObj, AlignedHash> set{allocator, 5};

    set.insert(AlignedObj{10});
    set.insert(AlignedObj{20});

    REQUIRE(set.contains({10}));
    REQUIRE(set.contains({20}));

    uint32_t sum = 0;
    for (auto const &v : set)
        sum += v.value;
    REQUIRE(sum == 30);
}

TEST_CASE("Pair")
{
    uint32_t const deadcafe = 0xDEADCAFE;
    uint32_t const coffee = 0xC0FFEEEE;
    uint16_t const onetwothreefour = 0x1234;
    uint16_t const abcd = 0xABCD;
    Pair<uint32_t, uint16_t> p0{deadcafe, onetwothreefour};
    Pair<uint32_t, uint16_t> p1{0xDEADCAFE, std::move(onetwothreefour)};
    Pair<uint32_t, uint16_t> p2{std::move(deadcafe), abcd};
    Pair<uint32_t, uint16_t> p3{std::move(coffee), std::move(abcd)};
    Pair<uint32_t, uint16_t> p4 =
        make_pair((uint32_t)0xC0FFEEEE, (uint16_t)0x1234);

    REQUIRE(p0.first == 0xDEADCAFE);
    REQUIRE(p0.second == 0x1234);
    REQUIRE(p1.first == 0xDEADCAFE);
    REQUIRE(p1.second == 0x1234);
    REQUIRE(p2.first == 0xDEADCAFE);
    REQUIRE(p2.second == 0xABCD);
    REQUIRE(p3.first == 0xC0FFEEEE);
    REQUIRE(p3.second == 0xABCD);
    REQUIRE(p4.first == 0xC0FFEEEE);
    REQUIRE(p4.second == 0x1234);
    REQUIRE(p0 == p1);
    REQUIRE(p0 != p2);
    REQUIRE(p0 != p4);
}

TEST_CASE("SmallMap::allocate_copy")
{
    SmallMap<uint32_t, uint16_t, 4> map;
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 4);

    map.insert_or_assign(10, 11);
    map.insert_or_assign(20, 21);
    map.insert_or_assign(30, 31);
    REQUIRE(!map.empty());
    REQUIRE(map.size() == 3);

    REQUIRE(map.contains(10));
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));
    REQUIRE(!map.contains(40));
    REQUIRE(*map.find(10) == 11);
    REQUIRE(*map.find(20) == 21);
    REQUIRE(*map.find(30) == 31);
    REQUIRE(map.find(40) == nullptr);
    map.insert_or_assign(make_pair((uint32_t)30, (uint16_t)29));
    REQUIRE(map.size() == 3);
    REQUIRE(*map.find(30) == 29);
    map.insert_or_assign(make_pair((uint32_t)30, (uint16_t)31));

    SmallMap<uint32_t, uint16_t, 4> map_copy_constructed{map};
    REQUIRE(*map_copy_constructed.find(10) == 11);
    REQUIRE(*map_copy_constructed.find(20) == 21);
    REQUIRE(*map_copy_constructed.find(30) == 31);
    REQUIRE(map_copy_constructed.size() == 3);

    SmallMap<uint32_t, uint16_t, 4> map_copy_assigned;
    map_copy_assigned = map_copy_constructed;
    map_copy_assigned = map_copy_assigned;
    REQUIRE(*map_copy_assigned.find(10) == 11);
    REQUIRE(*map_copy_assigned.find(20) == 21);
    REQUIRE(*map_copy_assigned.find(30) == 31);
    REQUIRE(map_copy_assigned.size() == 3);

    SmallMap<uint32_t, uint16_t, 4> map_move_constructed{WHEELS_MOV(map)};
    REQUIRE(*map_move_constructed.find(10) == 11);
    REQUIRE(*map_move_constructed.find(20) == 21);
    REQUIRE(*map_move_constructed.find(30) == 31);
    REQUIRE(map_move_constructed.size() == 3);

    SmallMap<uint32_t, uint16_t, 4> map_move_assigned;
    map_move_assigned = WHEELS_MOV(map_move_constructed);
    map_move_assigned = WHEELS_MOV(map_move_assigned);
    REQUIRE(*map_move_assigned.find(10) == 11);
    REQUIRE(*map_move_assigned.find(20) == 21);
    REQUIRE(*map_move_assigned.find(30) == 31);
    REQUIRE(map_move_assigned.size() == 3);
}

TEST_CASE("SmallMap::insert_lvalue")
{
    init_dtor_counters();

    SmallMap<DtorObj, DtorObj, 5> map;

    Pair<DtorObj, DtorObj> const lvalue_pair =
        make_pair(DtorObj{98}, DtorObj{99});
    map.insert_or_assign(lvalue_pair);
    REQUIRE(DtorObj::s_ctor_counter == 6);
    REQUIRE(DtorObj::s_value_ctor_counter == 2);
    REQUIRE(DtorObj::s_copy_ctor_counter == 2);
    REQUIRE(DtorObj::s_move_ctor_counter == 2);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(map.size() == 1);
    REQUIRE(map.contains(lvalue_pair.first));

    DtorObj const lvalue_value = DtorObj{97};
    map.insert_or_assign(DtorObj{96}, lvalue_value);
    REQUIRE(DtorObj::s_ctor_counter == 10);
    REQUIRE(DtorObj::s_value_ctor_counter == 4);
    REQUIRE(DtorObj::s_copy_ctor_counter == 4);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 1);
    REQUIRE(map.size() == 2);
    REQUIRE(map.contains(DtorObj{96}));
}

TEST_CASE("SmallMap::begin_end")
{
    SmallMap<uint32_t, uint32_t, 3> map = init_test_small_map_u32<3>(3);
    REQUIRE(map.size() == 3);
    REQUIRE(map.begin() == map.end() - 3);

    SmallMap<uint32_t, uint32_t, 3> const &map_const = map;
    REQUIRE(map_const.begin() == map_const.end() - 3);
}

TEST_CASE("SmallMap::clear")
{
    init_dtor_counters();
    SmallMap<DtorObj, DtorObj, 5> map = init_test_small_map_dtor<5>(5);
    REQUIRE(DtorObj::s_ctor_counter == 40);
    REQUIRE(DtorObj::s_value_ctor_counter == 10);
    REQUIRE(DtorObj::s_copy_ctor_counter == 0);
    REQUIRE(DtorObj::s_move_ctor_counter == 30);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(!map.empty());
    REQUIRE(map.size() == 5);
    REQUIRE(map.capacity() == 5);

    map.clear();
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 5);
    REQUIRE(DtorObj::s_ctor_counter == 40);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 10);
}

TEST_CASE("SmallMap::remove")
{
    SmallMap<uint32_t, uint32_t, 3> map = init_test_small_map_u32<3>(3);
    REQUIRE(map.size() == 3);
    REQUIRE(map.contains(10));
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));
    map.remove(10);
    REQUIRE(map.size() == 2);
    REQUIRE(!map.contains(10));
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));
    map.remove(10);
    REQUIRE(map.size() == 2);
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));
}

TEST_CASE("SmallMap::range_for")
{
    SmallMap<uint32_t, uint32_t, 5> map;
    // Make sure this skips
    for (auto &v : map)
        v.second++;

    map = init_test_small_map_u32<5>(3);

    for (auto &v : map)
        v.second++;

    REQUIRE(map.contains(10));
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));

    SmallMap<uint32_t, uint32_t, 5> const &map_const = map;
    uint32_t sum = 0;
    for (auto const &v : map_const)
        sum += v.second;
    REQUIRE(sum == 66);
}

TEST_CASE("SmallMap::aligned")
{
    CstdlibAllocator allocator;

    SmallMap<AlignedObj, AlignedObj, 2> map;

    map.insert_or_assign(make_pair(AlignedObj{10}, AlignedObj{11}));
    map.insert_or_assign(make_pair(AlignedObj{20}, AlignedObj{21}));

    REQUIRE(map.contains({10}));
    REQUIRE(map.contains({20}));

    uint32_t sum = 0;
    for (auto const &v : map)
        sum += v.second.value;
    REQUIRE(sum == 32);
}

TEST_CASE("hash::pointers", "[hash]")
{
    {
        Hash<uint64_t *> hasher;
        uint64_t zero = 0;
        uint64_t const const_zero = 0;
        REQUIRE(hasher(&zero) == hasher(&zero));
        REQUIRE(hasher(&const_zero) == hasher(&const_zero));
    }

    {
        struct CustomT
        {
            uint8_t value;
        };
        Hash<CustomT *> hasher;
        CustomT zero = {};
        CustomT const const_zero = {};
        REQUIRE(hasher(&zero) == hasher(&zero));
        REQUIRE(hasher(&const_zero) == hasher(&const_zero));
    }
}

#define STR2(v) #v
#define STR(v) STR2(v)
#define HASH_TEST_IMPLEMENTATION(T)                                            \
    TEST_CASE("hash::" STR(T), "[hash]")                                       \
    {                                                                          \
        Hash<T> hasher;                                                        \
        T zero = static_cast<T>(0);                                            \
        T const const_zero = static_cast<T>(0);                                \
        REQUIRE(hasher(zero) == hasher(zero));                                 \
        REQUIRE(hasher(const_zero) == hasher(const_zero));                     \
    }

HASH_TEST_IMPLEMENTATION(int8_t);
HASH_TEST_IMPLEMENTATION(uint8_t);
HASH_TEST_IMPLEMENTATION(int16_t);
HASH_TEST_IMPLEMENTATION(uint16_t);
HASH_TEST_IMPLEMENTATION(int32_t);
HASH_TEST_IMPLEMENTATION(uint32_t);
HASH_TEST_IMPLEMENTATION(int64_t);
HASH_TEST_IMPLEMENTATION(uint64_t);
HASH_TEST_IMPLEMENTATION(float);
HASH_TEST_IMPLEMENTATION(double);

#undef HASH_TEST_IMPLEMENTATIOn
#undef STR
#undef STR2

TEST_CASE("Optional::ctors")
{
    {
        Optional<uint32_t> opt;
        REQUIRE(!opt.has_value());
    }

    {
        init_dtor_counters();
        DtorObj const val{2};
        Optional<DtorObj> opt{val};
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_copy_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 2);

    {
        init_dtor_counters();
        DtorObj val{2};
        Optional<DtorObj> opt{WHEELS_MOV(val)};
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 1);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2{opt};
        REQUIRE(DtorObj::s_ctor_counter == 3);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 1);
        REQUIRE(DtorObj::s_copy_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt2.has_value());
        REQUIRE((*opt2).data == 2);
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 2);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2{WHEELS_MOV(opt)};
        REQUIRE(DtorObj::s_ctor_counter == 3);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 2);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt2.has_value());
        REQUIRE((*opt2).data == 2);
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 1);
}

TEST_CASE("Optional::assignments")
{
    {
        init_dtor_counters();
        Optional<DtorObj> opt;
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter == 0);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(!opt.has_value());
        REQUIRE(!opt2.has_value());

        opt2 = opt;
        REQUIRE(DtorObj::s_ctor_counter == 0);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(!opt.has_value());
        REQUIRE(!opt2.has_value());

        opt2 = WHEELS_MOV(opt);
        REQUIRE(DtorObj::s_ctor_counter == 0);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(!opt2.has_value());
    }
    REQUIRE(DtorObj::s_dtor_counter == 0);

    {
        init_dtor_counters();
        Optional<DtorObj> const opt{DtorObj{2}};
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
        REQUIRE(!opt2.has_value());

        opt2 = opt;

        REQUIRE(DtorObj::s_ctor_counter == 3);
        REQUIRE(DtorObj::s_copy_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
        REQUIRE(opt2.has_value());
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 2);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());
        REQUIRE(opt->data == 2);
        REQUIRE(!opt2.has_value());

        opt2 = WHEELS_MOV(opt);

        REQUIRE(DtorObj::s_ctor_counter == 3);
        REQUIRE(DtorObj::s_move_ctor_counter == 2);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt2.has_value());
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter == 1);
}

TEST_CASE("Optional::aligned")
{
    AlignedObj const value{2};
    Optional<AlignedObj> map{value};
    REQUIRE(map.has_value());
    REQUIRE(*map == value);
    REQUIRE(map->value == value.value);
}

TEST_CASE("Optional::emplace")
{
    init_dtor_counters();
    Optional<DtorObj> opt;
    opt.emplace(2u);
    REQUIRE(DtorObj::s_ctor_counter == 1);
    REQUIRE(DtorObj::s_value_ctor_counter == 1);
    REQUIRE(DtorObj::s_assign_counter == 0);
    REQUIRE(DtorObj::s_dtor_counter == 0);
    REQUIRE(opt.has_value());
    REQUIRE(opt->data == 2);
}

TEST_CASE("Optional::reset")
{
    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_value_ctor_counter == 1);
        REQUIRE(DtorObj::s_move_ctor_counter == 1);
        REQUIRE(DtorObj::s_assign_counter == 0);
        REQUIRE(DtorObj::s_dtor_counter == 0);
        REQUIRE(opt.has_value());

        opt.reset();
        REQUIRE(!opt.has_value());
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_dtor_counter == 1);

        opt.reset();
        REQUIRE(!opt.has_value());
        REQUIRE(DtorObj::s_ctor_counter == 2);
        REQUIRE(DtorObj::s_dtor_counter == 1);
    }
    REQUIRE(DtorObj::s_dtor_counter == 1);
}