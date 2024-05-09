#ifndef WHEELS_HASH_MAP
#define WHEELS_HASH_MAP

#include "../allocators/allocator.hpp"
#include "../assert.hpp"
#include "../utils.hpp"
#include "concepts.hpp"
#include "hash.hpp"
#include "pair.hpp"
#include "utils.hpp"

#include <cstring>

namespace wheels
{

// Based on Google's SwissMap cppcon 2017 talk by Matt Kulukundis
// without the SIMD magic for now
// https://www.youtube.com/watch?v=ncHmEUmJZf4

template <typename Key, typename Value, class Hasher = Hash<Key>> class HashMap
{
    static_assert(
        InvocableHash<Hasher, Key>, "Hasher has to be invocable with Key");
    static_assert(
        CorrectHashRetVal<Hasher, Key>,
        "Hasher return type has to match Hash<T>");

  public:
    struct Iterator
    {
        Iterator &operator++() noexcept;
        Iterator &operator++(int) noexcept;
        // Only value is mutable because changing the key could require
        // rehashing
        [[nodiscard]] Pair<Key const *, Value *> operator*() noexcept;
        [[nodiscard]] Pair<Key const *, Value const *> operator*()
            const noexcept;
        [[nodiscard]] bool operator!=(
            HashMap<Key, Value, Hasher>::Iterator const &other) const noexcept;
        [[nodiscard]] bool operator==(
            HashMap<Key, Value, Hasher>::Iterator const &other) const noexcept;

        HashMap const &map;
        size_t pos{0};
    };

    struct ConstIterator
    {
        ConstIterator &operator++() noexcept;
        ConstIterator &operator++(int) noexcept;
        [[nodiscard]] Pair<Key const *, Value const *> operator*()
            const noexcept;
        [[nodiscard]] bool operator!=(
            HashMap<Key, Value, Hasher>::ConstIterator const &other)
            const noexcept;
        [[nodiscard]] bool operator==(
            HashMap<Key, Value, Hasher>::ConstIterator const &other)
            const noexcept;

        HashMap const &map;
        size_t pos{0};
    };

    friend struct Iterator;
    friend struct ConstIterator;

  public:
    HashMap(Allocator &allocator, size_t initial_capacity = 0) noexcept;
    ~HashMap();

    HashMap(HashMap<Key, Value, Hasher> const &other) = delete;
    HashMap(HashMap<Key, Value, Hasher> &&other) noexcept;
    HashMap<Key, Value, Hasher> &operator=(
        HashMap<Key, Value, Hasher> const &other) = delete;
    HashMap<Key, Value, Hasher> &operator=(
        HashMap<Key, Value, Hasher> &&other) noexcept;

    [[nodiscard]] Iterator begin() noexcept;
    [[nodiscard]] ConstIterator begin() const noexcept;
    [[nodiscard]] Iterator end() noexcept;
    [[nodiscard]] ConstIterator end() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    [[nodiscard]] bool contains(Key const &key) const noexcept;
    [[nodiscard]] Value const *find(Key const &key) const noexcept;
    [[nodiscard]] Value *find(Key const &key) noexcept;

    void clear() noexcept;

    template <typename K, typename V>
    // Let's be pedantic and disallow implicit conversions
        requires(SameAs<K, Key> && SameAs<V, Value>)
    Value *insert_or_assign(K &&key, V &&value) noexcept;
    // Don't have a pair insert for now as it would have to either copy every
    // time or I'd have to write two versions: one for lvalue that copies and
    // one for rvalue that moves

    void remove(Key const &key) noexcept;

  private:
    [[nodiscard]] bool is_over_max_load() const noexcept;

    void grow(size_t capacity) noexcept;
    void destroy() noexcept;

    enum class Ctrl : uint8_t
    {
        Empty = 0b10000000,
        Deleted = 0b11111111,
        // TODO: Sentinel to speed up the tail of table scan?
        // Full = 0b0XXXXXXX, H2 hash
    };
    [[nodiscard]] static constexpr bool s_empty_pos(
        uint8_t const *metadata, size_t pos) noexcept
    {
        return (metadata[pos] & (uint8_t)Ctrl::Empty) == (uint8_t)Ctrl::Empty;
    }

    [[nodiscard]] static constexpr uint64_t s_h1(uint64_t hash) noexcept
    {
        return hash >> 7;
    }

    [[nodiscard]] static constexpr uint8_t s_h2(uint64_t hash) noexcept
    {
        return (uint8_t)(hash & 0x7F);
    }

    Allocator &m_allocator;
    Key *m_keys{nullptr};
    Value *m_values{nullptr};
    uint8_t *m_metadata{nullptr};
    size_t m_size{0};
    size_t m_capacity{0};
    Hasher m_hasher{};
};

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::HashMap(
    Allocator &allocator, size_t initial_capacity) noexcept
: m_allocator{allocator}
{
    static_assert(
        alignof(Key) <= alignof(std::max_align_t) &&
        alignof(Value) <= alignof(std::max_align_t) &&
        "Aligned allocations beyond std::max_align_t aren't supported");

    if (initial_capacity > 0)
        grow(initial_capacity);
}

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::~HashMap()
{
    destroy();
}

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher>::HashMap(
    HashMap<Key, Value, Hasher> &&other) noexcept
: m_allocator{other.m_allocator}
, m_keys{other.m_keys}
, m_values{other.m_values}
, m_metadata{other.m_metadata}
, m_size{other.m_size}
, m_capacity{other.m_capacity}
, m_hasher{WHEELS_MOV(other.m_hasher)}
{
    other.m_keys = nullptr;
}

template <typename Key, typename Value, class Hasher>
HashMap<Key, Value, Hasher> &HashMap<Key, Value, Hasher>::operator=(
    HashMap<Key, Value, Hasher> &&other) noexcept
{
    WHEELS_ASSERT(
        &m_allocator == &other.m_allocator &&
        "Move assigning a container with different allocators can lead to "
        "nasty bugs. Use the same allocator or copy the content instead.");

    if (this != &other)
    {
        destroy();

        m_keys = other.m_keys;
        m_values = other.m_values;
        m_metadata = other.m_metadata;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_hasher = WHEELS_MOV(other.m_hasher);

        other.m_keys = nullptr;
    }
    return *this;
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::Iterator HashMap<
    Key, Value, Hasher>::begin() noexcept
{
    Iterator iter{
        .map = *this,
        .pos = 0,
    };

    if (m_capacity == 0)
        return iter;

    if (s_empty_pos(m_metadata, iter.pos))
        iter++;
    WHEELS_ASSERT(iter == end() || !s_empty_pos(m_metadata, iter.pos));

    return iter;
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator HashMap<
    Key, Value, Hasher>::begin() const noexcept
{
    ConstIterator iter{
        .map = *this,
        .pos = 0,
    };

    if (m_capacity == 0)
        return iter;

    if (s_empty_pos(m_metadata, iter.pos))
        iter++;
    WHEELS_ASSERT(iter == end() || !s_empty_pos(m_metadata, iter.pos));

    return iter;
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::Iterator HashMap<
    Key, Value, Hasher>::end() noexcept
{
    return Iterator{
        .map = *this,
        .pos = m_capacity,
    };
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator HashMap<
    Key, Value, Hasher>::end() const noexcept
{
    return ConstIterator{
        .map = *this,
        .pos = m_capacity,
    };
}

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::empty() const noexcept
{
    return m_size == 0;
}

template <typename Key, typename Value, class Hasher>
size_t HashMap<Key, Value, Hasher>::size() const noexcept
{
    return m_size;
}

template <typename Key, typename Value, class Hasher>
size_t HashMap<Key, Value, Hasher>::capacity() const noexcept
{
    return m_capacity;
}

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::contains(Key const &key) const noexcept
{
    return find(key) != nullptr;
}

template <typename Key, typename Value, class Hasher>
Value const *HashMap<Key, Value, Hasher>::find(Key const &key) const noexcept
{
    if (m_size == 0)
        return nullptr;

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
        if (h2 == meta && key == m_keys[pos])
            return &m_values[pos];

        // capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
        if (pos == start_pos) [[unlikely]]
            break;
    }

    return nullptr;
}

template <typename Key, typename Value, class Hasher>
Value *HashMap<Key, Value, Hasher>::find(Key const &key) noexcept
{
    if (m_size == 0)
        return nullptr;

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
        if (h2 == meta && key == m_keys[pos])
            return &m_values[pos];

        // capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
        if (pos == start_pos) [[unlikely]]
            break;
    }

    return nullptr;
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::clear() noexcept
{
    if (m_size > 0)
    {
        if constexpr (
            !std::is_trivially_destructible_v<Key> ||
            !std::is_trivially_destructible_v<Value>)
        {
            for (size_t i = 0; i < m_capacity; ++i)
            {
                if (!s_empty_pos(m_metadata, i))
                {
                    if constexpr (!std::is_trivially_destructible_v<Key>)
                        m_keys[i].~Key();
                    if constexpr (!std::is_trivially_destructible_v<Value>)
                        m_values[i].~Value();
                }
            }
        }
        m_size = 0;
    }
    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));
}

template <typename Key, typename Value, class Hasher>
template <typename K, typename V>
    requires(SameAs<K, Key> && SameAs<V, Value>)
Value *HashMap<Key, Value, Hasher>::insert_or_assign(
    K &&key, V &&value) noexcept
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
            new (m_keys + pos) Key{WHEELS_FWD(key)};
            new (m_values + pos) Value{WHEELS_FWD(value)};
            m_metadata[pos] = h2;
            m_size++;
            return &m_values[pos];
        }
        else if (h2 == m_metadata[pos] && key == m_keys[pos])
        {
            m_values[pos].~Value();
            new (m_values + pos) Value{WHEELS_FWD(value)};
            return &m_values[pos];
        }

