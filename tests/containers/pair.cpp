#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/pair.hpp>

#include "common.hpp"

using namespace wheels;

TEST_CASE("Pair")
{
    uint32_t deadcafe = 0xDEADCAFE;
    uint32_t coffee = 0xC0FFEEEE;
    uint16_t onetwothreefour = 0x1234;
    uint16_t abcd = 0xABCD;
    Pair<uint32_t, uint16_t> p0{deadcafe, onetwothreefour};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
    Pair<uint32_t, uint16_t> p1{0xDEADCAFE, std::move(onetwothreefour)};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
    Pair<uint32_t, uint16_t> p2{std::move(deadcafe), abcd};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
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
