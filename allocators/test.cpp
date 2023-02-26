#include <catch2/catch_test_macros.hpp>

#include "cstdlib_allocator.hpp"
#include "linear_allocator.hpp"
#include "scoped_scratch.hpp"
#include "utils.hpp"

using namespace wheels;

namespace
{

struct alignas(std::max_align_t) AlignedObj
{
    uint32_t value{0};
    uint8_t _padding[alignof(std::max_align_t) - sizeof(uint32_t)];
};
static_assert(alignof(AlignedObj) > alignof(uint32_t));

} // namespace

TEST_CASE("aligned_offset", "[test]")
{
    REQUIRE(aligned_offset(0, 8) == 0);
    REQUIRE(aligned_offset(1, 8) == 8);
    REQUIRE(aligned_offset(4, 8) == 8);
    REQUIRE(aligned_offset(8, 8) == 8);
}

TEST_CASE("LinearAllocator", "[test]")
{
    LinearAllocator allocator{4096};
    {
        uint8_t *alloc = (uint8_t *)allocator.allocate(2048);
        REQUIRE(alloc != nullptr);
        memset(alloc, 0, 2048);
        alloc[0] = 0x12;
        alloc[2047] = 0x23;
        assert(alloc[0] == 0x12);
        assert(alloc[2047] == 0x23);
        REQUIRE(allocator.allocate(2048) != nullptr);
        REQUIRE(allocator.allocate(1) == nullptr);
    }
    allocator.reset();
    {
        void *alloc0 = allocator.allocate(2048);
        REQUIRE(alloc0 != nullptr);
        allocator.deallocate(alloc0);
        void *alloc1 = allocator.allocate(2048);
        REQUIRE(allocator.allocate(1) == nullptr);
        allocator.rewind(alloc1);
        REQUIRE(allocator.allocate(2048) == alloc1);
        allocator.rewind(alloc0);
        REQUIRE(allocator.allocate(4096) != nullptr);
    }
}

TEST_CASE("LinearAllocator::aligned_PoD", "[test]")
{
    LinearAllocator allocator{4096};

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
}

TEST_CASE("ScopedScratch::scalar_types", "[test]")
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

struct Float4
{
    float data[4];
};

TEST_CASE("ScopedScratch::PoD", "[test]")
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

TEST_CASE("ScopedScratch::aligned_PoD", "[test]")
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

class Obj
{
  public:
    static uint64_t s_dtor_counter;

    Obj(){};
    ~Obj()
    {
        s_dtor_counter++;
        data = 0;
    };

    uint64_t data{0};
};
uint64_t Obj::s_dtor_counter = 0;

TEST_CASE("ScopedScratch::dtor", "[test]")
{
    LinearAllocator allocator{4096};
    Obj::s_dtor_counter = 0;

    {
        ScopedScratch scratch{allocator};

        Obj *obj = scratch.allocate_object<Obj>();
        REQUIRE(obj != nullptr);
        obj->data = 0xDEADCAFEBEEFBABE;
        REQUIRE(obj->data == 0xDEADCAFEBEEFBABE);

        REQUIRE(Obj::s_dtor_counter == 0);
    }
    REQUIRE(Obj::s_dtor_counter == 1);
}

TEST_CASE("ScopedScratch::child_scopes", "[test]")
{
    LinearAllocator allocator{4096};
    Obj::s_dtor_counter = 0;

    {
        ScopedScratch scratch{allocator};

        REQUIRE(scratch.allocate_object<Obj>() != nullptr);
        {
            ScopedScratch child1 = scratch.child_scope();
            REQUIRE(child1.allocate_object<Obj>() != nullptr);
            {
                ScopedScratch child2 = child1.child_scope();
                REQUIRE(child2.allocate_object<Obj>() != nullptr);
                REQUIRE(Obj::s_dtor_counter == 0);
            }
            REQUIRE(Obj::s_dtor_counter == 1);
        }
        REQUIRE(Obj::s_dtor_counter == 2);
    }
    REQUIRE(Obj::s_dtor_counter == 3);
}

TEST_CASE("CstdlibAllocator", "[test]")
{
    CstdlibAllocator allocator;

    uint8_t *alloc = (uint8_t *)allocator.allocate(2048);
    REQUIRE(alloc != nullptr);
    memset(alloc, 0, 2048);
    alloc[0] = 0x12;
    alloc[2047] = 0x23;
    assert(alloc[0] == 0x12);
    assert(alloc[2047] == 0x23);
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