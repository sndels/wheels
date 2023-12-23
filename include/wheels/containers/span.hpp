
#ifndef WHEELS_CONTAINERS_SPAN_HPP
#define WHEELS_CONTAINERS_SPAN_HPP

#include "../assert.hpp"
#include "concepts.hpp"

#include <cstddef>
#include <cstring>

namespace wheels
{

template <typename T> class Span
{
  public:
    constexpr Span() = default;
    constexpr Span(T *ptr, size_t size);

    constexpr Span(Span<T> const &other) = default;
    constexpr Span &operator=(Span<T> const &other) = default;

    [[nodiscard]] constexpr T &operator[](size_t i);
    [[nodiscard]] constexpr T const &operator[](size_t i) const;
    [[nodiscard]] constexpr T *data();
    [[nodiscard]] constexpr T const *data() const;

    [[nodiscard]] constexpr T *begin();
    [[nodiscard]] constexpr T const *begin() const;
    [[nodiscard]] constexpr T *end();
    [[nodiscard]] constexpr T const *end() const;

    [[nodiscard]] constexpr bool empty() const;
    [[nodiscard]] constexpr size_t size() const;

    // Converting from non-const inner type to const inner type should be really
    // cheap so let's just have it. Spans to non-const things should be
    // compatible with interfaces that take spans to const things. Not having an
    // implicit conversion requires either
    //   a) templating the interfaces which makes the compiler miss implicit
    //      conversions from a container into a span
    // or
    //   b) mirroring explicit const and non-const versions of interfaces
    // Both other options seem unnecessarily complex unless this ends up being
    // expensive in some cases.
    constexpr operator Span<T const>() const;

  protected:
    T *m_data{nullptr};
    size_t m_size{0};
};

class StrSpan : public Span<char const>
{
  public:
    constexpr StrSpan(char const *str);
    constexpr StrSpan(char const *str, size_t size);
};

template <typename T>
constexpr Span<T>::Span(T *ptr, size_t size)
: m_data{ptr}
, m_size{size}
{
    WHEELS_ASSERT(m_data != nullptr);
}

template <typename T> constexpr T &Span<T>::operator[](size_t i)
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> constexpr T const &Span<T>::operator[](size_t i) const
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> constexpr T *Span<T>::data() { return m_data; }

template <typename T> constexpr T const *Span<T>::data() const
{
    return m_data;
}

template <typename T> constexpr T *Span<T>::begin() { return m_data; }

template <typename T> constexpr T const *Span<T>::begin() const
{
    return m_data;
}

template <typename T> constexpr T *Span<T>::end() { return m_data + m_size; }

template <typename T> constexpr T const *Span<T>::end() const
{
    return m_data + m_size;
}

template <typename T> constexpr bool Span<T>::empty() const
{
    return m_size == 0;
}

template <typename T> constexpr size_t Span<T>::size() const { return m_size; }

template <typename T> constexpr Span<T>::operator Span<T const>() const
{
    return Span<T const>{m_data, m_size};
}

// Compires the entire spans, no special handling for e.g. trailing nulls in
// Span<char>. Agnostic to inner const.
template <typename T, typename V>
    requires SameAs<T, V> bool
operator==(Span<T> lhs, Span<V> rhs)
{
    if (lhs.data() == rhs.data() && lhs.size() == rhs.size())
        return true;

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

inline constexpr StrSpan::StrSpan(char const *str)
{
    m_data = str;
    m_size = 0;
    // strlen isn't constexpr
    while (str[m_size] != '\0')
        m_size++;
}

inline constexpr StrSpan::StrSpan(char const *str, size_t size)
: Span{str, size}
{
}

// Returns true if the spans are equal as c-strings, treating data()[size()] as
// \0. Compares the spans assuming they are views into c-strings exluding the
// final null. This includes the assumption that the character at data()[size()]
// is allocated and may or may not be \0, depending on the span being a full
// view into the string or not.
inline bool operator==(StrSpan lhs, StrSpan rhs)
{
    if (lhs.data() == rhs.data() && lhs.size() == rhs.size())
        return true;

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

inline bool operator!=(StrSpan lhs, StrSpan rhs) { return !(lhs == rhs); }

} // namespace wheels

#endif // WHEELS_CONTAINERS_SPAN_HPP
