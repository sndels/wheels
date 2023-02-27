#ifndef WHEELS_PAIR_HPP
#define WHEELS_PAIR_HPP

#include "container_utils.hpp"

namespace wheels
{

template <typename T, typename V> struct Pair
{
    T first;
    V second;

    Pair(T &&first, V &&second);

    Pair(Pair<T, V> const &other);
    Pair(Pair<T, V> &&other);
    Pair<T, V> &operator=(Pair<T, V> const &other);
    Pair<T, V> &operator=(Pair<T, V> &&other);
};

template <typename T, typename V> Pair<T, V> make_pair(T &&first, V &&second)
{
    return Pair<T, V>{WHEELS_FWD(first), WHEELS_FWD(second)};
}

template <typename T, typename V>
Pair<T, V>::Pair(T &&first, V &&second)
: first{WHEELS_FWD(first)}
, second{WHEELS_FWD(second)} {};

template <typename T, typename V>
Pair<T, V>::Pair(Pair<T, V> const &other)
: first{other.first}
, second{other.second} {};

template <typename T, typename V>
Pair<T, V>::Pair(Pair<T, V> &&other)
: first{WHEELS_MOV(other.first)}
, second{WHEELS_MOV(other.second)} {};

template <typename T, typename V>
Pair<T, V> &Pair<T, V>::operator=(Pair<T, V> const &other)
{
    if (this != &other)
    {
        first = other.first;
        second = other.second;
    }
    return *this;
}

template <typename T, typename V>
Pair<T, V> &Pair<T, V>::operator=(Pair<T, V> &&other)
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

#endif // WHEELS_PAIR_HPP