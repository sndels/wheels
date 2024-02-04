#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/tlsf_allocator.hpp>
#include <wheels/owning_ptr.hpp>

#include "containers/common.hpp"

using namespace wheels;

TEST_CASE("OwningPtr")
{
    TlsfAllocator alloc{kilobytes(2)};
    { // Operations on nullptr
        OwningPtr<uint32_t> ptr;
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(ptr == nullptr);
        REQUIRE(nullptr == ptr);
        ptr.reset();
        OwningPtr<uint32_t> other;
        ptr.swap(other);
        OwningPtr<uint32_t> const &const_ref = ptr;
        REQUIRE(const_ref.get() == nullptr);
        REQUIRE(nullptr == const_ref);
    }

    { // Move ctor on nullptr
        OwningPtr<uint32_t> ptr;
        OwningPtr<uint32_t> const ptr2{WHEELS_MOV(ptr)};
        REQUIRE(nullptr == ptr.get());
    }

    { // Operations on allocated ptr
        init_dtor_counters();

        // First allocation
        OwningPtr<DtorObj> ptr{alloc, 2u};
        REQUIRE(alloc.stats().allocation_count == 1);
        REQUIRE(DtorObj::s_ctor_counter() == 1);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr != nullptr);
        REQUIRE(nullptr != ptr);
        REQUIRE((*ptr).data == 2);
        REQUIRE(ptr->data == 2);

        OwningPtr<DtorObj> const &const_ref = ptr;
        REQUIRE(const_ref.get() != nullptr);
        REQUIRE(const_ref != nullptr);
        REQUIRE(nullptr != const_ref);
        REQUIRE((*const_ref).data == 2);
        REQUIRE(const_ref->data == 2);

        // Re-assignment
        ptr = OwningPtr<DtorObj>{alloc, 3u};
        REQUIRE(alloc.stats().allocation_count == 1);
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 2);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 1);
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr != nullptr);
        REQUIRE((*ptr).data == 3);
        REQUIRE(ptr->data == 3);

        // Move ctor and swap
        OwningPtr<DtorObj> tmpPtr{alloc, 4u};
        OwningPtr<DtorObj> ptr2{WHEELS_MOV(tmpPtr)};
        REQUIRE(alloc.stats().allocation_count == 2);
        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_value_ctor_counter() == 3);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 1);
        REQUIRE(ptr2.get() != nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE((*ptr2).data == 4);
        REQUIRE(ptr2->data == 4);
        ptr.swap(ptr2);
        REQUIRE(alloc.stats().allocation_count == 2);
        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_value_ctor_counter() == 3);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 1);
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr != nullptr);
        REQUIRE((*ptr).data == 4);
        REQUIRE(ptr->data == 4);
        REQUIRE(ptr2.get() != nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE((*ptr2).data == 3);
        REQUIRE(ptr2->data == 3);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 3);
    REQUIRE(alloc.stats().allocation_count == 0);
}
