#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/optional.hpp>

#include "common.hpp"

using namespace wheels;

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
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 2);

    {
        init_dtor_counters();
        DtorObj val{2};
        Optional<DtorObj> opt{WHEELS_MOV(val)};
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 1);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2{opt};
        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt2.has_value());
        REQUIRE((*opt2).data == 2);
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 2);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2{WHEELS_MOV(opt)};
        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 2);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt2.has_value());
        REQUIRE((*opt2).data == 2);
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 1);
}

TEST_CASE("Optional::assignments")
{
    {
        init_dtor_counters();
        Optional<DtorObj> opt;
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(!opt.has_value());
        REQUIRE(!opt2.has_value());

        opt2 = opt;
        REQUIRE(DtorObj::s_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(!opt.has_value());
        REQUIRE(!opt2.has_value());

        opt2 = WHEELS_MOV(opt);
        REQUIRE(DtorObj::s_ctor_counter() == 0);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(!opt2.has_value());
    }
    REQUIRE(DtorObj::s_dtor_counter() == 0);

    {
        init_dtor_counters();
        Optional<DtorObj> const opt{DtorObj{2}};
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
        REQUIRE(!opt2.has_value());

        opt2 = opt;

        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_copy_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE((*opt).data == 2);
        REQUIRE(opt->data == 2);
        REQUIRE(opt2.has_value());
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 2);

    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        Optional<DtorObj> opt2;
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE(opt->data == 2);
        REQUIRE(!opt2.has_value());

        opt2 = WHEELS_MOV(opt);

        REQUIRE(DtorObj::s_ctor_counter() == 3);
        REQUIRE(DtorObj::s_move_ctor_counter() == 2);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt2.has_value());
        REQUIRE(opt2->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 1);
}

TEST_CASE("Optional::aligned")
{
    AlignedObj const value{2};
    Optional<AlignedObj> map{value};
    REQUIRE(map.has_value());
    REQUIRE(*map == value);
    REQUIRE(map->value == value.value);
}

TEST_CASE("Optional::swap")
{
    init_dtor_counters();
    Optional<DtorObj> opt;
    {
        const Optional<DtorObj> prev = opt.swap(DtorObj{2});
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE(opt->data == 2);
        REQUIRE(!prev.has_value());
    }
    {
        const Optional<DtorObj> prev = opt.swap(DtorObj{3});
        REQUIRE(DtorObj::s_ctor_counter() == 6);
        REQUIRE(DtorObj::s_value_ctor_counter() == 2);
        REQUIRE(DtorObj::s_move_ctor_counter() == 4);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());
        REQUIRE(opt->data == 3);
        REQUIRE(prev.has_value());
        REQUIRE(prev->data == 2);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 1);
}

TEST_CASE("Optional::take")
{
    init_dtor_counters();
    Optional<DtorObj> opt{DtorObj{2}};
    REQUIRE(DtorObj::s_ctor_counter() == 2);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_move_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    DtorObj obj{opt.take()};
    REQUIRE(DtorObj::s_ctor_counter() == 3);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_move_ctor_counter() == 2);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(!opt.has_value());
    REQUIRE(obj.data == 2);
}

TEST_CASE("Optional::emplace")
{
    init_dtor_counters();
    Optional<DtorObj> opt;
    opt.emplace(2u);
    REQUIRE(DtorObj::s_ctor_counter() == 1);
    REQUIRE(DtorObj::s_value_ctor_counter() == 1);
    REQUIRE(DtorObj::s_assign_counter() == 0);
    REQUIRE(DtorObj::s_dtor_counter() == 0);
    REQUIRE(opt.has_value());
    REQUIRE(opt->data == 2);
}

TEST_CASE("Optional::reset")
{
    {
        init_dtor_counters();
        Optional<DtorObj> opt{DtorObj{2}};
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_value_ctor_counter() == 1);
        REQUIRE(DtorObj::s_move_ctor_counter() == 1);
        REQUIRE(DtorObj::s_assign_counter() == 0);
        REQUIRE(DtorObj::s_dtor_counter() == 0);
        REQUIRE(opt.has_value());

        opt.reset();
        REQUIRE(!opt.has_value());
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_dtor_counter() == 1);

        opt.reset();
        REQUIRE(!opt.has_value());
        REQUIRE(DtorObj::s_ctor_counter() == 2);
        REQUIRE(DtorObj::s_dtor_counter() == 1);
    }
    REQUIRE(DtorObj::s_dtor_counter() == 1);
}

TEST_CASE("Optional::comparisons")
{
    Optional<uint32_t> empty;
    Optional<uint32_t> empty2;
    Optional<uint32_t> one{1};
    Optional<uint32_t> one2{1};
    Optional<uint32_t> two{2};

    REQUIRE(empty == empty);
    REQUIRE(empty == empty2);
    REQUIRE(!(one == empty));
    REQUIRE(one == one);
    REQUIRE(one == one2);
    REQUIRE(one2 == one);
    REQUIRE(!(one == two));
    REQUIRE(!(two == one));

    REQUIRE(!(empty != empty));
    REQUIRE(!(empty != empty2));
    REQUIRE(one != empty);
    REQUIRE(!(one != one));
    REQUIRE(!(one != one2));
    REQUIRE(!(one2 != one));
    REQUIRE(one != two);
    REQUIRE(two != one);
}
