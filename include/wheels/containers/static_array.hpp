
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
    constexpr StaticArray() = default;
    constexpr StaticArray(T const &default_value);
    constexpr StaticArray(T const (&elems)[N]);
    // No std::initializer_list ctor because constexpr can't validate
    // list.size() == N. Also no variadric template ctor because that will fail
    // type deduction in some cases. Let's just always use the array ctor for
    // consistency and safety.
    constexpr ~StaticArray() = default;

    constexpr StaticArray(StaticArray<T, N> const &other);
    constexpr StaticArray(StaticArray<T, N> &&other);
    constexpr StaticArray<T, N> &operator=(StaticArray<T, N> const &other);
    constexpr StaticArray<T, N> &operator=(StaticArray<T, N> &&other);

    [[nodiscard]] constexpr T &operator[](size_t i);
    [[nodiscard]] constexpr T const &operator[](size_t i) const;
    [[nodiscard]] constexpr T *data();
    [[nodiscard]] constexpr T const *data() const;

    [[nodiscard]] constexpr T *begin();
    [[nodiscard]] constexpr T const *begin() const;
    [[nodiscard]] constexpr T *end();
    [[nodiscard]] constexpr T const *end() const;

    [[nodiscard]] static constexpr size_t size() { return N; };
    [[nodiscard]] static constexpr size_t capacity() { return N; }

    operator Span<T>();
    operator Span<T const>() const;

  private:
    // Default ctor zero initializes
    T m_data[N]{};
};

template <typename T, size_t N>
constexpr StaticArray<T, N>::StaticArray(T const &default_value)
{
    for (size_t i = 0; i < N; ++i)
        m_data[i] = default_value;
}

// Deduction from raw arrays
template <typename T, size_t N>
StaticArray(T const (&)[N]) -> StaticArray<T, N>;
template <typename T> StaticArray(T const &) -> StaticArray<T, 1>;

template <typename T, size_t N>
constexpr StaticArray<T, N>::StaticArray(T const (&elems)[N])
{
    // TODO:
    // Likely branch on !std::is_constant_evaluated() and constexpr
    // std::is_trivially_copyable inside it for a memcpy version?

    for (size_t i = 0; i < N; ++i)
        m_data[i] = WHEELS_MOV(elems[i]);
}

template <typename T, size_t N>
constexpr StaticArray<T, N>::StaticArray(StaticArray<T, N> const &other)
{
    // TODO:
    // Likely branch on !std::is_constant_evaluated() and constexpr
    // std::is_trivially_copyable inside it for a memcpy version?

    for (size_t i = 0; i < N; ++i)
        m_data[i] = other.m_data[i];
}

template <typename T, size_t N>
constexpr StaticArray<T, N>::StaticArray(StaticArray<T, N> &&other)
{
    // TODO:
    // Likely branch on !std::is_constant_evaluated() and constexpr
    // std::is_trivially_copyable inside it for a memcpy version?

    for (size_t i = 0; i < N; ++i)
        m_data[i] = WHEELS_MOV(other.m_data[i]);
}

template <typename T, size_t N>
constexpr StaticArray<T, N> &StaticArray<T, N>::operator=(
    StaticArray<T, N> const &other)
{
    if (this != &other)
    {
        // TODO:
        // Likely branch on !std::is_constant_evaluated() and constexpr
        // std::is_trivially_copyable inside it for a memcpy version?

        for (size_t i = 0; i < N; ++i)
            m_data[i] = other.m_data[i];
    }
    return *this;
}

template <typename T, size_t N>
constexpr StaticArray<T, N> &StaticArray<T, N>::operator=(
    StaticArray<T, N> &&other)
{
    if (this != &other)
    {
        // TODO:
        // Likely branch on !std::is_constant_evaluated() and constexpr
        // std::is_trivially_copyable inside it for a memcpy version?

        for (size_t i = 0; i < N; ++i)
            m_data[i] = WHEELS_MOV(other.m_data[i]);
    }
    return *this;
}

template <typename T, size_t N>
constexpr T &StaticArray<T, N>::operator[](size_t i)
{
    return m_data[i];
}

template <typename T, size_t N>
constexpr T const &StaticArray<T, N>::operator[](size_t i) const
{
    return m_data[i];
}

template <typename T, size_t N> constexpr T *StaticArray<T, N>::data()
{
    return m_data;
}

template <typename T, size_t N>
constexpr T const *StaticArray<T, N>::data() const
{
    return m_data;
}

template <typename T, size_t N> constexpr T *StaticArray<T, N>::begin()
{
    return m_data;
}

template <typename T, size_t N>
constexpr T const *StaticArray<T, N>::begin() const
{
    return m_data;
}

template <typename T, size_t N> constexpr T *StaticArray<T, N>::end()
{
    return m_data + N;
}

template <typename T, size_t N>
constexpr T const *StaticArray<T, N>::end() const
{
    return m_data + N;
}

template <typename T, size_t N> StaticArray<T, N>::operator Span<T>()
{
    return Span{m_data, N};
}

template <typename T, size_t N>
StaticArray<T, N>::operator Span<T const>() const
{
    return Span{m_data, N};
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_STATIC_ARRAY_HPP
