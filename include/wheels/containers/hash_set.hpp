#ifndef WHEELS_HASH_SET
#define WHEELS_HASH_SET

#include "../allocators/allocator.hpp"
#include "concepts.hpp"
#include "hash.hpp"
#include "utils.hpp"

#include <cstring>

namespace wheels
{

// Based on Google's SwissMap cppcon 2017 talk by Matt Kulukundis
// without the SIMD magic for now
// https://www.youtube.com/watch?v=ncHmEUmJZf4

template <typename T, class Hasher = Hash<T>> class HashSet
{
  public:
    struct ConstIterator
    {
        ConstIterator &operator++();
        ConstIterator &operator++(int);
        T const &operator*() const;
        T const *operator->() const;
        bool operator!=(HashSet<T, Hasher>::ConstIterator const &other) const;
        bool operator==(HashSet<T, Hasher>::ConstIterator const &other) const;

        HashSet const &set;
        size_t pos{0};
    };

    friend struct ConstIterator;

  public:
    HashSet(Allocator &allocator, size_t initial_capacity = 32);
    ~HashSet();

    HashSet(HashSet<T, Hasher> const &other) = delete;
    HashSet(HashSet<T, Hasher> &&other);
    HashSet<T, Hasher> &operator=(HashSet<T, Hasher> const &other) = delete;
    HashSet<T, Hasher> &operator=(HashSet<T, Hasher> &&other);

    ConstIterator begin() const;
    ConstIterator end() const;

    bool empty() const;
    size_t size() const;
    size_t capacity() const;

    bool contains(T const &value) const;
    ConstIterator find(T const &value) const;

    void clear();

    template <typename U>
    // Let's be pedantic and disallow implicit conversions
        requires SameAs<U, T>
    void insert(U &&value);

    void remove(T const &value);

  private:
    bool is_over_max_load() const;

    void grow(size_t capacity);
    void free();

    enum class Ctrl : uint8_t
    {
        Empty = 0b10000000,
        Deleted = 0b11111111,
        // TODO: Sentinel to speed up the tail of table scan?
        // Full = 0b0XXXXXXX, H2 hash
    };
    static constexpr bool s_empty_pos(uint8_t const *metadata, size_t pos)
    {
        return (metadata[pos] & (uint8_t)Ctrl::Empty) == (uint8_t)Ctrl::Empty;
    }

    static constexpr uint64_t s_h1(uint64_t hash) { return hash >> 7; }

    static constexpr uint8_t s_h2(uint64_t hash)
    {
        return (uint8_t)(hash & 0x7F);
    }

    Allocator &m_allocator;
    T *m_data{nullptr};
    uint8_t *m_metadata{nullptr};
    size_t m_size{0};
    size_t m_capacity{0};
    Hasher m_hasher{};
};

template <typename T, class Hasher>
HashSet<T, Hasher>::HashSet(Allocator &allocator, size_t initial_capacity)
: m_allocator{allocator}
{
    static_assert(
        alignof(T) <= alignof(std::max_align_t) &&
        "Aligned allocations beyond std::max_align_t aren't supported");

    // Our max load factor is 15/16 so we have to have 32 as the capacity to
    // ensure we always grow in time so that there always is at least 1 Empty
    // slot to end find iteration
    initial_capacity = initial_capacity < 32 ? 32 : initial_capacity;
    initial_capacity = round_up_power_of_two(initial_capacity);

    grow(initial_capacity);
}

template <typename T, class Hasher> HashSet<T, Hasher>::~HashSet() { free(); }

template <typename T, class Hasher>
HashSet<T, Hasher>::HashSet(HashSet<T, Hasher> &&other)
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_metadata{other.m_metadata}
, m_size{other.m_size}
, m_capacity{other.m_capacity}
, m_hasher{WHEELS_MOV(other.m_hasher)}
{
    other.m_data = nullptr;
}

template <typename T, class Hasher>
HashSet<T, Hasher> &HashSet<T, Hasher>::operator=(HashSet<T, Hasher> &&other)
{
    if (this != &other)
    {
        free();

        m_allocator = other.m_allocator;
        m_data = other.m_data;
        m_metadata = other.m_metadata;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_hasher = WHEELS_MOV(other.m_hasher);

        other.m_data = nullptr;
    }
    return *this;
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::begin() const
{
    ConstIterator iter{
        .set = *this,
        .pos = 0,
    };

    if (s_empty_pos(m_metadata, iter.pos))
        iter++;
    assert(iter == end() || !s_empty_pos(m_metadata, iter.pos));

    return iter;
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::end() const
{
    return ConstIterator{
        .set = *this,
        .pos = m_capacity,
    };
}

template <typename T, class Hasher> bool HashSet<T, Hasher>::empty() const
{
    return m_size == 0;
}

template <typename T, class Hasher> size_t HashSet<T, Hasher>::size() const
{
    return m_size;
}

template <typename T, class Hasher> size_t HashSet<T, Hasher>::capacity() const
{
    return m_capacity;
}

template <typename T, class Hasher>
bool HashSet<T, Hasher>::contains(T const &value) const
{
    return find(value) != end();
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::find(
    T const &value) const
{
    uint64_t const hash = m_hasher(value);
    uint8_t const h2 = s_h2(hash);
    // Keep track of start pos so we can break out before looping again if all
    // slots are full or deleted.
    // Capacity is a power of 2 so this mask just works
    size_t const start_pos = s_h1(hash) & (m_capacity - 1);
    size_t pos = start_pos;
    while (m_metadata[pos] != (uint8_t)Ctrl::Empty)
    {
        uint8_t const meta = m_metadata[pos];
        if (h2 == meta && value == m_data[pos])
            return ConstIterator{
                .set = *this,
                .pos = pos,
            };

        // capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
        if (pos == start_pos) [[unlikely]]
            break;
    }

    return end();
}

template <typename T, class Hasher> void HashSet<T, Hasher>::clear()
{
    if (m_size > 0)
    {
        for (size_t i = 0; i < m_capacity; ++i)
        {
            if (!s_empty_pos(m_metadata, i))
            {
                m_data[i].~T();
            }
        }
        m_size = 0;
    }
    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));
}

template <typename T, class Hasher>
template <typename U>
    requires SameAs<U, T>
void HashSet<T, Hasher>::insert(U &&value)
{
    if (is_over_max_load())
        grow(m_capacity * 2);

    uint64_t const hash = m_hasher(value);
    uint8_t const h2 = s_h2(hash);
    // Capacity is a power of 2 so this mask just works
    size_t pos = s_h1(hash) & (m_capacity - 1);
    while (true)
    {
        if (s_empty_pos(m_metadata, pos))
        {
            new (m_data + pos) T{WHEELS_FWD(value)};
            m_metadata[pos] = h2;
            m_size++;
            return;
        }
        else if (h2 == m_metadata[pos] && value == m_data[pos])
            return;

        // Capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
    }
}

template <typename T, class Hasher>
void HashSet<T, Hasher>::remove(T const &value)
{
    uint64_t const hash = m_hasher(value);
    uint8_t const h2 = s_h2(hash);
    // Keep track of start pos so we can break out before looping again if all
    // slots are full or deleted.
    // Capacity is a power of 2 so this mask just works
    size_t const start_pos = s_h1(hash) & (m_capacity - 1);
    size_t pos = start_pos;
    while (m_metadata[pos] != (uint8_t)Ctrl::Empty)
    {
        uint8_t const meta = m_metadata[pos];
        if (h2 == meta && value == m_data[pos])
        {
            m_data[pos].~T();
            m_metadata[pos] = (uint8_t)Ctrl::Deleted;
            m_size--;

            // Find for missing value gets really bad if all slots are Deleted
            // so let's clean up to be safe
            if (m_size == 0) [[unlikely]]
                clear();

            return;
        }

        // Capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
        if (pos == start_pos) [[unlikely]]
            break;
    }
}

template <typename T, class Hasher>
bool HashSet<T, Hasher>::is_over_max_load() const
{
    // Magic factor from the talk, matching the arbitrary offset SSE version
    // as reading one metadata byte at a time is basically the same
    // size / capacity > 15 / 16
    return 16 * m_size > 15 * m_capacity;
}

template <typename T, class Hasher>
void HashSet<T, Hasher>::grow(size_t capacity)
{
    assert(capacity > m_capacity);
    // Assume capacity is a power of two so we can avoid modulus operations on
    // the hash
    assert(round_up_power_of_two(capacity) == capacity);

    T *old_data = m_data;
    uint8_t *old_metadata = m_metadata;
    size_t const old_capacity = m_capacity;

    m_data = (T *)m_allocator.allocate(capacity * sizeof(T));
    assert(m_data != nullptr);
    m_metadata = (uint8_t *)m_allocator.allocate(capacity * sizeof(uint8_t));
    assert(m_metadata != nullptr);

    m_size = 0;
    m_capacity = capacity;

    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));

