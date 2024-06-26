#ifndef WHEELS_CONTAINERS_PAIR_HPP
#define WHEELS_CONTAINERS_PAIR_HPP

#include "../utils.hpp"

#include "concepts.hpp"

namespace wheels
{

template <typename T, typename V> struct Pair
{
    using first_type = T;
    using second_type = V;

    T first;
    V second;

    constexpr Pair() noexcept
        requires(std::default_initializable<T> && std::default_initializable<V>)
    = default;

    // Let's be pedantic and disallow implicit conversions
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    constexpr Pair(U &&first, W &&second) noexcept;
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    constexpr Pair(Pair<U, W> &&other) noexcept;
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    constexpr Pair<T, V> &operator=(Pair<U, W> &&other) noexcept;
};

template <typename T, typename V>
[[nodiscard]] constexpr Pair<T, V> make_pair(T &&first, V &&second)
{
    return Pair<T, V>{WHEELS_FWD(first), WHEELS_FWD(second)};
}

template <typename T, typename V> Pair(T &&first, V &&second) -> Pair<T, V>;

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
constexpr Pair<T, V>::Pair(U &&first, W &&second) noexcept
: first{WHEELS_FWD(first)}
, second{WHEELS_FWD(second)} {};

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
constexpr Pair<T, V>::Pair(Pair<U, W> &&other) noexcept
: first{WHEELS_MOV(other.first)}
, second{WHEELS_MOV(other.second)} {};

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
constexpr Pair<T, V> &Pair<T, V>::operator=(Pair<U, W> &&other) noexcept
{
    if (this != &other)
    {
        first = WHEELS_MOV(other.first);
        second = WHEELS_MOV(other.second);
    }
    return *this;
}

template <typename T, typename V>
[[nodiscard]] constexpr bool operator==(
    Pair<T, V> const &lhs, Pair<T, V> const &rhs) noexcept
{
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

template <typename T, typename V>
[[nodiscard]] constexpr bool operator!=(
    Pair<T, V> const &lhs, Pair<T, V> const &rhs) noexcept
{
    return lhs.first != rhs.first || lhs.second != rhs.second;
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_PAIR_HPP
