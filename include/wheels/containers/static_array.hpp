
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
concept StaticArrayRequirements =
    std::is_default_constructible_v<T> &&
    (std::is_copy_assignable_v<T> || std::is_move_assignable_v<T>);

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
class StaticArray
{
  public:
    using value_type = T;

    constexpr StaticArray() noexcept = default;
    constexpr explicit StaticArray(T const &default_value) noexcept
        requires std::is_copy_assignable_v<T>;
    template <size_t K>
    constexpr StaticArray(T const (&elems)[K]) noexcept
        requires std::is_copy_assignable_v<T>;
    // No std::initializer_list ctor because constexpr can't validate
    // list.size() == N. Also no variadric template ctor because that will fail
    // type deduction in some cases. Let's just always use the array ctor for
    // consistency and safety.
    constexpr ~StaticArray() = default;

    constexpr StaticArray(StaticArray<T, N> const &other) noexcept
        requires std::is_copy_assignable_v<T>;
    constexpr StaticArray(StaticArray<T, N> &&other) noexcept
        requires std::is_move_assignable_v<T>;
    constexpr StaticArray<T, N> &operator=(
        StaticArray<T, N> const &other) noexcept
        requires std::is_copy_assignable_v<T>;
    constexpr StaticArray<T, N> &operator=(StaticArray<T, N> &&other) noexcept
        requires std::is_move_assignable_v<T>;

    [[nodiscard]] constexpr T &operator[](size_t i) noexcept;
    [[nodiscard]] constexpr T const &operator[](size_t i) const noexcept;
    [[nodiscard]] constexpr T *data() noexcept;
    [[nodiscard]] constexpr T const *data() const noexcept;

    [[nodiscard]] constexpr T *begin() noexcept;
    [[nodiscard]] constexpr T const *begin() const noexcept;
    [[nodiscard]] constexpr T *end() noexcept;
    [[nodiscard]] constexpr T const *end() const noexcept;

    // Template type inference can't seem to follow T with the inner const so
    // let's have explicit methods for const and mutable spans

    [[nodiscard]] Span<T> mut_span() noexcept;
    [[nodiscard]] Span<T const> span() const noexcept;
    [[nodiscard]] Span<T> mut_span(size_t begin_i, size_t end_i) noexcept;
    [[nodiscard]] Span<T const> span(
        size_t begin_i, size_t end_i) const noexcept;

    [[nodiscard]] static constexpr size_t size() noexcept { return N; };
    [[nodiscard]] static constexpr size_t capacity() noexcept { return N; }

    constexpr operator Span<T const>() const noexcept;

  private:
    constexpr void init(T const *source) noexcept;

    // Default ctor zero initializes
    T m_data[N]{};
};

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(T const &default_value) noexcept
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
template <size_t K>
constexpr StaticArray<T, N>::StaticArray(T const (&elems)[K]) noexcept
    requires std::is_copy_assignable_v<T>
{
    // Need to use a separate K because at least MSVC manages to miss N != N if
    // elems is [N]
    // Use a static assert instead of requires for a nicer error message
    static_assert(K == N, "Initializer element count doesn't match array size");
    init(elems);
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(
    StaticArray<T, N> const &other) noexcept
    requires std::is_copy_assignable_v<T>
{
    init(other.m_data);
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::StaticArray(StaticArray<T, N> &&other) noexcept
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
    StaticArray<T, N> const &other) noexcept
    requires std::is_copy_assignable_v<T>
{
    if (this != &other)
        init(other.m_data);

    return *this;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N> &StaticArray<T, N>::operator=(
    StaticArray<T, N> &&other) noexcept
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
constexpr T &StaticArray<T, N>::operator[](size_t i) noexcept
{
    return m_data[i];
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const &StaticArray<T, N>::operator[](size_t i) const noexcept
{
    return m_data[i];
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::data() noexcept
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::data() const noexcept
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::begin() noexcept
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::begin() const noexcept
{
    return m_data;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T *StaticArray<T, N>::end() noexcept
{
    return m_data + N;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr T const *StaticArray<T, N>::end() const noexcept
{
    return m_data + N;
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
Span<T> StaticArray<T, N>::mut_span() noexcept
{
    return Span{begin(), N};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
Span<T const> StaticArray<T, N>::span() const noexcept
{
    return Span<T const>{begin(), N};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
Span<T> StaticArray<T, N>::mut_span(size_t begin_i, size_t end_i) noexcept
{
    WHEELS_ASSERT(begin_i < N);
    WHEELS_ASSERT(end_i <= N);
    return Span{begin() + begin_i, end_i - begin_i};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
Span<T const> StaticArray<T, N>::span(
    size_t begin_i, size_t end_i) const noexcept
{
    WHEELS_ASSERT(begin_i < N);
    WHEELS_ASSERT(end_i <= N);
    return Span<T const>{begin() + begin_i, end_i - begin_i};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr StaticArray<T, N>::operator Span<T const>() const noexcept
{
    return Span{m_data, N};
}

template <typename T, size_t N>
    requires StaticArrayRequirements<T>
constexpr void StaticArray<T, N>::init(T const *src) noexcept
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
