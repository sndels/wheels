#ifndef WHEELS_CONTAINERS_PAIR_HPP
#define WHEELS_CONTAINERS_PAIR_HPP

#include "utils.hpp"

#include "concepts.hpp"

namespace wheels
{

template <typename T, typename V> struct Pair
{
    T first;
    V second;

    // Let's be pedantic and disallow implicit conversions
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    Pair(U &&first, W &&second);
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    Pair(Pair<U, W> &&other);
    template <typename U, typename W>
        requires(SameAs<U, T> && SameAs<W, V>)
    Pair<T, V> &operator=(Pair<U, W> &&other);
};

template <typename T, typename V> Pair<T, V> make_pair(T &&first, V &&second)
{
    return Pair<T, V>{WHEELS_FWD(first), WHEELS_FWD(second)};
}

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
Pair<T, V>::Pair(U &&first, W &&second)
: first{WHEELS_FWD(first)}
, second{WHEELS_FWD(second)} {};

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
Pair<T, V>::Pair(Pair<U, W> &&other)
: first{WHEELS_MOV(other.first)}
, second{WHEELS_MOV(other.second)} {};

template <typename T, typename V>
template <typename U, typename W>
    requires(SameAs<U, T> && SameAs<W, V>)
Pair<T, V> &Pair<T, V>::operator=(Pair<U, W> &&other)
{
    if (this != &other)
    {
        first = WHEELS_MOV(other.first);
        second = WHEELS_MOV(other.second);
    }
    return *this;
}

template <typename T, typename V>
bool operator==(Pair<T, V> const &lhs, Pair<T, V> const &rhs)
{
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

template <typename T, typename V>
bool operator!=(Pair<T, V> const &lhs, Pair<T, V> const &rhs)
{
    return lhs.first != rhs.first || lhs.second != rhs.second;
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_PAIR_HPP