
#ifndef WHEELS_CONTAINERS_SMALL_MAP_HPP
#define WHEELS_CONTAINERS_SMALL_MAP_HPP

#include "../utils.hpp"
#include "concepts.hpp"
#include "inline_array.hpp"
#include "pair.hpp"

namespace wheels
{

template <typename K, typename V, size_t N> class SmallMap
{
  public:
    using key_type = K;
    // Wording clashes with the STL counterpats, but is consistent with the
    // template interface
    using value_type = V;

    SmallMap() noexcept {};
    ~SmallMap();

    SmallMap(SmallMap<K, V, N> const &other) noexcept;
    SmallMap(SmallMap<K, V, N> &&other) noexcept;
    SmallMap<K, V, N> &operator=(SmallMap<K, V, N> const &other) noexcept;
    SmallMap<K, V, N> &operator=(SmallMap<K, V, N> &&other) noexcept;

    [[nodiscard]] Pair<K, V> *begin() noexcept;
    [[nodiscard]] Pair<K, V> const *begin() const noexcept;
    [[nodiscard]] Pair<K, V> *end() noexcept;
    [[nodiscard]] Pair<K, V> const *end() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    [[nodiscard]] bool contains(K const &key) const noexcept;
    [[nodiscard]] V *find(K const &key) noexcept;
    [[nodiscard]] V const *find(K const &key) const noexcept;

    void clear() noexcept;
    template <typename Key, typename Value>
    // Let's be pedantic and disallow implicit conversions
        requires(SameAs<K, Key> && SameAs<V, Value>)
    void insert_or_assign(Key &&key, Value &&value) noexcept;
    void remove(K const &key) noexcept;

  private:
    InlineArray<Pair<K, V>, N> m_data;
};

template <typename K, typename V, size_t N> SmallMap<K, V, N>::~SmallMap()
{
    clear();
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N>::SmallMap(SmallMap<K, V, N> const &other) noexcept
: m_data{other.m_data}
{
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N>::SmallMap(SmallMap<K, V, N> &&other) noexcept
: m_data{WHEELS_MOV(other.m_data)}
{
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N> &SmallMap<K, V, N>::operator=(
    SmallMap<K, V, N> const &other) noexcept
{
    if (this != &other)
        m_data = other.m_data;

    return *this;
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N> &SmallMap<K, V, N>::operator=(
    SmallMap<K, V, N> &&other) noexcept
{
    if (this != &other)
        m_data = WHEELS_MOV(other.m_data);

    return *this;
}

template <typename K, typename V, size_t N>
Pair<K, V> *SmallMap<K, V, N>::begin() noexcept
{
    return m_data.begin();
}

template <typename K, typename V, size_t N>
Pair<K, V> const *SmallMap<K, V, N>::begin() const noexcept
{
    return m_data.begin();
}

template <typename K, typename V, size_t N>
Pair<K, V> *SmallMap<K, V, N>::end() noexcept
{
    return m_data.end();
}

template <typename K, typename V, size_t N>
Pair<K, V> const *SmallMap<K, V, N>::end() const noexcept
{
    return m_data.end();
}

template <typename K, typename V, size_t N>
bool SmallMap<K, V, N>::empty() const noexcept
{
    return m_data.empty();
}

template <typename K, typename V, size_t N>
size_t SmallMap<K, V, N>::size() const noexcept
{
    return m_data.size();
}

template <typename K, typename V, size_t N>
size_t SmallMap<K, V, N>::capacity() const noexcept
{
    return m_data.capacity();
}

template <typename K, typename V, size_t N>
bool SmallMap<K, V, N>::contains(K const &key) const noexcept
{
    for (auto const &kv : m_data)
    {
        if (kv.first == key)
            return true;
    }

    return false;
}

template <typename K, typename V, size_t N>
V *SmallMap<K, V, N>::find(K const &key) noexcept
{
    for (auto &kv : m_data)
    {
        if (kv.first == key)
            return &kv.second;
    }

    return nullptr;
}

template <typename K, typename V, size_t N>
V const *SmallMap<K, V, N>::find(K const &key) const noexcept
{
    for (auto const &kv : m_data)
    {
        if (kv.first == key)
            return &kv.second;
    }

    return nullptr;
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::clear() noexcept
{
    m_data.clear();
}

template <typename K, typename V, size_t N>
template <typename Key, typename Value>
// Let's be pedantic and disallow implicit conversions
    requires(SameAs<K, Key> && SameAs<V, Value>)
void SmallMap<K, V, N>::insert_or_assign(Key &&key, Value &&value) noexcept
{
    if (V *v = find(key); v != nullptr)
        *v = value;
    else
        m_data.emplace_back(WHEELS_FWD(key), WHEELS_FWD(value));
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::remove(K const &key) noexcept
{
    for (auto &v : m_data)
    {
        if (v.first == key)
        {
            std::swap(v, m_data.back());
            m_data.pop_back();
            return;
        }
    }
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_SMALL_MAP_HPP
