
#ifndef WHEELS_CONTAINERS_SPAN_HPP
#define WHEELS_CONTAINERS_SPAN_HPP

#include "../assert.hpp"
#include "concepts.hpp"
#include "optional.hpp"

#include <cstddef>
#include <cstring>

namespace wheels
{

template <typename T> class Span
{
  public:
    using value_type = T;

    constexpr Span() noexcept = default;
    constexpr Span(T *ptr, size_t size) noexcept;

    constexpr Span(Span<T> const &other) noexcept = default;
    constexpr Span &operator=(Span<T> const &other) noexcept = default;

    [[nodiscard]] constexpr T &operator[](size_t i) noexcept;
    [[nodiscard]] constexpr T const &operator[](size_t i) const noexcept;
    [[nodiscard]] constexpr T *data() noexcept;
    [[nodiscard]] constexpr T const *data() const noexcept;

    [[nodiscard]] constexpr T *begin() noexcept;
    [[nodiscard]] constexpr T const *begin() const noexcept;
    [[nodiscard]] constexpr T *end() noexcept;
    [[nodiscard]] constexpr T const *end() const noexcept;

    [[nodiscard]] constexpr bool empty() const noexcept;
    [[nodiscard]] constexpr size_t size() const noexcept;

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
    constexpr operator Span<T const>() const noexcept;

  protected:
    T *m_data{nullptr};
    size_t m_size{0};
};

class StrSpan : public Span<char const>
{
  public:
    constexpr StrSpan() noexcept = default;
    constexpr StrSpan(char const *str) noexcept;
    constexpr StrSpan(char const *str, size_t size) noexcept;

    [[nodiscard]] Optional<size_t> find_first(StrSpan substr) const noexcept;
    [[nodiscard]] Optional<size_t> find_first(
        char const *substr) const noexcept;
    [[nodiscard]] Optional<size_t> find_first(char ch) const noexcept;

    [[nodiscard]] Optional<size_t> find_last(StrSpan substr) const noexcept;
    [[nodiscard]] Optional<size_t> find_last(char const *substr) const noexcept;
    [[nodiscard]] Optional<size_t> find_last(char ch) const noexcept;

    [[nodiscard]] bool contains(StrSpan substr) const noexcept;
    [[nodiscard]] bool contains(char const *substr) const noexcept;
    [[nodiscard]] bool contains(char ch) const noexcept;

    [[nodiscard]] bool starts_with(StrSpan substr) const noexcept;
    [[nodiscard]] bool starts_with(char const *substr) const noexcept;
    [[nodiscard]] bool starts_with(char ch) const noexcept;

    [[nodiscard]] bool ends_with(StrSpan substr) const noexcept;
    [[nodiscard]] bool ends_with(char const *substr) const noexcept;
    [[nodiscard]] bool ends_with(char ch) const noexcept;
};

template <typename T>
constexpr Span<T>::Span(T *ptr, size_t size) noexcept
: m_data{ptr}
, m_size{size}
{
    // Let's catch weird input values, but allow explicitly empty spans
    WHEELS_ASSERT(m_data != nullptr || m_size == 0);
}

template <typename T> constexpr T &Span<T>::operator[](size_t i) noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T>
constexpr T const &Span<T>::operator[](size_t i) const noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> constexpr T *Span<T>::data() noexcept { return m_data; }

template <typename T> constexpr T const *Span<T>::data() const noexcept
{
    return m_data;
}

template <typename T> constexpr T *Span<T>::begin() noexcept { return m_data; }

template <typename T> constexpr T const *Span<T>::begin() const noexcept
{
    return m_data;
}

template <typename T> constexpr T *Span<T>::end() noexcept
{
    return m_data + m_size;
}

template <typename T> constexpr T const *Span<T>::end() const noexcept
{
    return m_data + m_size;
}

template <typename T> constexpr bool Span<T>::empty() const noexcept
{
    return m_size == 0;
}

template <typename T> constexpr size_t Span<T>::size() const noexcept
{
    return m_size;
}

template <typename T> constexpr Span<T>::operator Span<T const>() const noexcept
{
    return Span<T const>{m_data, m_size};
}

// Compires the entire spans, no special handling for e.g. trailing nulls in
// Span<char>. Agnostic to inner const.
template <typename T, typename V>
    requires SameAs<T, V>
bool operator==(Span<T> lhs, Span<V> rhs) noexcept
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
    requires SameAs<T, V>
bool operator!=(Span<T> lhs, Span<T> rhs) noexcept
{
    return !(lhs == rhs);
}

inline constexpr StrSpan::StrSpan(char const *str) noexcept
{
    m_data = str;
    m_size = 0;
    // strlen isn't constexpr
    while (str[m_size] != '\0')
        m_size++;
}

inline constexpr StrSpan::StrSpan(char const *str, size_t size) noexcept
: Span{str, size}
{
}

inline Optional<size_t> StrSpan::find_first(StrSpan substr) const noexcept
{
    if (m_size < substr.m_size)
        return {};

    size_t subs_i = 0;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_data[i] == substr.m_data[subs_i])
        {
            if (subs_i == substr.m_size - 1)
                return {i + 1 - substr.m_size};
            subs_i++;
        }
        else
            subs_i = 0;
    }
    return {};
}

