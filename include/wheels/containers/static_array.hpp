
#ifndef WHEELS_CONTAINERS_STATIC_ARRAY_HPP
#define WHEELS_CONTAINERS_STATIC_ARRAY_HPP

#include "../assert.hpp"
#include "../utils.hpp"
#include "span.hpp"

#include <cstring>
#include <initializer_list>

namespace wheels
{

template <typename T>
concept StaticArrayRequirements = std::is_default_constructible_v<T> &&
                                  (std::is_copy_assignable_v<T> ||
                                   std::is_move_assignable_v<T>);

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
class StaticArray
{
  public:
    constexpr StaticArray() = default;
    constexpr StaticArray(T const &default_value)
        requires std::is_copy_assignable_v<T>;
    constexpr StaticArray(T const (&elems)[N])
        requires std::is_copy_assignable_v<T>;
    // No std::initializer_list ctor because constexpr can't validate
    // list.size() == N. Also no variadric template ctor because that will fail
    // type deduction in some cases. Let's just always use the array ctor for
    // consistency and safety.
    constexpr ~StaticArray() = default;

    constexpr StaticArray(StaticArray<T, N> const &other)
        requires std::is_copy_assignable_v<T>;
    constexpr StaticArray(StaticArray<T, N> &&other)
        requires std::is_move_assignable_v<T>;
    constexpr StaticArray<T, N> &operator=(StaticArray<T, N> const &other)
        requires std::is_copy_assignable_v<T>;
    constexpr StaticArray<T, N> &operator=(StaticArray<T, N> &&other)
        requires std::is_move_assignable_v<T>;

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

    constexpr operator Span<T>();
    constexpr operator Span<T const>() const;

  private:
    constexpr void init(T const *source);

    // Default ctor zero initializes
    T m_data[N]{};
};

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(T const &default_value)
    requires std::is_copy_assignable_v<T>
{
    for (size_t i = 0; i < N; ++i)
        m_data[i] = default_value;
}

// Deduction from raw arrays
template <typename T, size_t N>
    requires StaticArrayRequirements<T>
StaticArray(T const (&)[N]) -> StaticArray<T, N>;
template <typename T> StaticArray(T const &) -> StaticArray<T, 1>;

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(T const (&elems)[N])
    requires std::is_copy_assignable_v<T>
{
    init(elems);
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(StaticArray<T, N> const &other)
    requires std::is_copy_assignable_v<T>
{
    init(other.m_data);
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(StaticArray<T, N> &&other)
    requires std::is_move_assignable_v<T>
{
    // No constexpr since that forces const evaluated, but mark likely for
    // runtime that always takes the else
    if (!std::is_constant_evaluated()) [[likely]]
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(m_data, other.m_data, N * sizeof(T));
            return;
        }
    }

    for (size_t i = 0; i < N; ++i)
        m_data[i] = WHEELS_MOV(other.m_data[i]);
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N> &StaticArray<T, N>::operator=(
    StaticArray<T, N> const &other)
    requires std::is_copy_assignable_v<T>
{
    if (this != &other)
        init(other.m_data);

    return *this;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N> &StaticArray<T, N>::operator=(
    StaticArray<T, N> &&other)
    requires std::is_move_assignable_v<T>
{
    if (this != &other)
    {
        // No constexpr since that forces const evaluated, but mark likely for
        // runtime that always takes the else
        if (!std::is_constant_evaluated()) [[likely]]
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                memcpy(m_data, other.m_data, N * sizeof(T));
                return *this;
            }
        }

        for (size_t i = 0; i < N; ++i)
            m_data[i] = WHEELS_MOV(other.m_data[i]);
    }
    return *this;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T &StaticArray<T, N>::operator[](size_t i)
{
    return m_data[i];
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const &StaticArray<T, N>::operator[](size_t i) const
{
    return m_data[i];
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::data()
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::data() const
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::begin()
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::begin() const
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::end()
{
    return m_data + N;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::end() const
{
    return m_data + N;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::operator Span<T>()
{
    return Span{m_data, N};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::operator Span<T const>() const
{
    return Span{m_data, N};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr void StaticArray<T, N>::init(T const *src)
{
    // No constexpr since that forces const evaluated, but mark likely for
    // runtime that always takes this
    if (!std::is_constant_evaluated()) [[likely]]
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            memcpy(m_data, src, N * sizeof(T));
            return;
        }
    }

    for (size_t i = 0; i < N; ++i)
        m_data[i] = src[i];
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_STATIC_ARRAY_HPP
