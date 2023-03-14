
#ifndef WHEELS_CONTAINERS_SMALL_SET_HPP
#define WHEELS_CONTAINERS_SMALL_SET_HPP

#include "static_array.hpp"
#include "utils.hpp"


namespace wheels
{

template <typename T, size_t N> class SmallSet
{
  public:
    SmallSet(){};
    ~SmallSet();

    SmallSet(SmallSet<T, N> const &other);
    SmallSet(SmallSet<T, N> &&other);
    SmallSet<T, N> &operator=(SmallSet<T, N> const &other);
    SmallSet<T, N> &operator=(SmallSet<T, N> &&other);

    T *begin();
    T const *begin() const;
    T *end();
    T const *end() const;

    bool empty() const;
    size_t size() const;
    size_t capacity() const;

    bool contains(T const &value) const;

    void clear();
    void insert(T const &value);
    void insert(T &&value);
    void remove(T const &value);

  private:
    StaticArray<T, N> m_data;
};

template <typename T, size_t N> SmallSet<T, N>::~SmallSet() { clear(); }

template <typename T, size_t N>
SmallSet<T, N>::SmallSet(SmallSet<T, N> const &other)
: m_data{other.m_data}
{
}

template <typename T, size_t N>
SmallSet<T, N>::SmallSet(SmallSet<T, N> &&other)
: m_data{WHEELS_MOV(other.m_data)}
{
}

template <typename T, size_t N>
SmallSet<T, N> &SmallSet<T, N>::operator=(SmallSet<T, N> const &other)
{
    if (this != &other)
        m_data = other.m_data;

    return *this;
}

template <typename T, size_t N>
SmallSet<T, N> &SmallSet<T, N>::operator=(SmallSet<T, N> &&other)
{
    if (this != &other)
        m_data = WHEELS_MOV(other.m_data);

    return *this;
}

template <typename T, size_t N> T *SmallSet<T, N>::begin()
{
    return m_data.begin();
}

template <typename T, size_t N> T const *SmallSet<T, N>::begin() const
{
    return m_data.begin();
}

template <typename T, size_t N> T *SmallSet<T, N>::end()
{
    return m_data.end();
}

template <typename T, size_t N> T const *SmallSet<T, N>::end() const
{
    return m_data.end();
}

template <typename T, size_t N> bool SmallSet<T, N>::empty() const
{
    return m_data.empty();
}

template <typename T, size_t N> size_t SmallSet<T, N>::size() const
{
    return m_data.size();
}

template <typename T, size_t N> size_t SmallSet<T, N>::capacity() const
{
    return m_data.capacity();
}

template <typename T, size_t N>
bool SmallSet<T, N>::contains(T const &value) const
{
    for (auto const &v : m_data)
    {
        if (v == value)
            return true;
    }

    return false;
}

template <typename T, size_t N> void SmallSet<T, N>::clear() { m_data.clear(); }

template <typename T, size_t N> void SmallSet<T, N>::insert(T const &value)
{
    if (contains(value))
        return;
    m_data.push_back(value);
}

template <typename T, size_t N> void SmallSet<T, N>::insert(T &&value)
{
    if (contains(value))
        return;
    m_data.push_back(WHEELS_FWD(value));
}

template <typename T, size_t N> void SmallSet<T, N>::remove(T const &value)
{
    for (auto &v : m_data)
    {
        if (v == value)
        {
            std::swap(v, m_data.back());
            m_data.pop_back();
            return;
        }
    }
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_SMALL_SET_HPP