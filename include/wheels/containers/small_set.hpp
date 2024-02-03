
#ifndef WHEELS_CONTAINERS_SMALL_SET_HPP
#define WHEELS_CONTAINERS_SMALL_SET_HPP

#include "../utils.hpp"
#include "inline_array.hpp"

namespace wheels
{

template <typename T, size_t N> class SmallSet
{
  public:
    SmallSet() noexcept {};
    ~SmallSet();

    SmallSet(SmallSet<T, N> const &other) noexcept;
    SmallSet(SmallSet<T, N> &&other) noexcept;
    SmallSet<T, N> &operator=(SmallSet<T, N> const &other) noexcept;
    SmallSet<T, N> &operator=(SmallSet<T, N> &&other) noexcept;

    [[nodiscard]] T *begin() noexcept;
    [[nodiscard]] T const *begin() const noexcept;
    [[nodiscard]] T *end() noexcept;
    [[nodiscard]] T const *end() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    [[nodiscard]] bool contains(T const &value) const noexcept;

    void clear() noexcept;
    void insert(T const &value) noexcept;
    void insert(T &&value) noexcept;
    void remove(T const &value) noexcept;

  private:
    InlineArray<T, N> m_data;
};

template <typename T, size_t N> SmallSet<T, N>::~SmallSet() { clear(); }

template <typename T, size_t N>
SmallSet<T, N>::SmallSet(SmallSet<T, N> const &other) noexcept
: m_data{other.m_data}
{
}

template <typename T, size_t N>
SmallSet<T, N>::SmallSet(SmallSet<T, N> &&other) noexcept
: m_data{WHEELS_MOV(other.m_data)}
{
}

template <typename T, size_t N>
SmallSet<T, N> &SmallSet<T, N>::operator=(SmallSet<T, N> const &other) noexcept
{
    if (this != &other)
        m_data = other.m_data;

    return *this;
}

template <typename T, size_t N>
SmallSet<T, N> &SmallSet<T, N>::operator=(SmallSet<T, N> &&other) noexcept
{
    if (this != &other)
        m_data = WHEELS_MOV(other.m_data);

    return *this;
}

template <typename T, size_t N> T *SmallSet<T, N>::begin() noexcept
{
    return m_data.begin();
}

template <typename T, size_t N> T const *SmallSet<T, N>::begin() const noexcept
{
    return m_data.begin();
}

template <typename T, size_t N> T *SmallSet<T, N>::end() noexcept
{
    return m_data.end();
}

template <typename T, size_t N> T const *SmallSet<T, N>::end() const noexcept
{
    return m_data.end();
}

template <typename T, size_t N> bool SmallSet<T, N>::empty() const noexcept
{
    return m_data.empty();
}

template <typename T, size_t N> size_t SmallSet<T, N>::size() const noexcept
{
    return m_data.size();
}

template <typename T, size_t N> size_t SmallSet<T, N>::capacity() const noexcept
{
    return m_data.capacity();
}

template <typename T, size_t N>
bool SmallSet<T, N>::contains(T const &value) const noexcept
{
    for (auto const &v : m_data)
    {
        if (v == value)
            return true;
    }

    return false;
}

template <typename T, size_t N> void SmallSet<T, N>::clear() noexcept
{
    m_data.clear();
}

template <typename T, size_t N>
void SmallSet<T, N>::insert(T const &value) noexcept
{
    if (contains(value))
        return;
    m_data.push_back(value);
}

template <typename T, size_t N> void SmallSet<T, N>::insert(T &&value) noexcept
{
    if (contains(value))
        return;
    m_data.push_back(WHEELS_FWD(value));
}

template <typename T, size_t N>
void SmallSet<T, N>::remove(T const &value) noexcept
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
