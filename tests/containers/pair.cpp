#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/pair.hpp>

#include "common.hpp"

using namespace wheels;

namespace
{

constexpr uint32_t test_constexpr()
{
    uint32_t zero = 0;
    uint32_t one = 1;
    uint16_t two = 2;
    uint16_t three = 3;
    Pair<uint32_t, uint16_t> p0{zero, two};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
    Pair<uint32_t, uint16_t> p1{0u, std::move(two)};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
    Pair<uint32_t, uint16_t> p2{std::move(zero), three};
    // This should still typecheck rvalue interface?
    // NOLINTNEXTLINE(performance-move-const-arg)
    Pair<uint32_t, uint16_t> p3{std::move(one), std::move(three)};
    Pair<uint32_t, uint16_t> p4 = make_pair((uint32_t)3, (uint16_t)1);

    return (uint32_t)(p0 == p1) + (uint32_t)(p0 != p2) + (uint32_t)(p0 != p3) +
           (uint32_t)(p0 != p4);
}

} // namespace

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

TEST_CASE("constexpr Pair")
{
    constexpr uint32_t result = test_constexpr();
    REQUIRE(result == 4);
}
