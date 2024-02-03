
#ifndef WHEELS_CONTAINERS_STRING_HPP
#define WHEELS_CONTAINERS_STRING_HPP

#include "../allocators/allocator.hpp"
#include "../assert.hpp"
#include "array.hpp"
#include "hash.hpp"
#include "optional.hpp"
#include "utils.hpp"

#include <cstddef>
#include <cstring>

namespace wheels
{

// NOTE:
// All conversions, comparisons and find algorithms use the size of the String
// as the end limit. Explicitly appending/inserting \0s will cause the String to
// no longer behave like you would expect from a c-string.
// All spans are inner const for simplicity since having mutable spans seems
// unnecessary.

class String
{
  public:
    String(Allocator &allocator, size_t initial_capacity = 16) noexcept;
    String(Allocator &allocator, char const *str) noexcept;
    String(Allocator &allocator, StrSpan str) noexcept;
    ~String();

    String(String const &) = delete;
    String(String &&other) noexcept;
    String &operator=(String const &) = delete;
    String &operator=(String &&other) noexcept;

    [[nodiscard]] char &operator[](size_t i) noexcept;
    [[nodiscard]] char const &operator[](size_t i) const noexcept;
    [[nodiscard]] char &front() noexcept;
    [[nodiscard]] char const &front() const noexcept;
    [[nodiscard]] char &back() noexcept;
    [[nodiscard]] char const &back() const noexcept;
    [[nodiscard]] char *data() noexcept;
    [[nodiscard]] char const *data() const noexcept;
    [[nodiscard]] char *c_str() noexcept;
    [[nodiscard]] char const *c_str() const noexcept;

    [[nodiscard]] char *begin() noexcept;
    [[nodiscard]] char const *begin() const noexcept;
    [[nodiscard]] char *end() noexcept;
    [[nodiscard]] char const *end() const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    // Reserves capacity characters plus a final null
    void reserve(size_t capacity) noexcept;
    // Doesn't include the final null
    [[nodiscard]] size_t capacity() const noexcept;

    void clear() noexcept;

    void push_back(char ch) noexcept;
    char pop_back() noexcept;

    void resize(size_t size, char ch = '\0') noexcept;

    // No operator+ as it returns a new String and reusing the internal
    // allocator would make the lifetime of the new thing ambiguous. Use
    // concat() instead. Have extend() instead of operator+= to just not have
    // overloaded math operators.

    friend String concat(
        Allocator &allocator, StrSpan first, StrSpan second) noexcept;
    friend String concat(
        Allocator &allocator, StrSpan first, char const *second) noexcept;
    friend String concat(
        Allocator &allocator, char const *first, StrSpan second) noexcept;

    String &extend(StrSpan str) noexcept;
    String &extend(char const *str) noexcept;

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

    // Require explicit allocator for split for the same reason as concat()

    [[nodiscard]] Array<StrSpan> split(
        Allocator &allocator, StrSpan substr) const noexcept;
    [[nodiscard]] Array<StrSpan> split(
        Allocator &allocator, char const *substr) const noexcept;
    [[nodiscard]] Array<StrSpan> split(
        Allocator &allocator, char ch) const noexcept;

    operator StrSpan() const noexcept;
    StrSpan span(size_t begin, size_t end) const noexcept;

  private:
    void reallocate(size_t capacity) noexcept;
    void free() noexcept;

    [[nodiscard]] static StrSpan span(char const *str) noexcept;

    [[nodiscard]] static Optional<size_t> find_first(
        StrSpan from, StrSpan substr) noexcept;
    [[nodiscard]] static Optional<size_t> find_first(
        StrSpan from, char ch) noexcept;

    Allocator &m_allocator;
    char *m_data{nullptr};
    size_t m_capacity{0};
    size_t m_size{0};
};

inline String::String(Allocator &allocator, size_t initial_capacity) noexcept
: m_allocator{allocator}
{
    if (initial_capacity == 0)
        initial_capacity = 16;
    reallocate(initial_capacity + 1);
    m_data[0] = '\0';
}

inline String::String(Allocator &allocator, char const *str) noexcept
: m_allocator{allocator}
{
    size_t const len = strlen(str);

    reallocate(len + 1);

    memcpy(m_data, str, len);
    m_data[len] = '\0';

    m_size = len;
}

inline String::String(Allocator &allocator, StrSpan str) noexcept
: m_allocator{allocator}
{
    reallocate(str.size() + 1);

    memcpy(m_data, str.data(), str.size());
    m_data[str.size()] = '\0';

    m_size = str.size();
}

inline String::~String() { free(); }

inline String::String(String &&other) noexcept
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_capacity{other.m_capacity}
, m_size{other.m_size}
{
    other.m_data = nullptr;
}

inline String &String::operator=(String &&other) noexcept
{
    WHEELS_ASSERT(
        &m_allocator == &other.m_allocator &&
        "Move assigning a container with different allocators can lead to "
        "nasty bugs. Use the same allocator or copy the content instead.");

    if (this != &other)
    {
        free();

        m_data = other.m_data;
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        other.m_data = nullptr;
    }
    return *this;
}

inline char &String::operator[](size_t i) noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

inline char const &String::operator[](size_t i) const noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

inline char &String::front() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[0];
}

