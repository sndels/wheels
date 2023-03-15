#ifndef WHEELS_TESTS_CONTAINERS_COMMON_HPP
#define WHEELS_TESTS_CONTAINERS_COMMON_HPP

#include <cstddef>
#include <cstdint>

#include <wyhash.h>

struct alignas(std::max_align_t) AlignedObj
{
    uint32_t value{0};
    uint8_t _padding[alignof(std::max_align_t) - sizeof(uint32_t)];
};
static_assert(alignof(AlignedObj) > alignof(uint32_t));

inline bool operator==(AlignedObj const &lhs, AlignedObj const &rhs)
{
    return lhs.value == rhs.value;
}

struct AlignedHash
{
    /// Delete implementation for types that don't specifically override with a
    /// valid hasher implementation
    uint64_t operator()(AlignedObj const &value) const noexcept
    {
        return wyhash(&value.value, sizeof(value.value), 0, _wyp);
    }
};

class DtorObj
{
  public:
    static uint64_t const s_null_value = (uint64_t)-1;
    static uint64_t &s_ctor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_default_ctor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_value_ctor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_copy_ctor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_move_ctor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_assign_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }
    static uint64_t &s_dtor_counter()
    {
        static uint64_t counter = 0;
        return counter;
    }

    DtorObj()
    {
        s_ctor_counter()++;
        s_default_ctor_counter()++;
    };

    DtorObj(uint32_t data)
    : data{data}
    {
        s_ctor_counter()++;
        s_value_ctor_counter()++;
    };

    ~DtorObj()
    {
        if (data < s_null_value)
        {
            s_dtor_counter()++;
            data = s_null_value;
        }
    };

    DtorObj(DtorObj const &other)
    : data{other.data}
    {
        s_ctor_counter()++;
        s_copy_ctor_counter()++;
    }

    DtorObj(DtorObj &&other)
    : data{other.data}
    {
        s_ctor_counter()++;
        s_move_ctor_counter()++;
        other.data = s_null_value;
    }

    DtorObj &operator=(DtorObj const &other)
    {
        if (this != &other)
        {
            data = other.data;
            s_assign_counter()++;
        }
        return *this;
    }

    DtorObj &operator=(DtorObj &&other)
    {
        if (this != &other)
        {
            data = other.data;
            other.data = s_null_value;
            s_assign_counter()++;
        }
        return *this;
    }

    // -1 means the value has been moved or destroyed, which skips
    // dtor_counter
    uint64_t data{0};
};

inline void init_dtor_counters()
{
    DtorObj::s_dtor_counter() = 0;
    DtorObj::s_ctor_counter() = 0;
    DtorObj::s_default_ctor_counter() = 0;
    DtorObj::s_value_ctor_counter() = 0;
    DtorObj::s_copy_ctor_counter() = 0;
    DtorObj::s_move_ctor_counter() = 0;
    DtorObj::s_assign_counter() = 0;
}

inline bool operator==(DtorObj const &lhs, DtorObj const &rhs)
{
    return lhs.data == rhs.data;
}

struct DtorHash
{
    /// Delete implementation for types that don't specifically override
    /// with a valid hasher implementation
    uint64_t operator()(DtorObj const &value) const noexcept
    {
        return wyhash(&value.data, sizeof(value.data), 0, _wyp);
    }
};

#endif // WHEELS_TESTS_CONTAINERS_COMMON_HPP