        // Capacity is a power of 2 so this mask just works
        pos = (pos + 1) & (m_capacity - 1);
    }
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::remove(Key const &key) noexcept
{
    if (m_size == 0)
        return;

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
        if (h2 == meta && key == m_keys[pos])
        {
            if constexpr (!std::is_trivially_destructible_v<Key>)
                m_keys[pos].~Key();
            if constexpr (!std::is_trivially_destructible_v<Value>)
                m_values[pos].~Value();
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
bool HashMap<Key, Value, Hasher>::is_over_max_load() const noexcept
{
    // Magic factor from the talk, matching the arbitrary offmap SSE version
    // as reading one metadata byte at a time is basically the same
    // size / capacity > 15 / 16
    return m_capacity == 0 || 16 * m_size > 15 * m_capacity;
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::grow(size_t capacity) noexcept
{
    // Our max load factor is 15/16 so we have to have 32 as the capacity to
    // ensure we always grow in time so that there always is at least 1 Empty
    // slot to end find iteration
    if (capacity < 32)
        capacity = 32;
    // Have capacity be a power of two so we can avoid modulus operations on
    // the hash
    capacity = round_up_power_of_two(capacity);

    WHEELS_ASSERT(capacity > m_capacity);

    Key *old_keys = m_keys;
    Value *old_values = m_values;
    uint8_t *old_metadata = m_metadata;
    size_t const old_capacity = m_capacity;

    m_keys = (Key *)m_allocator.allocate(capacity * sizeof(Key));
    WHEELS_ASSERT(m_keys != nullptr);
    m_values = (Value *)m_allocator.allocate(capacity * sizeof(Value));
    WHEELS_ASSERT(m_values != nullptr);
    m_metadata = (uint8_t *)m_allocator.allocate(capacity * sizeof(uint8_t));
    WHEELS_ASSERT(m_metadata != nullptr);

    m_size = 0;
    m_capacity = capacity;

    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));

    for (size_t pos = 0; pos < old_capacity; ++pos)
    {
        if (s_empty_pos(old_metadata, pos))
            continue;

        // TODO: We know these are unique, could skip find and just assign
        insert_or_assign(
            WHEELS_MOV(old_keys[pos]), WHEELS_MOV(old_values[pos]));
        if constexpr (!std::is_trivially_destructible_v<Key>)
            // Moved from value might still require dtor
            old_keys[pos].~Key();
        if constexpr (!std::is_trivially_destructible_v<Value>)
            // Moved from value might still require dtor
            old_values[pos].~Value();
    }

    // No need to call dtors as we moved the values
    m_allocator.deallocate(old_keys);
    m_allocator.deallocate(old_values);
    m_allocator.deallocate(old_metadata);
}

template <typename Key, typename Value, class Hasher>
void HashMap<Key, Value, Hasher>::destroy() noexcept
{
    if (m_keys != nullptr)
    {
        clear();
        m_allocator.deallocate(m_keys);
        m_allocator.deallocate(m_values);
        m_allocator.deallocate(m_metadata);
        m_keys = nullptr;
    }
}

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::Iterator &HashMap<
    Key, Value, Hasher>::Iterator::operator++() noexcept
{
    WHEELS_ASSERT(pos < map.capacity());
    do
    {
        pos++;
    } while (pos < map.capacity() && map.s_empty_pos(map.m_metadata, pos));
    return *this;
};

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::Iterator &HashMap<
    Key, Value, Hasher>::Iterator::operator++(int) noexcept
{
    return ++*this;
}

template <typename Key, typename Value, class Hasher>
Pair<Key const *, Value *> HashMap<
    Key, Value, Hasher>::Iterator::operator*() noexcept
{
    WHEELS_ASSERT(pos < map.capacity());
    WHEELS_ASSERT(!map.s_empty_pos(map.m_metadata, pos));

    Key const *key = map.m_keys + pos;
    Value *value = map.m_values + pos;
    return make_pair(key, value);
};

template <typename Key, typename Value, class Hasher>
Pair<Key const *, Value const *> HashMap<
    Key, Value, Hasher>::Iterator::operator*() const noexcept
{
    WHEELS_ASSERT(pos < map.capacity());
    WHEELS_ASSERT(!map.s_empty_pos(map.m_metadata, pos));

    Key const *key = map.m_keys + pos;
    Value const *value = map.m_values + pos;
    return make_pair(key, value);
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::Iterator::operator!=(
    HashMap<Key, Value, Hasher>::Iterator const &other) const noexcept
{
    return pos != other.pos;
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::Iterator::operator==(
    HashMap<Key, Value, Hasher>::Iterator const &other) const noexcept
{
    return pos == other.pos;
};

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator &HashMap<
    Key, Value, Hasher>::ConstIterator::operator++() noexcept
{
    WHEELS_ASSERT(pos < map.capacity());
    do
    {
        pos++;
    } while (pos < map.capacity() && map.s_empty_pos(map.m_metadata, pos));
    return *this;
};

template <typename Key, typename Value, class Hasher>
typename HashMap<Key, Value, Hasher>::ConstIterator &HashMap<
    Key, Value, Hasher>::ConstIterator::operator++(int) noexcept
{
    return ++*this;
}

template <typename Key, typename Value, class Hasher>
Pair<Key const *, Value const *> HashMap<
    Key, Value, Hasher>::ConstIterator::operator*() const noexcept
{
    WHEELS_ASSERT(pos < map.capacity());
    WHEELS_ASSERT(!map.s_empty_pos(map.m_metadata, pos));

    Key const *key = map.m_keys + pos;
    Value const *value = map.m_values + pos;
    return make_pair(key, value);
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::ConstIterator::operator!=(
    HashMap<Key, Value, Hasher>::ConstIterator const &other) const noexcept
{
    return pos != other.pos;
};

template <typename Key, typename Value, class Hasher>
bool HashMap<Key, Value, Hasher>::ConstIterator::operator==(
    HashMap<Key, Value, Hasher>::ConstIterator const &other) const noexcept
{
    return pos == other.pos;
};

} // namespace wheels

#endif // WHEELS_HASH_MAP
