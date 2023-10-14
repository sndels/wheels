
#ifndef WHEELS_CONTAINERS_STATIC_ARRAY_HPP
#define WHEELS_CONTAINERS_STATIC_ARRAY_HPP

#include "../assert.hpp"
#include "../utils.hpp"
#include "span.hpp"

#include <cstring>
#include <initializer_list>

namespace wheels
{

template <typename T, size_t N> class StaticArray
{
  public:
    StaticArray(){};
    StaticArray(T const (&elems)[N]);
    // Takes in either a single default value for the entire array or a list of
    // values filling all slots
    StaticArray(std::initializer_list<T> elems);
    ~StaticArray();

    StaticArray(StaticArray<T, N> const &other);
    StaticArray(StaticArray<T, N> &&other);
    StaticArray<T, N> &operator=(StaticArray<T, N> const &other);
    StaticArray<T, N> &operator=(StaticArray<T, N> &&other);

    [[nodiscard]] T &operator[](size_t i);
    [[nodiscard]] T const &operator[](size_t i) const;
    [[nodiscard]] T &front();
    [[nodiscard]] T const &front() const;
    [[nodiscard]] T &back();
    [[nodiscard]] T const &back() const;
    [[nodiscard]] T *data();
    [[nodiscard]] T const *data() const;

    [[nodiscard]] T *begin();
    [[nodiscard]] T const *begin() const;
    [[nodiscard]] T *end();
    [[nodiscard]] T const *end() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] static constexpr size_t capacity() { return N; }

    void clear();
    void push_back(T const &value);
    void push_back(T &&value);
    template <typename... Args> void emplace_back(Args &&...args);
    T pop_back();
    void resize(size_t size);
    void resize(size_t size, T const &value);

    operator Span<T>();
    operator Span<T const>() const;

  private:
    alignas(T) uint8_t m_data[N * sizeof(T)];
    size_t m_size{0};
};

// Deduction from raw arrays
template <typename T, size_t N>
StaticArray(T const (&)[N]) -> StaticArray<T, N>;

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(T const (&elems)[N])
: m_size{N}
{
    // TODO: memcpy for entire elems array if T is trivially copyable
    for (size_t i = 0; i < N; ++i)
        new (((T *)m_data) + i) T{WHEELS_MOV(elems[i])};
}

// Deduction from intializer list
template <typename T, typename... Ts>
StaticArray(T const &, Ts...) -> StaticArray<T, 1 + sizeof...(Ts)>;

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(std::initializer_list<T> elems)
: m_size{elems.size()}
{
    WHEELS_ASSERT(elems.size() == 1 || elems.size() == N);

    if (elems.size() == 1)
    {
        m_size = N;
        T const &default_value = *elems.begin();
        for (size_t i = 0; i < N; ++i)
            new (((T *)m_data) + i) T{default_value};
    }
    else
    {
        // TODO: memcpy for entire elems array if T is trivially copyable
        size_t i = 0;
        auto const end = elems.end();
        for (auto iter = elems.begin(); iter != end; ++iter, ++i)
            new (((T *)m_data) + i) T{WHEELS_MOV(*iter)};
    }
}

template <typename T, size_t N> StaticArray<T, N>::~StaticArray() { clear(); }

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(StaticArray<T, N> const &other)
: m_size{other.m_size}
{
    for (size_t i = 0; i < other.m_size; ++i)
        new ((T *)m_data + i) T{((T *)other.m_data)[i]};
}

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(StaticArray<T, N> &&other)
: m_size{other.m_size}
{
    for (size_t i = 0; i < other.m_size; ++i)
        new ((T *)m_data + i) T{WHEELS_MOV(((T *)other.m_data)[i])};
    other.m_size = 0;
}

template <typename T, size_t N>
StaticArray<T, N> &StaticArray<T, N>::operator=(StaticArray<T, N> const &other)
{
    if (this != &other)
    {
        clear();

        for (size_t i = 0; i < other.m_size; ++i)
            new ((T *)m_data + i) T{((T *)other.m_data)[i]};
        m_size = other.m_size;
    }
    return *this;
}

template <typename T, size_t N>
StaticArray<T, N> &StaticArray<T, N>::operator=(StaticArray<T, N> &&other)
{
    if (this != &other)
    {
        clear();

        for (size_t i = 0; i < other.m_size; ++i)
            new ((T *)m_data + i) T{WHEELS_MOV(((T *)other.m_data)[i])};
        m_size = other.m_size;

        other.m_size = 0;
    }
    return *this;
}

