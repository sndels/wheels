#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>

#include "common.hpp"

using namespace wheels;

TEST_CASE("CstdlibAllocator")
{
    CstdlibAllocator allocator;

    uint8_t *alloc = (uint8_t *)allocator.allocate(2048);
    REQUIRE(alloc != nullptr);
    memset(alloc, 0, 2048);
    alloc[0] = 0x12;
    alloc[2047] = 0x23;
    REQUIRE(alloc[0] == 0x12);
    REQUIRE(alloc[2047] == 0x23);
    allocator.deallocate(alloc);

    AlignedObj *aligned_alloc0 =
        (AlignedObj *)allocator.allocate(sizeof(AlignedObj));
    uint8_t *u8_alloc = (uint8_t *)allocator.allocate(sizeof(uint8_t));
    AlignedObj *aligned_alloc1 =
        (AlignedObj *)allocator.allocate(sizeof(AlignedObj));
    REQUIRE(aligned_alloc0 != nullptr);
    REQUIRE(u8_alloc != nullptr);
    REQUIRE(aligned_alloc1 != nullptr);
    REQUIRE((std::uintptr_t)aligned_alloc0 % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)aligned_alloc1 % alignof(AlignedObj) == 0);

    allocator.deallocate(aligned_alloc1);
    allocator.deallocate(u8_alloc);
    allocator.deallocate(aligned_alloc0);
}
