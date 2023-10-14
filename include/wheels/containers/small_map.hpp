
#ifndef WHEELS_CONTAINERS_SMALL_MAP_HPP
#define WHEELS_CONTAINERS_SMALL_MAP_HPP

#include "../utils.hpp"
#include "concepts.hpp"
#include "pair.hpp"
#include "static_array.hpp"

namespace wheels
{

template <typename K, typename V, size_t N> class SmallMap
{
  public:
    SmallMap(){};
    ~SmallMap();

    SmallMap(SmallMap<K, V, N> const &other);
    SmallMap(SmallMap<K, V, N> &&other);
    SmallMap<K, V, N> &operator=(SmallMap<K, V, N> const &other);
    SmallMap<K, V, N> &operator=(SmallMap<K, V, N> &&other);

    [[nodiscard]] Pair<K, V> *begin();
    [[nodiscard]] Pair<K, V> const *begin() const;
    [[nodiscard]] Pair<K, V> *end();
    [[nodiscard]] Pair<K, V> const *end() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t capacity() const;

    [[nodiscard]] bool contains(K const &key) const;
    [[nodiscard]] V *find(K const &key);
    [[nodiscard]] V const *find(K const &key) const;

    void clear();
    template <typename Key, typename Value>
    // Let's be pedantic and disallow implicit conversions
        requires(SameAs<K, Key> && SameAs<V, Value>)
    void insert_or_assign(Key &&key, Value &&value);
    void remove(K const &key);

  private:
    StaticArray<Pair<K, V>, N> m_data;
};

template <typename K, typename V, size_t N> SmallMap<K, V, N>::~SmallMap()
{
    clear();
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N>::SmallMap(SmallMap<K, V, N> const &other)
: m_data{other.m_data}
{
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N>::SmallMap(SmallMap<K, V, N> &&other)
: m_data{WHEELS_MOV(other.m_data)}
{
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N> &SmallMap<K, V, N>::operator=(SmallMap<K, V, N> const &other)
{
    if (this != &other)
        m_data = other.m_data;

    return *this;
}

template <typename K, typename V, size_t N>
SmallMap<K, V, N> &SmallMap<K, V, N>::operator=(SmallMap<K, V, N> &&other)
{
    if (this != &other)
        m_data = WHEELS_MOV(other.m_data);

    return *this;
}

template <typename K, typename V, size_t N>
Pair<K, V> *SmallMap<K, V, N>::begin()
{
    return m_data.begin();
}

template <typename K, typename V, size_t N>
Pair<K, V> const *SmallMap<K, V, N>::begin() const
{
    return m_data.begin();
}

template <typename K, typename V, size_t N> Pair<K, V> *SmallMap<K, V, N>::end()
{
    return m_data.end();
}

template <typename K, typename V, size_t N>
Pair<K, V> const *SmallMap<K, V, N>::end() const
{
    return m_data.end();
}

template <typename K, typename V, size_t N>
bool SmallMap<K, V, N>::empty() const
{
    return m_data.empty();
}

template <typename K, typename V, size_t N>
size_t SmallMap<K, V, N>::size() const
{
    return m_data.size();
}

template <typename K, typename V, size_t N>
size_t SmallMap<K, V, N>::capacity() const
{
    return m_data.capacity();
}

template <typename K, typename V, size_t N>
bool SmallMap<K, V, N>::contains(K const &key) const
{
    for (auto const &kv : m_data)
    {
        if (kv.first == key)
            return true;
    }

    return false;
}

template <typename K, typename V, size_t N>
V *SmallMap<K, V, N>::find(K const &key)
{
    for (auto &kv : m_data)
    {
        if (kv.first == key)
            return &kv.second;
    }

    return nullptr;
}

template <typename K, typename V, size_t N>
V const *SmallMap<K, V, N>::find(K const &key) const
{
    for (auto const &kv : m_data)
    {
        if (kv.first == key)
            return &kv.second;
    }

    return nullptr;
}

template <typename K, typename V, size_t N> void SmallMap<K, V, N>::clear()
{
    m_data.clear();
}

template <typename K, typename V, size_t N>
template <typename Key, typename Value>
// Let's be pedantic and disallow implicit conversions
    requires(SameAs<K, Key> && SameAs<V, Value>)
void SmallMap<K, V, N>::insert_or_assign(Key &&key, Value &&value)
{
    if (V *v = find(key); v != nullptr)
        *v = value;
    else
        m_data.emplace_back(WHEELS_FWD(key), WHEELS_FWD(value));
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::remove(K const &key)
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
