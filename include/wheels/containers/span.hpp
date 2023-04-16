
#ifndef WHEELS_CONTAINERS_SPAN_HPP
#define WHEELS_CONTAINERS_SPAN_HPP

#include "concepts.hpp"

#include <cstddef>

namespace wheels
{

template <typename T> class Span
{
  public:
    Span(T *ptr, size_t size);

    Span(Span<T> const &other) = default;
    Span &operator=(Span<T> const &other) = default;

    [[nodiscard]] T &operator[](size_t i);
    [[nodiscard]] T const &operator[](size_t i) const;
    [[nodiscard]] T *data();
    [[nodiscard]] T const *data() const;

    [[nodiscard]] T *begin();
    [[nodiscard]] T const *begin() const;
    [[nodiscard]] T *end();
    [[nodiscard]] T const *end() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;

    operator Span<T const>() const;

  private:
    T *m_data{nullptr};
    size_t m_size{0};
};

template <typename T>
Span<T>::Span(T *ptr, size_t size)
: m_data{ptr}
, m_size{size}
{
    assert(m_data != nullptr);
}

template <typename T> T &Span<T>::operator[](size_t i)
{
    assert(i < m_size);
    return m_data[i];
}

template <typename T> T const &Span<T>::operator[](size_t i) const
{
    assert(i < m_size);
    return m_data[i];
}

template <typename T> T *Span<T>::data() { return m_data; }

template <typename T> T const *Span<T>::data() const { return m_data; }

template <typename T> T *Span<T>::begin() { return m_data; }

template <typename T> T const *Span<T>::begin() const { return m_data; }

template <typename T> T *Span<T>::end() { return m_data + m_size; }

template <typename T> T const *Span<T>::end() const { return m_data + m_size; }

template <typename T> bool Span<T>::empty() const { return m_size == 0; }

template <typename T> size_t Span<T>::size() const { return m_size; }

template <typename T> Span<T>::operator Span<T const>() const
{
    return Span<T const>{m_data, m_size};
}

// Compires the entire spans, no special handling for e.g. trailing nulls in
// Span<char>. Agnostic to inner const.
template <typename T, typename V>
    requires SameAs<T, V> bool
operator==(Span<T> lhs, Span<V> rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<T>)
        return memcmp(lhs.data(), rhs.data(), lhs.size() * sizeof(T)) == 0;
    else
    {
        size_t const size = lhs.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (lhs.data()[i] != rhs.data()[i])
                return false;
        }
    }

    return true;
}

// Compires the entire spans, no special handling for e.g. trailing nulls in
// Span<char>. Agnostic to inner const.
template <typename T, typename V>
    requires SameAs<T, V> bool
operator!=(Span<T> lhs, Span<T> rhs)
{
    return !(lhs == rhs);
}

// Returns true if the spans are equal as c-strings, treating data()[size()] as
// \0. Compares the spans assuming they are views into c-strings exluding the
// final null. This includes the assumption that the character at data()[size()]
// is allocated and may or may not be \0, depending on the span being a full
// view into the string or not.
template <typename T, typename V>
    requires(SameAs<T, char> && SameAs<V, char>) bool
compare_str(Span<T> lhs, Span<V> rhs)
{
    size_t first_len = 0;
    size_t second_len = 0;
    char const *first_ptr = nullptr;
    char const *second_ptr = nullptr;
    if (lhs.size() < rhs.size())
    {
        first_len = lhs.size();
        second_len = rhs.size();
        first_ptr = lhs.data();
        second_ptr = rhs.data();
    }
    else
    {
        first_len = rhs.size();
        second_len = lhs.size();
        first_ptr = rhs.data();
        second_ptr = lhs.data();
    }

    // Compare by the shorter span
    if (strncmp(first_ptr, second_ptr, first_len) != 0)
        return false;

    // Make sure the spans are the same size of the longer span ends in null
    // after the common part
    return first_len == second_len || second_ptr[first_len] == '\0';
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_SPAN_HPP