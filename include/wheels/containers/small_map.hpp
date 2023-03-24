
#ifndef WHEELS_CONTAINERS_SMALL_MAP_HPP
#define WHEELS_CONTAINERS_SMALL_MAP_HPP

#include "pair.hpp"
#include "static_array.hpp"
#include "utils.hpp"

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

    Pair<K, V> *begin();
    Pair<K, V> const *begin() const;
    Pair<K, V> *end();
    Pair<K, V> const *end() const;

    bool empty() const;
    size_t size() const;
    size_t capacity() const;

    bool contains(K const &key) const;
    V *find(K const &key);
    V const *find(K const &key) const;

    void clear();
    void insert_or_assign(K const &key, V const &value);
    void insert_or_assign(K const &key, V &&value);
    void insert_or_assign(Pair<K, V> const &key_value);
    void insert_or_assign(Pair<K, V> &&key_value);
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
void SmallMap<K, V, N>::insert_or_assign(K const &key, V const &value)
{
    if (V *v = find(key); v != nullptr)
        *v = value;
    else
        m_data.emplace_back(key, value);
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::insert_or_assign(K const &key, V &&value)
{
    if (V *v = find(key); v != nullptr)
        *v = value;
    else
        m_data.emplace_back(key, WHEELS_FWD(value));
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::insert_or_assign(Pair<K, V> const &key_value)
{
    if (V *v = find(key_value.first); v != nullptr)
        *v = key_value.second;
    else
        m_data.emplace_back(key_value);
}

template <typename K, typename V, size_t N>
void SmallMap<K, V, N>::insert_or_assign(Pair<K, V> &&key_value)
{
    if (V *v = find(key_value.first); v != nullptr)
        *v = key_value.second;
    else
        m_data.emplace_back(WHEELS_FWD(key_value));
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