    for (size_t pos = 0; pos < old_capacity; ++pos)
    {
        if (s_empty_pos(old_metadata, pos))
            continue;

        insert(WHEELS_MOV(old_data[pos]));
    }

    // No need to call dtors as we moved the values
    m_allocator.deallocate(old_data);
    m_allocator.deallocate(old_metadata);
}

template <typename T, class Hasher> void HashSet<T, Hasher>::free()
{
    if (m_data != nullptr)
    {
        clear();
        m_allocator.deallocate(m_data);
        m_allocator.deallocate(m_metadata);
        m_data = nullptr;
    }
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator &HashSet<
    T, Hasher>::ConstIterator::operator++()
{
    assert(pos < set.capacity());
    do
    {
        pos++;
    } while (pos < set.capacity() && set.s_empty_pos(set.m_metadata, pos));
    return *this;
};

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator &HashSet<
    T, Hasher>::ConstIterator::operator++(int)
{
    return ++*this;
}

template <typename T, class Hasher>
T const &HashSet<T, Hasher>::ConstIterator::operator*() const
{
    assert(pos < set.capacity());
    assert(!set.s_empty_pos(set.m_metadata, pos));

    return set.m_data[pos];
};

template <typename T, class Hasher>
T const *HashSet<T, Hasher>::ConstIterator::operator->() const
{
    return &**this;
};

template <typename T, class Hasher>
bool HashSet<T, Hasher>::ConstIterator::operator!=(
    HashSet<T, Hasher>::ConstIterator const &other) const
{
    return pos != other.pos;
};

template <typename T, class Hasher>
bool HashSet<T, Hasher>::ConstIterator::operator==(
    HashSet<T, Hasher>::ConstIterator const &other) const
{
    return pos == other.pos;
};

} // namespace wheels

#endif // WHEELS_HASH_SET