inline char const &String::front() const noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[0];
}

inline char &String::back() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
};

inline char const &String::back() const noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
};

inline char *String::data() noexcept { return m_data; }

inline char const *String::data() const noexcept { return m_data; }

inline char *String::c_str() noexcept { return m_data; }

inline char const *String::c_str() const noexcept { return m_data; }

inline char *String::begin() noexcept { return m_data; }

inline char const *String::begin() const noexcept { return m_data; }

inline char *String::end() noexcept { return m_data + m_size; }

inline char const *String::end() const noexcept { return m_data + m_size; }

inline bool String::empty() const noexcept { return m_size == 0; }

inline size_t String::size() const noexcept { return m_size; }

inline void String::reserve(size_t capacity) noexcept
{
    if (capacity + 1 > m_capacity)
        reallocate(capacity + 1);
}

inline size_t String::capacity() const noexcept
{
    // Don't count the extra null slot
    return m_capacity - 1;
}

inline void String::clear() noexcept
{
    m_data[0] = '\0';
    m_size = 0;
}

inline void String::push_back(char ch) noexcept
{
    if (m_size == m_capacity - 1)
        reallocate(m_capacity * 2);

    m_data[m_size++] = ch;
    m_data[m_size] = '\0';
}

inline char String::pop_back() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    m_size--;
    char const ret = m_data[m_size];
    m_data[m_size] = '\0';
    return ret;
}

inline void String::resize(size_t size, char ch) noexcept
{
    if (size <= m_size)
        m_size = size;
    else
    {
        reserve(size);

        memset(m_data + m_size, ch, size - m_size);
        m_size = size;
    }
    m_data[m_size] = '\0';
}

inline String &String::extend(StrSpan str) noexcept
{
    reserve(m_size + str.size());

    memcpy(m_data + m_size, str.data(), str.size());
    m_size = m_size + str.size();
    m_data[m_size] = '\0';

    return *this;
}

inline String &String::extend(char const *str) noexcept
{
    return extend(span(str));
}

inline Optional<size_t> String::find_first(StrSpan substr) const noexcept
{
    return find_first(StrSpan(*this), substr);
}

inline Optional<size_t> String::find_first(char const *substr) const noexcept
{
    return find_first(span(substr));
}

inline Optional<size_t> String::find_first(char ch) const noexcept
{
    return find_first(StrSpan{*this}, ch);
}

inline Optional<size_t> String::find_last(StrSpan substr) const noexcept
{
    if (m_size < substr.size())
        return {};

    size_t subs_i = substr.size();
    for (size_t i = m_size; i > 0; --i)
    {
        if (m_data[i - 1] == substr.data()[subs_i - 1])
        {
            if (subs_i == 1)
                return Optional{i - 1};
            subs_i--;
        }
        else
            subs_i = substr.size();
    }
    return {};
}

inline Optional<size_t> String::find_last(char const *substr) const noexcept
{
    return find_last(span(substr));
}

inline Optional<size_t> String::find_last(char ch) const noexcept
{
    for (size_t i = m_size; i > 0; --i)
    {
        if (m_data[i - 1] == ch)
            return Optional{i - 1};
    }
    return {};
}

inline bool String::contains(StrSpan substr) const noexcept
{
    return find_first(substr).has_value();
}

inline bool String::contains(char const *substr) const noexcept
{
    return find_first(substr).has_value();
}

inline bool String::contains(char ch) const noexcept
{
    return find_first(ch).has_value();
}

inline bool String::starts_with(StrSpan substr) const noexcept
{
    Optional<size_t> const found = find_first(substr);
    if (found.has_value())
        return *found == 0;

    return false;
}

inline bool String::starts_with(char const *substr) const noexcept
{
    return starts_with(span(substr));
}

inline bool String::starts_with(char ch) const noexcept
{
    Optional<size_t> const found = find_first(ch);
    if (found.has_value())
        return *found == 0;

    return false;
}

inline bool String::ends_with(StrSpan substr) const noexcept
{
    Optional<size_t> const found = find_last(substr);
    if (found.has_value())
        return *found == m_size - substr.size();

    return false;
}

inline bool String::ends_with(char const *substr) const noexcept
{
    return ends_with(span(substr));
}

inline bool String::ends_with(char ch) const noexcept
{
    Optional<size_t> const found = find_last(ch);
    if (found.has_value())
        return *found == m_size - 1;

    return false;
}

