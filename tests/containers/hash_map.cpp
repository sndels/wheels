#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/hash_map.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

template <size_t N>
HashMap<uint32_t, uint32_t> init_test_small_map_u32(
    Allocator &allocator, size_t initial_size)
{
    REQUIRE(initial_size <= N);

    HashMap<uint32_t, uint32_t> map{allocator};
    for (uint32_t i = 0; i < initial_size; ++i)
        map.insert_or_assign(10 * (i + 1), 10 * (i + 1) + 1);

    return map;
}

template <size_t N>
HashMap<DtorObj, DtorObj, DtorHash> init_test_small_map_dtor(
    Allocator &allocator, size_t initial_size)
{
    REQUIRE(initial_size <= N);

    HashMap<DtorObj, DtorObj, DtorHash> map{allocator};
    for (uint32_t i = 0; i < initial_size; ++i)
        map.insert_or_assign(DtorObj{10 * i}, DtorObj{10 * (i + 1)});

    return map;
}

} // namespace

TEST_CASE("HashMap::allocate_copy")
{
    CstdlibAllocator allocator;

    { // Initial capacity should be allocated, potentially rounded up
        size_t const cap = 8;
        HashMap<uint32_t, uint16_t> map{allocator, cap};
        REQUIRE(map.empty());
        REQUIRE(map.size() == 0);
        REQUIRE(map.capacity() >= cap);
    }

    HashMap<uint32_t, uint16_t> map{allocator};
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 0);
    REQUIRE(map.find(0) == nullptr);

    HashMap<uint32_t, uint16_t> const &const_map = map;
    REQUIRE(const_map.find(0) == nullptr);

    map.insert_or_assign(10u, (uint16_t)11);
    map.insert_or_assign(20u, (uint16_t)21);
    map.insert_or_assign(30u, (uint16_t)31);
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

    HashMap<uint32_t, uint16_t> map_move_constructed{WHEELS_MOV(map)};
    REQUIRE(*map_move_constructed.find(10) == 11);
    REQUIRE(*map_move_constructed.find(20) == 21);
    REQUIRE(*map_move_constructed.find(30) == 31);
    REQUIRE(map_move_constructed.size() == 3);

    HashMap<uint32_t, uint16_t> map_move_assigned{allocator};
    map_move_assigned = WHEELS_MOV(map_move_constructed);
    map_move_assigned = WHEELS_MOV(map_move_assigned);
    REQUIRE(*map_move_assigned.find(10) == 11);
    REQUIRE(*map_move_assigned.find(20) == 21);
    REQUIRE(*map_move_assigned.find(30) == 31);
    REQUIRE(map_move_assigned.size() == 3);
}

TEST_CASE("HashMap::insert_lvalue")
{
    CstdlibAllocator allocator;

    init_dtor_counters();

    HashMap<DtorObj, DtorObj, DtorHash> map{allocator};

    DtorObj const lvalue_value = DtorObj{97};
    map.insert_or_assign(DtorObj{96}, lvalue_value);
    REQUIRE(DtorObj::s_ctor_counter() == 4);
    REQUIRE(DtorObj::s_value_ctor_counter() == 2);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
    REQUIRE(DtorObj::s_move_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(map.size() == 1);
    REQUIRE(map.contains(DtorObj{96}));
}

TEST_CASE("HashMap::begin_end")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> empty_map{allocator};
    REQUIRE(empty_map.size() == 0);
    REQUIRE(empty_map.begin() == empty_map.end());

    HashMap<uint32_t, uint32_t> const &const_empty_map = empty_map;
    REQUIRE(const_empty_map.begin() == const_empty_map.end());

    HashMap<uint32_t, uint32_t> map = init_test_small_map_u32<3>(allocator, 3);
    REQUIRE(map.size() == 3);
    REQUIRE(++ ++ ++(map.begin()) == map.end());
}

TEST_CASE("HashMap::clear")
{
    CstdlibAllocator allocator;

    init_dtor_counters();
    HashMap<DtorObj, DtorObj, DtorHash> map =
        init_test_small_map_dtor<5>(allocator, 5);
    REQUIRE(DtorObj::s_ctor_counter() == 20);
    REQUIRE(DtorObj::s_move_ctor_counter() == 10);
    REQUIRE(DtorObj::s_value_ctor_counter() == 10);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!map.empty());
    REQUIRE(map.size() == 5);
    REQUIRE(map.capacity() == 32);

    map.clear();
    REQUIRE(map.empty());
    REQUIRE(map.size() == 0);
    REQUIRE(map.capacity() == 32);
    REQUIRE(DtorObj::s_ctor_counter() == 20);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 10);

    HashMap<DtorObj, DtorObj, DtorHash> empty_map{allocator};
    empty_map.clear();
    REQUIRE(empty_map.empty());
    REQUIRE(empty_map.size() == 0);
    REQUIRE(empty_map.capacity() == 0);
}

TEST_CASE("HashMap::remove")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map = init_test_small_map_u32<3>(allocator, 3);
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

    HashMap<uint32_t, uint32_t> empty_map{allocator};
    empty_map.remove(10);
}

TEST_CASE("HashMap::range_for")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map{allocator};
    // Make sure this skips
    for (auto v : map)
        v.second++;

    map = init_test_small_map_u32<5>(allocator, 3);

    for (auto v : map)
        (*v.second)++;

    REQUIRE(map.contains(10));
    REQUIRE(map.contains(20));
    REQUIRE(map.contains(30));

    HashMap<uint32_t, uint32_t> const &map_const = map;
    uint32_t sum = 0;
    for (auto const v : map_const)
        sum += *v.second;
    REQUIRE(sum == 66);
}

