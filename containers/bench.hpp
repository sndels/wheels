#ifndef CONTAINERS_BENCH_HPP
#define CONTAINERS_BENCH_HPP

// This is not really a header but included into the main bench file unity build

#include <benchmark/benchmark.h>

#include "allocators/cstdlib_allocator.hpp"
#include "containers/array.hpp"
#include "containers/pair.hpp"
#include "containers/small_map.hpp"
#include "containers/small_set.hpp"
#include "containers/static_array.hpp"

#include <cstdlib>
#include <unordered_set>

using namespace wheels;

#ifdef _MSC_VER
#define INLINE_ASM(...)
#else
#define INLINE_ASM(...) asm(__VA_ARGS__)
#endif // __MSVC

namespace
{

uint32_t s_dtor_counter = 0;

class DtorObj
{
  public:
    DtorObj(){};

    DtorObj(uint32_t data)
    : data{data} {};

    ~DtorObj() { s_dtor_counter++; };

    DtorObj(DtorObj const &other)
    : data{other.data}
    {
    }

    DtorObj(DtorObj &&other)
    : data{other.data}
    {
    }

    DtorObj &operator=(DtorObj const &other)
    {
        if (this != &other)
            data = other.data;
        return *this;
    }

    DtorObj &operator=(DtorObj &&other)
    {
        if (this != &other)
            data = other.data;
        return *this;
    }

