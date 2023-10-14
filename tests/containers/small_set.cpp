#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/small_set.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

template <size_t N>
SmallSet<uint32_t, N> init_test_small_set_u32(size_t initial_size)
{
    REQUIRE(initial_size <= N);

    SmallSet<uint32_t, N> set;
    for (uint32_t i = 0; i < initial_size; ++i)
        set.insert(10 * (i + 1));

    return set;
}

template <size_t N>
SmallSet<DtorObj, N> init_test_small_set_dtor(size_t initial_size)
{
    REQUIRE(initial_size <= N);

    SmallSet<DtorObj, N> set;
    for (uint32_t i = 0; i < initial_size; ++i)
        set.insert(DtorObj{10 * (i + 1)});

    return set;
}

} // namespace

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
    REQUIRE(DtorObj::s_ctor_counter() == 2);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
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
// TODO: Why do these differ?
#if defined(_MSC_VER) && !defined(NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 15);
    REQUIRE(DtorObj::s_move_ctor_counter() == 10);
#else  // !(_MSC_VER && !NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 10);
    REQUIRE(DtorObj::s_move_ctor_counter() == 5);
#endif // _MSC_VER && !NDEBUG
    REQUIRE(DtorObj::s_value_ctor_counter() == 5);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 5);
    REQUIRE(set.capacity() == 5);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 5);
#if defined(_MSC_VER) && !defined(NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 15);
#else  // !(_MSC_VER && !NDEBUG)
    REQUIRE(DtorObj::s_ctor_counter() == 10);
#endif // _MSC_VER && !NDEBUG
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 5);
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
