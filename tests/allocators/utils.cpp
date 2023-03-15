#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/utils.hpp>

using namespace wheels;

TEST_CASE("aligned_offset")
{
    REQUIRE(aligned_offset(0, 8) == 0);
    REQUIRE(aligned_offset(1, 8) == 8);
    REQUIRE(aligned_offset(4, 8) == 8);
    REQUIRE(aligned_offset(8, 8) == 8);
}