#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/small_map.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

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
        map.insert_or_assign(DtorObj{10 * i}, DtorObj{10 * (i + 1)});

    return map;
}

} // namespace

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

    DtorObj const lvalue_value = DtorObj{97};
    map.insert_or_assign(DtorObj{96}, lvalue_value);
    REQUIRE(DtorObj::s_ctor_counter() == 4);
    REQUIRE(DtorObj::s_value_ctor_counter() == 2);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 2);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 1);
    REQUIRE(map.size() == 1);
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
// TODO: Why do these differ?
#if defined(_MSC_VER) && !defined(NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 40);
    REQUIRE(DtorObj::s_move_ctor_counter() == 30);
#else  // !(_MSC_VER && !NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 30);
    REQUIRE(DtorObj::s_move_ctor_counter() == 20);
#endif // _MSC_VER && !NDEBUG
    REQUIRE(DtorObj::s_value_ctor_counter() == 10);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!map.empty());
    REQUIRE(map.size() == 5);
    REQUIRE(map.capacity() == 5);

    map.clear();
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 5);
#if defined(_MSC_VER) && !defined(NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 40);
#else  // !(_MSC_VER && !NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 30);
#endif // _MSC_VER && !NDEBUG
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 10);
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
    SmallMap<AlignedObj, AlignedObj, 2> map;

    map.insert_or_assign(AlignedObj{10}, AlignedObj{11});
    map.insert_or_assign(AlignedObj{20}, AlignedObj{21});

    REQUIRE(map.contains({10}));
    REQUIRE(map.contains({20}));

    uint32_t sum = 0;
    for (auto const &v : map)
        sum += v.second.value;
    REQUIRE(sum == 32);
}