inline Optional<size_t> StrSpan::find_first(char const *substr) const noexcept
{
    return find_first(StrSpan{substr});
}

inline Optional<size_t> StrSpan::find_first(char ch) const noexcept
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_data[i] == ch)
            return {i};
    }
    return {};
}

inline Optional<size_t> StrSpan::find_last(StrSpan substr) const noexcept
{
    if (m_size < substr.m_size)
        return {};

    size_t subs_i = substr.m_size;
    for (size_t i = m_size; i > 0; --i)
    {
        if (m_data[i - 1] == substr.m_data[subs_i - 1])
        {
            if (subs_i == 1)
                return Optional{i - 1};
            subs_i--;
        }
        else
            subs_i = substr.m_size;
    }
    return {};
}

inline Optional<size_t> StrSpan::find_last(char const *substr) const noexcept
{
    return find_last(StrSpan{substr});
}

inline Optional<size_t> StrSpan::find_last(char ch) const noexcept
{
    for (size_t i = m_size; i > 0; --i)
    {
        if (m_data[i - 1] == ch)
            return Optional{i - 1};
    }
    return {};
}

inline bool StrSpan::contains(StrSpan substr) const noexcept
{
    return find_first(substr).has_value();
}

inline bool StrSpan::contains(char const *substr) const noexcept
{
    return find_first(substr).has_value();
}

inline bool StrSpan::contains(char ch) const noexcept
{
    return find_first(ch).has_value();
}

inline bool StrSpan::starts_with(StrSpan substr) const noexcept
{
    if (substr.m_size == 0)
        return false;

    if (m_size < substr.m_size)
        return false;

    return memcmp(substr.m_data, m_data, substr.m_size) == 0;
}

inline bool StrSpan::starts_with(char const *substr) const noexcept
{
    return starts_with(StrSpan{substr});
}

inline bool StrSpan::starts_with(char ch) const noexcept
{
    if (m_size == 0)
        return false;

    return m_data[0] == ch;
}

inline bool StrSpan::ends_with(StrSpan substr) const noexcept
{
    if (substr.m_size == 0)
        return false;

    if (m_size < substr.m_size)
        return false;

    return memcmp(
               substr.m_data, m_data + m_size - substr.m_size, substr.m_size) ==
           0;
}

inline bool StrSpan::ends_with(char const *substr) const noexcept
{
    return ends_with(StrSpan{substr});
}

inline bool StrSpan::ends_with(char ch) const noexcept
{
    if (m_size == 0)
        return false;

    return m_data[m_size - 1] == ch;
}

// Returns true if the spans are equal as c-strings, treating data()[size()] as
// \0. Compares the spans assuming they are views into c-strings exluding the
// final null. This includes the assumption that the character at data()[size()]
// is allocated and may or may not be \0, depending on the span being a full
// view into the string or not.
inline bool operator==(StrSpan lhs, StrSpan rhs) noexcept
{
    if (lhs.data() == rhs.data() && lhs.size() == rhs.size())
        return true;

    // empty == empty was matched above
    if (lhs.data() == nullptr || rhs.data() == nullptr)
        return false;

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

inline bool operator!=(StrSpan lhs, StrSpan rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_SPAN_HPP
