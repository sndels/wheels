#ifndef WHEELS_CONTAINERS_HASH_HPP
#define WHEELS_CONTAINERS_HASH_HPP

#include <functional>
#include <wyhash.h>

namespace wheels
{

template <typename T> struct Hash
{
    /// Delete implementation for types that don't specifically override with a
    /// valid hasher implementation
    [[nodiscard]] uint64_t operator()(T const &value) const noexcept = delete;
};

template <typename T> struct Hash<T *>
{
    [[nodiscard]] uint64_t operator()(T const *ptr) const noexcept
    {
        return wyhash(&ptr, sizeof(ptr), 0, _wyp);
    }
};

#define WHEELS_HASH_DEFINE_IMPLEMENTATION(T)                                   \
    template <> struct Hash<T>                                                 \
    {                                                                          \
        [[nodiscard]] uint64_t operator()(T const &value) const noexcept       \
        {                                                                      \
            return wyhash(&value, sizeof(value), 0, _wyp);                     \
        }                                                                      \
    }

// TODO: Faster small value hashes? Only casting seems bad as these might get
// used with strided values
WHEELS_HASH_DEFINE_IMPLEMENTATION(int8_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(uint8_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(int16_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(uint16_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(int32_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(uint32_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(int64_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(uint64_t);
WHEELS_HASH_DEFINE_IMPLEMENTATION(float);
WHEELS_HASH_DEFINE_IMPLEMENTATION(double);

#undef WHEELS_HASH_DEFINE_IMPLEMENTATION

} // namespace wheels

#endif // WHEELS_CONTAINERS_HASH_HPP