TEST_CASE("HashMap::grow_on_assign")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map{allocator, 2};
    // Actual capacity might be rounded up
    size_t const initial_capacity = map.capacity();
    for (size_t i = 0; i < initial_capacity; ++i)
        map.insert_or_assign((uint32_t)i, (uint32_t)i);

    REQUIRE(map.capacity() > initial_capacity);
    REQUIRE(map.size() == initial_capacity);

    for (size_t i = 0; i < initial_capacity; ++i)
    {
        REQUIRE(map.contains(i));
        REQUIRE(*map.find(i) == i);
    }
}

TEST_CASE("HashMap::find_no_slot_empty")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map{allocator};
    // Insert a single 'persitent' value to avoid clearing deleted slots with
    // size == 0
    map.insert_or_assign(0xFFFFFFFF, 0xFFFFFFFF);
    for (uint32_t i = 0; i < 1'000; ++i)
    {
        map.insert_or_assign(i, i);
        map.remove(i);
        REQUIRE(!map.contains(i));
        REQUIRE(map.find(i) == nullptr);
    }
    // Exercise const find
    HashMap<uint32_t, uint32_t> const &const_map = map;
    REQUIRE(!const_map.contains(0));
    REQUIRE(const_map.find(0) == nullptr);

    // Exercise remove with missing element
    map.remove(0xFFFFFFFF - 1);

    // Exercise the clear with size == 0
    map.remove(0xFFFFFFFF);
}

TEST_CASE("HashMap::assign_existing")
{
    CstdlibAllocator allocator;

    init_dtor_counters();
    HashMap<DtorObj, DtorObj, DtorHash> map{allocator};
    // Insert a single 'persitent' value to avoid clear with size == 0
    DtorObj const zero{0};
    DtorObj const one{1};
    DtorObj const two{2};
    map.insert_or_assign(zero, one);
    REQUIRE(DtorObj::s_ctor_counter() == 5);
    REQUIRE(DtorObj::s_value_ctor_counter() == 3);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 2);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(map.size() == 1);
    REQUIRE(map.contains(zero));
    REQUIRE(map.find(zero)->data == 1);

    map.insert_or_assign(zero, two);
    REQUIRE(DtorObj::s_ctor_counter() == 6);
    REQUIRE(DtorObj::s_value_ctor_counter() == 3);
    REQUIRE(DtorObj::s_copy_ctor_counter() == 3);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 1);
    REQUIRE(map.size() == 1);
    REQUIRE(map.contains(zero));
    REQUIRE(map.find(zero)->data == 2);
}

TEST_CASE("HashMap::iterator_const_star")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map{allocator};
    map.insert_or_assign(0xC0FFEEEE, 0xCAFEBABE);

    HashMap<uint32_t, uint32_t>::Iterator iter = map.begin();
    HashMap<uint32_t, uint32_t>::Iterator const &const_iter = iter;
    REQUIRE(const_iter != map.end());
    REQUIRE(*((*const_iter).first) == 0xC0FFEEEE);
    REQUIRE(*((*const_iter).second) == 0xCAFEBABE);
}

TEST_CASE("HashMap::iterator_increment")
{
    CstdlibAllocator allocator;

    HashMap<uint32_t, uint32_t> map{allocator};
    map.insert_or_assign(0xC0FFEEEE, 0xCAFEBABE);
    map.insert_or_assign(0xDEADCAFE, 0x87654321);

    {
        HashMap<uint32_t, uint32_t>::Iterator iter = map.begin();
        HashMap<uint32_t, uint32_t>::Iterator const begin_iter = iter;
        HashMap<uint32_t, uint32_t>::Iterator const increment_iter = ++iter;
        REQUIRE(begin_iter != increment_iter);
        REQUIRE(iter == increment_iter);
    }

    {
        HashMap<uint32_t, uint32_t>::Iterator iter = map.begin();
        HashMap<uint32_t, uint32_t>::Iterator const begin_iter = iter;
        HashMap<uint32_t, uint32_t>::Iterator const increment_iter = iter++;
        REQUIRE(begin_iter == increment_iter);
        REQUIRE(iter != begin_iter);
    }

    HashMap<uint32_t, uint32_t> const &const_map = map;

    {
        HashMap<uint32_t, uint32_t>::ConstIterator iter = const_map.begin();
        HashMap<uint32_t, uint32_t>::ConstIterator const begin_iter = iter;
        HashMap<uint32_t, uint32_t>::ConstIterator const increment_iter =
            ++iter;
        REQUIRE(begin_iter != increment_iter);
        REQUIRE(iter == increment_iter);
    }

    {
        HashMap<uint32_t, uint32_t>::ConstIterator iter = const_map.begin();
        HashMap<uint32_t, uint32_t>::ConstIterator const begin_iter = iter;
        HashMap<uint32_t, uint32_t>::ConstIterator const increment_iter =
            iter++;
        REQUIRE(begin_iter == increment_iter);
        REQUIRE(iter != begin_iter);
    }
}

TEST_CASE("HashMap::aligned")
{
    CstdlibAllocator allocator;

    HashMap<AlignedObj, AlignedObj, AlignedHash> map{allocator};

    map.insert_or_assign(AlignedObj{10}, AlignedObj{11});
    map.insert_or_assign(AlignedObj{20}, AlignedObj{21});

    REQUIRE(map.contains({10}));
    REQUIRE(map.contains({20}));

    uint32_t sum = 0;
    for (auto const v : map)
        sum += v.second->value;
    REQUIRE(sum == 32);
}
