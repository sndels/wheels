#include <catch2/catch_test_macros.hpp>

#define WHEELS_SCOPED_SCRATCH_TESTS_INTERNAL
#include <wheels/allocators/scoped_scratch.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

struct Float4
{
    float data[4];
};

class Obj
{
  public:
    static uint64_t &s_dtor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }

    Obj(){};
    Obj(uint64_t data)
    : data{data} {};
    ~Obj()
    {
        s_dtor_counter()++;
        data = 0;
    };

    uint64_t data{0};
};

} // namespace

TEST_CASE("ScopedScratch::scalar_types")
{
    LinearAllocator allocator{4096};
    {
        ScopedScratch scratch{allocator};

        uint8_t *u8_alloc = scratch.allocate_pod<uint8_t>();
        REQUIRE(u8_alloc != nullptr);
        *u8_alloc = 0xAB;

        uint16_t *u16_alloc = scratch.allocate_pod<uint16_t>();
        REQUIRE(u16_alloc != nullptr);
        *u16_alloc = 0x1234;

        uint32_t *u32_alloc = scratch.allocate_pod<uint32_t>();
        REQUIRE(u32_alloc != nullptr);
        *u32_alloc = 0xC0FFEEEE;

        uint64_t *u64_alloc = scratch.allocate_pod<uint64_t>();
        REQUIRE(u64_alloc != nullptr);
        *u64_alloc = 0xDEADCAFEBEEFBABE;

        REQUIRE(*u8_alloc == 0xAB);
        REQUIRE(*u16_alloc == 0x1234);
        REQUIRE(*u32_alloc == 0xC0FFEEEE);
        REQUIRE(*u64_alloc == 0xDEADCAFEBEEFBABE);
    }
}

TEST_CASE("ScopedScratch::PoD")
{
    LinearAllocator allocator{4096};
    {
        ScopedScratch scratch{allocator};

        Float4 *float4_alloc = scratch.allocate_pod<Float4>();
        REQUIRE(float4_alloc != nullptr);
        float4_alloc->data[0] = 1.f;
        float4_alloc->data[1] = 2.f;
        float4_alloc->data[2] = 3.f;
        float4_alloc->data[3] = 4.f;

        REQUIRE(float4_alloc->data[0] == 1.f);
        REQUIRE(float4_alloc->data[1] == 2.f);
        REQUIRE(float4_alloc->data[2] == 3.f);
        REQUIRE(float4_alloc->data[3] == 4.f);
    }
}

TEST_CASE("ScopedScratch::aligned_PoD")
{
    LinearAllocator allocator{4096};
    {
        ScopedScratch scratch{allocator};

        AlignedObj *aligned_alloc0 = scratch.allocate_pod<AlignedObj>();
        uint8_t *u8_alloc = scratch.allocate_pod<uint8_t>();
        AlignedObj *aligned_alloc1 = scratch.allocate_pod<AlignedObj>();
        REQUIRE(aligned_alloc0 != nullptr);
        REQUIRE(u8_alloc != nullptr);
        REQUIRE(aligned_alloc1 != nullptr);
        REQUIRE((std::uintptr_t)aligned_alloc0 % alignof(AlignedObj) == 0);
        REQUIRE((std::uintptr_t)aligned_alloc1 % alignof(AlignedObj) == 0);
    }
}

TEST_CASE("ScopedScratch::dtor")
{
    LinearAllocator allocator{4096};
    Obj::s_dtor_counter() = 0;

    {
        ScopedScratch scratch{allocator};

        {
            Obj *obj = scratch.allocate_object<Obj>();
            REQUIRE(obj != nullptr);
            REQUIRE(obj->data == 0);
        }

        {
            Obj *obj = scratch.allocate_object<Obj>(0xDEADCAFEBEEFBABE);
            REQUIRE(obj != nullptr);
            REQUIRE(obj->data == 0xDEADCAFEBEEFBABE);
        }

        REQUIRE(Obj::s_dtor_counter() == 0);
    }
    REQUIRE(Obj::s_dtor_counter() == 2);
}

TEST_CASE("ScopedScratch::child_scopes")
{
    LinearAllocator allocator{4096};
    Obj::s_dtor_counter() = 0;

    {
        ScopedScratch scratch{allocator};

        REQUIRE(scratch.allocate_object<Obj>() != nullptr);
        {
            ScopedScratch child1 = scratch.child_scope();
            REQUIRE(child1.allocate_object<Obj>() != nullptr);
            {
                ScopedScratch child2 = child1.child_scope();
                REQUIRE(child2.allocate_object<Obj>() != nullptr);
                REQUIRE(Obj::s_dtor_counter() == 0);
            }
            REQUIRE(Obj::s_dtor_counter() == 1);
        }
        REQUIRE(Obj::s_dtor_counter() == 2);
    }
    REQUIRE(Obj::s_dtor_counter() == 3);
}

TEST_CASE("ScopedScratch::allocate")
{
    LinearAllocator allocator{4096};
    ScopedScratch scratch{allocator};

    uint8_t *alloc = (uint8_t *)scratch.allocate(2048);
    REQUIRE(alloc != nullptr);
    memset(alloc, 0, 2048);
    alloc[0] = 0x12;
    alloc[2047] = 0x23;
    assert(alloc[0] == 0x12);
    assert(alloc[2047] == 0x23);
    REQUIRE(scratch.allocate(2048) != nullptr);
    REQUIRE(scratch.allocate(1) == nullptr);
}

TEST_CASE("ScopedScratch::allocate_aligned_PoD")
{
    LinearAllocator allocator{4096};
    ScopedScratch scratch{allocator};

    AlignedObj *aligned_alloc0 =
        (AlignedObj *)scratch.allocate(sizeof(AlignedObj));
    uint8_t *u8_alloc = (uint8_t *)allocator.allocate(sizeof(uint8_t));
    AlignedObj *aligned_alloc1 =
        (AlignedObj *)scratch.allocate(sizeof(AlignedObj));
    REQUIRE(aligned_alloc0 != nullptr);
    REQUIRE(u8_alloc != nullptr);
    REQUIRE(aligned_alloc1 != nullptr);
    REQUIRE((std::uintptr_t)aligned_alloc0 % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)aligned_alloc1 % alignof(AlignedObj) == 0);
}

TEST_CASE("ScopedScratch::rewind_behavior")
{
    LinearAllocator allocator{4096};
    ScopedScratch scratch{allocator};

    uint8_t *alloc = (uint8_t *)scratch.allocate(2048);
    REQUIRE(alloc != nullptr);

    uint8_t *scratch_peek_before_child = (uint8_t *)scratch.peek();
    REQUIRE(scratch_peek_before_child == alloc + 2048);
    { // Child from a full allocator should return nullptrs
        ScopedScratch child = scratch.child_scope();
        uint8_t *child_alloc = (uint8_t *)child.allocate(20);
        REQUIRE(child_alloc != nullptr);
    }
    REQUIRE(scratch_peek_before_child == (uint8_t *)scratch.peek());
}
