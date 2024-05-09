#ifndef WHEELS_HASH_SET
#define WHEELS_HASH_SET

#include "../allocators/allocator.hpp"
#include "../assert.hpp"
#include "../utils.hpp"
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
    static_assert(
        InvocableHash<Hasher, T>, "Hasher has to be invocable with Key");
    static_assert(
        CorrectHashRetVal<Hasher, T>,
        "Hasher return type has to match Hash<T>");

  public:
    struct ConstIterator
    {
        ConstIterator &operator++() noexcept;
        ConstIterator &operator++(int) noexcept;
        [[nodiscard]] T const &operator*() const noexcept;
        [[nodiscard]] T const *operator->() const noexcept;
        [[nodiscard]] bool operator!=(
            HashSet<T, Hasher>::ConstIterator const &other) const noexcept;
        [[nodiscard]] bool operator==(
            HashSet<T, Hasher>::ConstIterator const &other) const noexcept;

        HashSet const &set;
        size_t pos{0};
    };

    friend struct ConstIterator;

  public:
    HashSet(Allocator &allocator, size_t initial_capacity = 0) noexcept;
    ~HashSet();

    HashSet(HashSet<T, Hasher> const &other) = delete;
    HashSet(HashSet<T, Hasher> &&other) noexcept;
    HashSet<T, Hasher> &operator=(HashSet<T, Hasher> const &other) = delete;
    HashSet<T, Hasher> &operator=(HashSet<T, Hasher> &&other) noexcept;

    [[nodiscard]] ConstIterator begin() const noexcept;
    [[nodiscard]] ConstIterator end() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    [[nodiscard]] bool contains(T const &value) const noexcept;
    [[nodiscard]] ConstIterator find(T const &value) const noexcept;

    void clear() noexcept;

    template <typename U>
    // Let's be pedantic and disallow implicit conversions
        requires SameAs<U, T>
    void insert(U &&value) noexcept;

    void remove(T const &value) noexcept;

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
    T *m_data{nullptr};
    uint8_t *m_metadata{nullptr};
    size_t m_size{0};
    size_t m_capacity{0};
    Hasher m_hasher{};
};

template <typename T, class Hasher>
HashSet<T, Hasher>::HashSet(
    Allocator &allocator, size_t initial_capacity) noexcept
: m_allocator{allocator}
{
    static_assert(
        alignof(T) <= alignof(std::max_align_t) &&
        "Aligned allocations beyond std::max_align_t aren't supported");

    if (initial_capacity > 0)
        grow(initial_capacity);
}

template <typename T, class Hasher> HashSet<T, Hasher>::~HashSet()
{
    destroy();
}

template <typename T, class Hasher>
HashSet<T, Hasher>::HashSet(HashSet<T, Hasher> &&other) noexcept
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
HashSet<T, Hasher> &HashSet<T, Hasher>::operator=(
    HashSet<T, Hasher> &&other) noexcept
{
    WHEELS_ASSERT(
        &m_allocator == &other.m_allocator &&
        "Move assigning a container with different allocators can lead to "
        "nasty bugs. Use the same allocator or copy the content instead.");

    if (this != &other)
    {
        destroy();

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
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::begin()
    const noexcept
{
    ConstIterator iter{
        .set = *this,
        .pos = 0,
    };

    if (m_capacity == 0)
        return iter;

    if (s_empty_pos(m_metadata, iter.pos))
        iter++;
    WHEELS_ASSERT(iter == end() || !s_empty_pos(m_metadata, iter.pos));

    return iter;
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::end()
    const noexcept
{
    return ConstIterator{
        .set = *this,
        .pos = m_capacity,
    };
}

template <typename T, class Hasher>
bool HashSet<T, Hasher>::empty() const noexcept
{
    return m_size == 0;
}

template <typename T, class Hasher>
size_t HashSet<T, Hasher>::size() const noexcept
{
    return m_size;
}

template <typename T, class Hasher>
size_t HashSet<T, Hasher>::capacity() const noexcept
{
    return m_capacity;
}

template <typename T, class Hasher>
bool HashSet<T, Hasher>::contains(T const &value) const noexcept
{
    return find(value) != end();
}

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator HashSet<T, Hasher>::find(
    T const &value) const noexcept
{
    if (m_size == 0)
        return end();

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

template <typename T, class Hasher> void HashSet<T, Hasher>::clear() noexcept
{
    if (m_size > 0)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < m_capacity; ++i)
            {
                if (!s_empty_pos(m_metadata, i))
                {
                    m_data[i].~T();
                }
            }
        }
        m_size = 0;
    }
    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));
}

