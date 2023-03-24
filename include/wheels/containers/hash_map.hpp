#ifndef WHEELS_HASH_MAP
#define WHEELS_HASH_MAP

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

template <typename Key, typename Value, class Hasher = Hash<Key>> class HashMap
{
  public:
    struct ConstIterator
    {
        ConstIterator &operator++();
        ConstIterator &operator++(int);
        Pair<Key const *, Value const *> operator*();
        Pair<Key const *, Value const *> operator->();
        bool operator!=(
            HashMap<Key, Value, Hasher>::ConstIterator const &other) const;
        bool operator==(
            HashMap<Key, Value, Hasher>::ConstIterator const &other) const;

        HashMap const &map;
        size_t pos{0};
    };

    friend struct ConstIterator;

  public:
    HashMap(Allocator &allocator, size_t initial_capacity = 32);
    ~HashMap();

    HashMap(HashMap<Key, Value, Hasher> const &other) = delete;
    HashMap(HashMap<Key, Value, Hasher> &&other);
    HashMap<Key, Value, Hasher> &operator=(
        HashMap<Key, Value, Hasher> const &other) = delete;
    HashMap<Key, Value, Hasher> &operator=(HashMap<Key, Value, Hasher> &&other);

    ConstIterator begin() const;
    ConstIterator end() const;

    bool empty() const;
    size_t size() const;
    size_t capacity() const;

    bool contains(Key const &key) const;
    ConstIterator find(Key const &key) const;

    void clear();

    template <typename K, typename V>
    // Let's be pedantic and disallow implicit conversions
        requires(SameAs<K, Key> && SameAs<V, Value>)
    void insert(K &&key, V &&value);

    void remove(Key const &key);

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
    T *m_keys_data{nullptr};
    T *m_values_data{nullptr};
    uint8_t *m_metadata{nullptr};
    size_t m_size{0};
    size_t m_capacity{0};
    Hasher m_hasher{};
};

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::HashMap(
    Allocator &allocator, size_t initial_capacity)
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

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::~HashMap()
{
    free();
}

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::HashMap(HashMap<Key, Value, Hasher> &&other)
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_metadata{other.m_metadata}
, m_size{other.m_size}
, m_capacity{other.m_capacity}
, m_hasher{WHEELS_MOV(other.m_hasher)}
{
    other.m_data = nullptr;
}

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher> &HashMap<Key, Value, Hasher>::operator=(
    HashMap<Key, Value, Hasher> &&other)
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

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator HashMap<
    Key, Value, Hasher>::begin() const
{
    ConstIterator iter{
        .map = *this,
        .pos = 0,
    };

    if (s_empty_pos(m_metadata, iter.pos))
        iter++;
    assert(iter == end() || !s_empty_pos(m_metadata, iter.pos));

    return iter;
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator HashMap<
    Key, Value, Hasher>::end() const
{
    return ConstIterator{
        .map = *this,
        .pos = m_capacity,
    };
}

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::empty() const
{
    return m_size == 0;
}

template <typename Key, typename Value, class Hasher>
size_t HashMap<Key, Value, Hasher>::size() const
{
    return m_size;
}

template <typename Key, typename Value, class Hasher>
size_t HashMap<Key, Value, Hasher>::capacity() const
{
    return m_capacity;
}

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::contains(Key const &key) const
{
    return find(key) != end();
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator HashMap<
    Key, Value, Hasher>::find(Key const &key) const
{
    uint64_t const hash = m_hasher(key);
    uint8_t const h2 = s_h2(hash);
    // Keep track of start pos so we can break out before looping again if all
    // slots are full or deleted.
    // Capacity is a power of 2 so this mask just works
    size_t const start_pos = s_h1(hash) & (m_capacity - 1);
    size_t pos = start_pos;
    while (m_metadata[pos] != (uint8_t)Ctrl::Empty)
    {
        uint8_t const meta = m_metadata[pos];
        if (h2 == meta && key == m_data[pos])
            return ConstIterator{
                .map = *this,
                .pos = pos,
            };

        // capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
        if (pos == start_pos) [[unlikely]]
            break;
    }

    return end();
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::clear()
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
    memmap(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));
}

template <typename Key, typename Value, class Hasher>
template <typename K, typename V>
    requires(SameAs<K, Key> && SameAs<V, Value>)
void HashMap<Key, Value, Hasher>::insert(K &&key, V &&value)
{
    if (is_over_max_load())
        grow(m_capacity * 2);

    uint64_t const hash = m_hasher(key);
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
        else if (h2 == m_metadata[pos] && key == m_data[pos])
            return;

        // Capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
    }
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::remove(Key const &key)
{
    uint64_t const hash = m_hasher(key);
    uint8_t const h2 = s_h2(hash);
    // Keep track of start pos so we can break out before looping again if all
    // slots are full or deleted.
    // Capacity is a power of 2 so this mask just works
    size_t const start_pos = s_h1(hash) & (m_capacity - 1);
    size_t pos = start_pos;
    while (m_metadata[pos] != (uint8_t)Ctrl::Empty)
    {
        uint8_t const meta = m_metadata[pos];
        if (h2 == meta && key == m_data[pos])
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

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::is_over_max_load() const
{
    // Magic factor from the talk, matching the arbitrary offmap SSE version
    // as reading one metadata byte at a time is basically the same
    // size / capacity > 15 / 16
    return 16 * m_size > 15 * m_capacity;
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::grow(size_t capacity)
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
    m_metadata = (uint8_t *)m_allocator.allocate(capacity * sizeof(T));
    assert(m_metadata != nullptr);

    m_size = 0;
    m_capacity = capacity;

    memmap(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));

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

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::free()
{
    if (m_data != nullptr)
    {
        clear();
        m_allocator.deallocate(m_data);
        m_allocator.deallocate(m_metadata);
        m_data = nullptr;
    }
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator &HashMap<
    T, Hasher>::ConstIterator::operator++()
{
    assert(pos < map.capacity());
    do
    {
        pos++;
    } while (pos < map.capacity() && map.s_empty_pos(map.m_metadata, pos));
    return *this;
};

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator &HashMap<
    T, Hasher>::ConstIterator::operator++(int)
{
    return ++*this;
}

template <typename Key, typename Value, class Hasher>
T const &HashMap<Key, Value, Hasher>::ConstIterator::operator*()
{
    assert(pos < map.capacity());
    assert(!map.s_empty_pos(map.m_metadata, pos));

    return map.m_data[pos];
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::ConstIterator::operator!=(
    HashMap<Key, Value, Hasher>::ConstIterator const &other) const
{
    return pos != other.pos;
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::ConstIterator::operator==(
    HashMap<Key, Value, Hasher>::ConstIterator const &other) const
{
    return pos == other.pos;
};

} // namespace wheels

#endif // WHEELS_HASH_MAP