inline Array<StrSpan> String::split(
    Allocator &allocator, StrSpan substr) const noexcept
{
    Array<StrSpan> spans{allocator, 16};

    StrSpan remaining{m_data, m_size};
    Optional<size_t> found = find_first(remaining, substr);
    while (found.has_value())
    {
        // Don't add empty spans
        if (*found > 0)
            spans.emplace_back(remaining.data(), *found);

        size_t const offset = *found + substr.size();
        remaining =
            StrSpan{remaining.data() + offset, remaining.size() - offset};
        found = find_first(remaining, substr);
    }

    if (remaining.size() > 0)
        spans.push_back(remaining);

    return spans;
}

inline Array<StrSpan> String::split(
    Allocator &allocator, char const *substr) const noexcept
{
    return split(allocator, span(substr));
}

inline Array<StrSpan> String::split(
    Allocator &allocator, char ch) const noexcept
{
    Array<StrSpan> spans{allocator, 16};

    StrSpan remaining{m_data, m_size};
    Optional<size_t> found = find_first(remaining, ch);
    while (found.has_value())
    {
        // Don't add empty spans
        if (*found > 0)
            spans.emplace_back(remaining.data(), *found);

        size_t const offset = *found + 1;
        remaining =
            StrSpan{remaining.data() + offset, remaining.size() - offset};
        found = find_first(remaining, ch);
    }

    if (remaining.size() > 0)
        spans.push_back(remaining);

    return spans;
}

inline void String::reallocate(size_t capacity) noexcept
{
    char *data = (char *)m_allocator.allocate(capacity * sizeof(char));
    WHEELS_ASSERT(data != nullptr);

    if (m_data != nullptr)
    {
        memcpy(data, m_data, m_size * sizeof(char));
        m_allocator.deallocate(m_data);
    }

    m_data = data;
    m_capacity = capacity;
}

inline void String::free() noexcept
{
    if (m_data != nullptr)
    {
        m_allocator.deallocate(m_data);
        m_data = nullptr;
    }
}

inline StrSpan String::span(char const *str) noexcept { return StrSpan{str}; }

inline Optional<size_t> String::find_first(
    StrSpan from, StrSpan substr) noexcept
{
    if (from.size() < substr.size())
        return {};

    size_t subs_i = 0;
    size_t const size = from.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (from.data()[i] == substr.data()[subs_i])
        {
            if (subs_i == substr.size() - 1)
                return {i + 1 - substr.size()};
            subs_i++;
        }
        else
            subs_i = 0;
    }
    return {};
}

inline Optional<size_t> String::find_first(StrSpan from, char ch) noexcept
{
    size_t const size = from.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (from.data()[i] == ch)
            return {i};
    }
    return {};
}

inline String::operator StrSpan() const noexcept
{
    return StrSpan{m_data, m_size};
}

inline StrSpan String::span(size_t begin, size_t end) const noexcept
{
    WHEELS_ASSERT(begin <= end);
    WHEELS_ASSERT(end <= m_size);
    return StrSpan{m_data + begin, end - begin};
}

static inline bool operator==(String const &lhs, String const &rhs) noexcept
{
    return StrSpan{lhs.c_str(), lhs.size()} == StrSpan{rhs.c_str(), rhs.size()};
}

static inline bool operator!=(String const &lhs, String const &rhs) noexcept
{
    return !(lhs == rhs);
}

static inline bool operator==(String const &lhs, char const *rhs) noexcept
{
    return StrSpan{lhs.c_str(), lhs.size()} == StrSpan{rhs};
}

static inline bool operator!=(String const &lhs, char const *rhs) noexcept
{
    return !(lhs == rhs);
}

static inline bool operator==(char const *lhs, String const &rhs) noexcept
{
    return StrSpan{lhs} == StrSpan{rhs.c_str(), rhs.size()};
}

static inline bool operator!=(char const *lhs, String const &rhs) noexcept
{
    return !(lhs == rhs);
}

inline String concat(
    Allocator &allocator, StrSpan first, StrSpan second) noexcept
{
    String ret{allocator, first.size() + second.size()};

    memcpy(ret.m_data, first.data(), first.size());
    memcpy(ret.m_data + first.size(), second.data(), second.size());

    ret.m_size = first.size() + second.size();
    ret.m_data[ret.m_size] = '\0';

    return ret;
}

inline String concat(
    Allocator &allocator, StrSpan first, char const *second) noexcept
{
    return concat(allocator, first, String::span(second));
}

inline String concat(
    Allocator &allocator, char const *first, StrSpan second) noexcept
{
    return concat(allocator, String::span(first), second);
}

template <> struct Hash<String>
{
    [[nodiscard]] uint64_t operator()(String const &value) const noexcept
    {
        return wyhash(value.c_str(), value.size(), 0, _wyp);
    }
};

} // namespace wheels

#endif // WHEELS_CONTAINERS_STRING_HPP