template <typename T, size_t N> T &StaticArray<T, N>::operator[](size_t i)
{
    WHEELS_ASSERT(i < m_size);
    return ((T *)m_data)[i];
}

template <typename T, size_t N>
T const &StaticArray<T, N>::operator[](size_t i) const
{
    WHEELS_ASSERT(i < m_size);
    return ((T *)m_data)[i];
}

template <typename T, size_t N> T &StaticArray<T, N>::front()
{
    WHEELS_ASSERT(m_size > 0);
    return *((T *)m_data);
}

template <typename T, size_t N> T const &StaticArray<T, N>::front() const
{
    WHEELS_ASSERT(m_size > 0);
    return *((T *)m_data);
}

template <typename T, size_t N> T &StaticArray<T, N>::back()
{
    WHEELS_ASSERT(m_size > 0);
    return ((T *)m_data)[m_size - 1];
}

template <typename T, size_t N> T const &StaticArray<T, N>::back() const
{
    WHEELS_ASSERT(m_size > 0);
    return ((T *)m_data)[m_size - 1];
}

template <typename T, size_t N> T *StaticArray<T, N>::data()
{
    return ((T *)m_data);
}

template <typename T, size_t N> T const *StaticArray<T, N>::data() const
{
    return ((T *)m_data);
}

template <typename T, size_t N> T *StaticArray<T, N>::begin()
{
    return ((T *)m_data);
}

template <typename T, size_t N> T const *StaticArray<T, N>::begin() const
{
    return ((T *)m_data);
}

template <typename T, size_t N> T *StaticArray<T, N>::end()
{
    return ((T *)m_data) + m_size;
}

template <typename T, size_t N> T const *StaticArray<T, N>::end() const
{
    return ((T *)m_data) + m_size;
}

template <typename T, size_t N> bool StaticArray<T, N>::empty() const
{
    return m_size == 0;
}

template <typename T, size_t N> size_t StaticArray<T, N>::size() const
{
    return m_size;
}

template <typename T, size_t N> void StaticArray<T, N>::clear()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (auto &v : *this)
            v.~T();
    }
    m_size = 0;
}

template <typename T, size_t N>
void StaticArray<T, N>::push_back(T const &value)
{
    WHEELS_ASSERT(m_size < N);
    new (((T *)m_data) + m_size++) T{value};
}

template <typename T, size_t N> void StaticArray<T, N>::push_back(T &&value)
{
    WHEELS_ASSERT(m_size < N);
    new (((T *)m_data) + m_size++) T{WHEELS_FWD(value)};
}

template <typename T, size_t N>
template <typename... Args>
void StaticArray<T, N>::emplace_back(Args &&...args)
{
    WHEELS_ASSERT(m_size < N);
    new (((T *)m_data) + m_size++) T{WHEELS_FWD(args)...};
}

template <typename T, size_t N> T StaticArray<T, N>::pop_back()
{
    WHEELS_ASSERT(m_size > 0);
    m_size--;
    return WHEELS_MOV(((T *)m_data)[m_size]);
}

template <typename T, size_t N> void StaticArray<T, N>::resize(size_t size)
{
    if (size < m_size)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = size; i < m_size; ++i)
                ((T *)m_data)[i].~T();
        }
        m_size = size;
    }
    else
    {
        WHEELS_ASSERT(size <= N);
        if constexpr (std::is_class_v<T>)
        {
            for (size_t i = m_size; i < size; ++i)
                new (((T *)m_data) + i) T{};
        }
        m_size = size;
    }
}

template <typename T, size_t N>
void StaticArray<T, N>::resize(size_t size, T const &value)
{
    if (size < m_size)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = size; i < m_size; ++i)
                ((T *)m_data)[i].~T();
        }
        m_size = size;
    }
    else
    {
        WHEELS_ASSERT(size <= N);
        // TODO:
        // Is it faster to init a bunch of non-class values by assigning
        // directly instead of placement new?
        for (size_t i = m_size; i < size; ++i)
            new (((T *)m_data) + i) T{value};
        m_size = size;
    }
}

template <typename T, size_t N> StaticArray<T, N>::operator Span<T>()
{
    return Span{(T *)m_data, m_size};
}

template <typename T, size_t N>
StaticArray<T, N>::operator Span<T const>() const
{
    return Span{(T const *)m_data, m_size};
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_STATIC_ARRAY_HPP
