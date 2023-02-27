#ifndef WHEELS_PAIR_HPP
#define WHEELS_PAIR_HPP

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
    // static_cast<T&&>(...) is std::forward<T>(...)
    return Pair<T, V>{static_cast<T &&>(first), static_cast<V &&>(second)};
}

template <typename T, typename V>
Pair<T, V>::Pair(T &&first, V &&second)
: first{static_cast<T &&>(first)}
, second{static_cast<V &&>(second)} {};

template <typename T, typename V>
Pair<T, V>::Pair(Pair<T, V> const &other)
: first{other.first}
, second{other.second} {};

template <typename T, typename V>
Pair<T, V>::Pair(Pair<T, V> &&other)
: first{std::move(other.first)}
, second{std::move(other.second)} {};

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

        first = std::move(other.first);
        second = std::move(other.second);
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