#include <catch2/catch_test_macros.hpp>

#include <wheels/containers/hash.hpp>

using namespace wheels;

TEST_CASE("hash::pointers", "[hash]")
{
    {
        Hash<uint64_t *> hasher;
        uint64_t zero = 0;
        uint64_t const const_zero = 0;
        REQUIRE(hasher(&zero) == hasher(&zero));
        REQUIRE(hasher(&const_zero) == hasher(&const_zero));
    }

    {
        struct CustomT
        {
            uint8_t value;
        };
        Hash<CustomT *> hasher;
        CustomT zero = {};
        CustomT const const_zero = {};
        REQUIRE(hasher(&zero) == hasher(&zero));
        REQUIRE(hasher(&const_zero) == hasher(&const_zero));
    }
}

#define STR2(v) #v
#define STR(v) STR2(v)
#define HASH_TEST_IMPLEMENTATION(T)                                            \
    TEST_CASE("hash::" STR(T), "[hash]")                                       \
    {                                                                          \
        Hash<T> hasher;                                                        \
        T zero = static_cast<T>(0);                                            \
        T const const_zero = static_cast<T>(0);                                \
        REQUIRE(hasher(zero) == hasher(zero));                                 \
        REQUIRE(hasher(const_zero) == hasher(const_zero));                     \
    }

HASH_TEST_IMPLEMENTATION(int8_t);
HASH_TEST_IMPLEMENTATION(uint8_t);
HASH_TEST_IMPLEMENTATION(int16_t);
HASH_TEST_IMPLEMENTATION(uint16_t);
HASH_TEST_IMPLEMENTATION(int32_t);
HASH_TEST_IMPLEMENTATION(uint32_t);
HASH_TEST_IMPLEMENTATION(int64_t);
HASH_TEST_IMPLEMENTATION(uint64_t);
HASH_TEST_IMPLEMENTATION(float);
HASH_TEST_IMPLEMENTATION(double);

#undef HASH_TEST_IMPLEMENTATIOn
#undef STR
#undef STR2