    uint64_t data{0};
    uint64_t padding[7];
};

template <typename Arr>
void emplace_back_clear_loop(
    benchmark::State &state, Arr &arr, uint32_t object_count)
{
    while (state.KeepRunning())
    {
        benchmark::DoNotOptimize(arr.data());
        INLINE_ASM("nop # Start loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        for (uint32_t i = 0; i < object_count; ++i)
            arr.emplace_back(i);
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End loop");
        benchmark::ClobberMemory();
        INLINE_ASM("nop # Start clear");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        arr.clear();
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End clear");
        benchmark::ClobberMemory();
    }
}

template <typename Arr>
void sum_loop(benchmark::State &state, Arr &arr, uint32_t object_count)
{
    for (uint32_t i = 0; i < object_count; ++i)
        arr.emplace_back(i);

    while (state.KeepRunning())
    {
        INLINE_ASM("nop # Start bench loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        uint32_t sum = 0;
        for (uint32_t v : arr)
            sum += v;
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End bench loop");
        benchmark::DoNotOptimize(sum);
    }
}

} // namespace

static void empty_std_vector_push_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    while (state.KeepRunning())
    {
        INLINE_ASM("nop # Create");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        std::vector<uint32_t> vec;
        INLINE_ASM("nop # Start loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        for (uint32_t i = 0; i < object_count; ++i)
            vec.push_back(i);
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End loop");
    }
}
BENCHMARK(empty_std_vector_push_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

static void empty_array_push_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    while (state.KeepRunning())
    {
        INLINE_ASM("nop # Create");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        Array<uint32_t> arr{allocator, 0};
        INLINE_ASM("nop # Start loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        for (uint32_t i = 0; i < object_count; ++i)
            arr.push_back(i);
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End loop");
    }
}
BENCHMARK(empty_array_push_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

static void reserved_std_vector_push_clear_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    std::vector<uint32_t> vec;
    vec.reserve(object_count);

    emplace_back_clear_loop(state, vec, object_count);
}
BENCHMARK(reserved_std_vector_push_clear_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

static void reserved_array_push_clear_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    Array<uint32_t> arr{allocator, object_count};

    emplace_back_clear_loop(state, arr, object_count);
}
BENCHMARK(reserved_array_push_clear_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

template <uint32_t N>
static void static_array_push_clear_uint32_t(benchmark::State &state)
{
    StaticArray<uint32_t, N> arr;

    emplace_back_clear_loop(state, arr, arr.capacity());
}
BENCHMARK(static_array_push_clear_uint32_t<32>);
BENCHMARK(static_array_push_clear_uint32_t<128>);
BENCHMARK(static_array_push_clear_uint32_t<512>);
BENCHMARK(static_array_push_clear_uint32_t<2048>);
BENCHMARK(static_array_push_clear_uint32_t<8096>);

static void reserved_std_vector_push_clear_obj(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    std::vector<DtorObj> vec;
    vec.reserve(object_count);

    emplace_back_clear_loop(state, vec, object_count);
}
BENCHMARK(reserved_std_vector_push_clear_obj)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

static void reserved_array_push_clear_obj(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    Array<DtorObj> arr{allocator, object_count};

    emplace_back_clear_loop(state, arr, object_count);
}
BENCHMARK(reserved_array_push_clear_obj)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

template <uint32_t N>
static void static_array_push_clear_obj(benchmark::State &state)
{
    StaticArray<DtorObj, N> arr;

    emplace_back_clear_loop(state, arr, arr.capacity());
}
BENCHMARK(static_array_push_clear_obj<32>);
BENCHMARK(static_array_push_clear_obj<128>);
BENCHMARK(static_array_push_clear_obj<512>);
BENCHMARK(static_array_push_clear_obj<2048>);
BENCHMARK(static_array_push_clear_obj<8096>);

static void std_vector_sum_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    std::vector<uint32_t> vec;
    vec.reserve(object_count);

    sum_loop(state, vec, object_count);
}
BENCHMARK(std_vector_sum_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

static void array_sum_uint32_t(benchmark::State &state)
{
    CstdlibAllocator allocator;

    uint32_t const object_count = (uint32_t)state.range_x();

    Array<uint32_t> arr{allocator, object_count};

    sum_loop(state, arr, object_count);
}
BENCHMARK(array_sum_uint32_t)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8096);

template <uint32_t N>
static void static_array_sum_uint32_t(benchmark::State &state)
{
    StaticArray<uint32_t, N> arr;

    sum_loop(state, arr, arr.capacity());
}
BENCHMARK(static_array_sum_uint32_t<32>);
BENCHMARK(static_array_sum_uint32_t<128>);
BENCHMARK(static_array_sum_uint32_t<512>);
BENCHMARK(static_array_sum_uint32_t<2048>);
BENCHMARK(static_array_sum_uint32_t<8096>);

template <uint32_t N>
static void unordered_set_insert_uint32_t(benchmark::State &state)
{

    while (state.KeepRunning())
    {
        std::unordered_set<uint32_t> set;
        INLINE_ASM("nop # Start loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        for (uint32_t i = 0; i < N; ++i)
            set.insert(i);
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End loop");
    }
}
BENCHMARK(unordered_set_insert_uint32_t<2>);
BENCHMARK(unordered_set_insert_uint32_t<4>);
BENCHMARK(unordered_set_insert_uint32_t<8>);
BENCHMARK(unordered_set_insert_uint32_t<16>);
BENCHMARK(unordered_set_insert_uint32_t<32>);

template <uint32_t N>
static void small_set_insert_uint32_t(benchmark::State &state)
{

    while (state.KeepRunning())
    {
        SmallSet<uint32_t, N> set;
        INLINE_ASM("nop # Start loop");
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        for (uint32_t i = 0; i < N; ++i)
            set.insert(i);
        INLINE_ASM("nop");
        INLINE_ASM("nop");
        INLINE_ASM("nop # End loop");
    }
}
BENCHMARK(small_set_insert_uint32_t<2>);
BENCHMARK(small_set_insert_uint32_t<4>);
BENCHMARK(small_set_insert_uint32_t<8>);
BENCHMARK(small_set_insert_uint32_t<16>);
BENCHMARK(small_set_insert_uint32_t<32>);

template <uint32_t N>
static void unordered_set_contains_uint32_t(benchmark::State &state)
{
    std::unordered_set<uint32_t> set;
    for (uint32_t i = 0; i < N; ++i)
        set.insert(i);

    bool contained = false;
    while (state.KeepRunning())
        benchmark::DoNotOptimize(set.contains(rand() % N));
}
BENCHMARK(unordered_set_contains_uint32_t<2>);
BENCHMARK(unordered_set_contains_uint32_t<4>);
BENCHMARK(unordered_set_contains_uint32_t<8>);
BENCHMARK(unordered_set_contains_uint32_t<16>);
BENCHMARK(unordered_set_contains_uint32_t<32>);

template <uint32_t N>
static void small_set_contains_uint32_t(benchmark::State &state)
{
    SmallSet<uint32_t, N> set;
    for (uint32_t i = 0; i < N; ++i)
        set.insert(i);

    while (state.KeepRunning())
        benchmark::DoNotOptimize(set.contains(rand() % N));
}
BENCHMARK(small_set_contains_uint32_t<2>);
BENCHMARK(small_set_contains_uint32_t<4>);
BENCHMARK(small_set_contains_uint32_t<8>);
BENCHMARK(small_set_contains_uint32_t<16>);
BENCHMARK(small_set_contains_uint32_t<32>);

template <uint32_t N>
static void unordered_set_doesnt_contain_uint32_t(benchmark::State &state)
{
    std::unordered_set<uint32_t> set;
    for (uint32_t i = 0; i < N; ++i)
        set.insert(i);

    bool contained = false;
    while (state.KeepRunning())
        benchmark::DoNotOptimize(set.contains((rand() % N) + N));
}
BENCHMARK(unordered_set_doesnt_contain_uint32_t<2>);
BENCHMARK(unordered_set_doesnt_contain_uint32_t<4>);
BENCHMARK(unordered_set_doesnt_contain_uint32_t<8>);
BENCHMARK(unordered_set_doesnt_contain_uint32_t<16>);
BENCHMARK(unordered_set_doesnt_contain_uint32_t<32>);

template <uint32_t N>
static void small_set_doesnt_contain_uint32_t(benchmark::State &state)
{
    SmallSet<uint32_t, N> set;
    for (uint32_t i = 0; i < N; ++i)
        set.insert(i);

    while (state.KeepRunning())
        benchmark::DoNotOptimize(set.contains((rand() % N) + N));
}
BENCHMARK(small_set_doesnt_contain_uint32_t<2>);
BENCHMARK(small_set_doesnt_contain_uint32_t<4>);
BENCHMARK(small_set_doesnt_contain_uint32_t<8>);
BENCHMARK(small_set_doesnt_contain_uint32_t<16>);
BENCHMARK(small_set_doesnt_contain_uint32_t<32>);

template <typename T> static void std_hash(benchmark::State &state)
{
    std::hash<T> hash;
    T value = 0;
    while (state.KeepRunning())
        benchmark::DoNotOptimize(hash(value++));
}
BENCHMARK(std_hash<uint8_t>);
BENCHMARK(std_hash<uint16_t>);
BENCHMARK(std_hash<uint32_t>);
BENCHMARK(std_hash<uint64_t>);
BENCHMARK(std_hash<float>);
BENCHMARK(std_hash<double>);
BENCHMARK(std_hash<uint64_t *>);
BENCHMARK(std_hash<uint64_t const *>);

template <typename T> static void wheels_hash(benchmark::State &state)
{
    Hash<T> hash;
    T value = 0;
    while (state.KeepRunning())
        benchmark::DoNotOptimize(hash(value++));
}
BENCHMARK(wheels_hash<uint8_t>);
BENCHMARK(wheels_hash<uint16_t>);
BENCHMARK(wheels_hash<uint32_t>);
BENCHMARK(wheels_hash<uint64_t>);
BENCHMARK(std_hash<float>);
BENCHMARK(std_hash<double>);
BENCHMARK(wheels_hash<uint64_t *>);
BENCHMARK(wheels_hash<uint64_t const *>);

#endif // CONTAINERS_BENCH_HPP