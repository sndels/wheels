#ifndef WHEELS_PAIR_HPP
#define WHEELS_PAIR_HPP

namespace wheels
{

template <typename T, typename V> struct Pair
{
    T first;
    V second;

    Pair(T const &first, V const &second)
    : first{first}
    , second{second} {};
};

template <typename T, typename V> Pair<T, V> make_pair(T &&first, V &&second)
{
    // static_cast<T&&>(...) is std::forward<T>(...)
    return Pair<T, V>{static_cast<T &&>(first), static_cast<V &&>(second)};
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