template <typename T, class Hasher>
template <typename U>
    requires SameAs<U, T>
void HashSet<T, Hasher>::insert(U &&value) noexcept
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
void HashSet<T, Hasher>::remove(T const &value) noexcept
{
    if (m_size == 0)
        return;

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
            if constexpr (!std::is_trivially_destructible_v<T>)
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
bool HashSet<T, Hasher>::is_over_max_load() const noexcept
{
    // Magic factor from the talk, matching the arbitrary offset SSE version
    // as reading one metadata byte at a time is basically the same
    // size / capacity > 15 / 16
    return m_capacity == 0 || 16 * m_size > 15 * m_capacity;
}

template <typename T, class Hasher>
void HashSet<T, Hasher>::grow(size_t capacity) noexcept
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

    T *old_data = m_data;
    uint8_t *old_metadata = m_metadata;
    size_t const old_capacity = m_capacity;

    m_data = (T *)m_allocator.allocate(capacity * sizeof(T));
    WHEELS_ASSERT(m_data != nullptr);
    m_metadata = (uint8_t *)m_allocator.allocate(capacity * sizeof(uint8_t));
    WHEELS_ASSERT(m_metadata != nullptr);

    m_size = 0;
    m_capacity = capacity;

    memset(m_metadata, (uint8_t)Ctrl::Empty, m_capacity * sizeof(uint8_t));

    for (size_t pos = 0; pos < old_capacity; ++pos)
    {
        if (s_empty_pos(old_metadata, pos))
            continue;

        insert(WHEELS_MOV(old_data[pos]));
        if constexpr (!std::is_trivially_destructible_v<T>)
            // Moved from value might still require dtor
            old_data[pos].~T();
    }

    // No need to call dtors as we moved the values
    m_allocator.deallocate(old_data);
    m_allocator.deallocate(old_metadata);
}

template <typename T, class Hasher> void HashSet<T, Hasher>::destroy() noexcept
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
    T, Hasher>::ConstIterator::operator++() noexcept
{
    WHEELS_ASSERT(pos < set.capacity());
    do
    {
        pos++;
    } while (pos < set.capacity() && set.s_empty_pos(set.m_metadata, pos));
    return *this;
};

template <typename T, class Hasher>
typename HashSet<T, Hasher>::ConstIterator &HashSet<
    T, Hasher>::ConstIterator::operator++(int) noexcept
{
    return ++*this;
}

template <typename T, class Hasher>
T const &HashSet<T, Hasher>::ConstIterator::operator*() const noexcept
{
    WHEELS_ASSERT(pos < set.capacity());
    WHEELS_ASSERT(!set.s_empty_pos(set.m_metadata, pos));

    return set.m_data[pos];
};

template <typename T, class Hasher>
T const *HashSet<T, Hasher>::ConstIterator::operator->() const noexcept
{
    return &**this;
};

template <typename T, class Hasher>
bool HashSet<T, Hasher>::ConstIterator::operator!=(
    HashSet<T, Hasher>::ConstIterator const &other) const noexcept
{
    return pos != other.pos;
};

template <typename T, class Hasher>
bool HashSet<T, Hasher>::ConstIterator::operator==(
    HashSet<T, Hasher>::ConstIterator const &other) const noexcept
{
    return pos == other.pos;
};

} // namespace wheels

#endif // WHEELS_HASH_SET
