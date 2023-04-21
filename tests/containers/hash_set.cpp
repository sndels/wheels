#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/hash.hpp>
#include <wheels/containers/hash_set.hpp>

#include "common.hpp"

using namespace wheels;

// TODO:
// Check that non trivially copyable types go through move ctor on
// reallocations and the likes that actually move the objects in memory.

namespace
{

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

} // namespace

TEST_CASE("HashSet::allocate_copy")
{
    CstdlibAllocator allocator;

    { // Initial capacity should be allocated, potentially rounded up
        size_t const cap = 8;
        HashSet<uint32_t> set{allocator, cap};
        REQUIRE(set.empty());
        REQUIRE(set.size() == 0);
        REQUIRE(set.capacity() >= cap);
    }

    HashSet<uint32_t> set{allocator};
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 0);
    REQUIRE(set.contains(0) == false);

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

TEST_CASE("HashSet::find")
{
    CstdlibAllocator allocator;

    HashSet<DtorObj, DtorHash> set{allocator, 8};
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);

    set.insert(DtorObj{10u});
    set.insert(DtorObj{20u});
    set.insert(DtorObj{30u});

    auto iter = set.find(DtorObj{10u});
    REQUIRE(iter != set.end());
    REQUIRE((*iter).data == 10u);
    REQUIRE(iter->data == 10u);
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
    REQUIRE(DtorObj::s_ctor_counter() == 2);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
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

    HashSet<uint32_t> empty_set{allocator};
    REQUIRE(empty_set.begin() == empty_set.end());
}

TEST_CASE("HashSet::clear")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    HashSet<DtorObj, DtorHash> set = init_test_hash_set_dtor(allocator, 8);
    REQUIRE(DtorObj::s_ctor_counter() == 16);
    REQUIRE(DtorObj::s_value_ctor_counter() == 8);
    REQUIRE(DtorObj::s_move_ctor_counter() == 8);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!set.empty());
    REQUIRE(set.size() == 8);
    REQUIRE(set.capacity() == 32);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(set.size() == 0);
    REQUIRE(set.capacity() == 32);
    REQUIRE(DtorObj::s_ctor_counter() == 16);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 8);
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

    HashSet<uint32_t> empty_set{allocator};
    empty_set.remove(0);
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
