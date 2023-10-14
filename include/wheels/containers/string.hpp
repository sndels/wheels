
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
    String(Allocator &allocator, size_t initial_capacity = 16);
    String(Allocator &allocator, char const *str);
    String(Allocator &allocator, StrSpan str);
    ~String();

    String(String const &) = delete;
    String(String &&other);
    String &operator=(String const &) = delete;
    String &operator=(String &&other);

    [[nodiscard]] char &operator[](size_t i);
    [[nodiscard]] char const &operator[](size_t i) const;
    [[nodiscard]] char &front();
    [[nodiscard]] char const &front() const;
    [[nodiscard]] char &back();
    [[nodiscard]] char const &back() const;
    [[nodiscard]] char *data();
    [[nodiscard]] char const *data() const;
    [[nodiscard]] char *c_str();
    [[nodiscard]] char const *c_str() const;

    [[nodiscard]] char *begin();
    [[nodiscard]] char const *begin() const;
    [[nodiscard]] char *end();
    [[nodiscard]] char const *end() const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    // Reserves capacity characters plus a final null
    void reserve(size_t capacity);
    // Doesn't include the final null
    [[nodiscard]] size_t capacity() const;

    void clear();

    void push_back(char ch);
    char pop_back();

    void resize(size_t size, char ch = '\0');

    // No operator+ as it returns a new String and reusing the internal
    // allocator would make the lifetime of the new thing ambiguous. Use
    // concat() instead. Have extend() instead of operator+= to just not have
    // overloaded math operators.

    friend String concat(Allocator &allocator, StrSpan first, StrSpan second);
    friend String concat(
        Allocator &allocator, StrSpan first, char const *second);
    friend String concat(
        Allocator &allocator, char const *first, StrSpan second);

    String &extend(StrSpan str);
    String &extend(char const *str);

    [[nodiscard]] Optional<size_t> find_first(StrSpan substr) const;
    [[nodiscard]] Optional<size_t> find_first(char const *substr) const;
    [[nodiscard]] Optional<size_t> find_first(char ch) const;

    [[nodiscard]] Optional<size_t> find_last(StrSpan substr) const;
    [[nodiscard]] Optional<size_t> find_last(char const *substr) const;
    [[nodiscard]] Optional<size_t> find_last(char ch) const;

    [[nodiscard]] bool contains(StrSpan substr) const;
    [[nodiscard]] bool contains(char const *substr) const;
    [[nodiscard]] bool contains(char ch) const;

    [[nodiscard]] bool starts_with(StrSpan substr) const;
    [[nodiscard]] bool starts_with(char const *substr) const;
    [[nodiscard]] bool starts_with(char ch) const;

    [[nodiscard]] bool ends_with(StrSpan substr) const;
    [[nodiscard]] bool ends_with(char const *substr) const;
    [[nodiscard]] bool ends_with(char ch) const;

    // Require explicit allocator for split for the same reason as concat()

    [[nodiscard]] Array<StrSpan> split(
        Allocator &allocator, StrSpan substr) const;
    [[nodiscard]] Array<StrSpan> split(
        Allocator &allocator, char const *substr) const;
    [[nodiscard]] Array<StrSpan> split(Allocator &allocator, char ch) const;

    operator StrSpan() const;
    StrSpan span(size_t begin, size_t end) const;

  private:
    void reallocate(size_t capacity);
    void free();

    [[nodiscard]] static StrSpan span(char const *str);

    [[nodiscard]] static Optional<size_t> find_first(
        StrSpan from, StrSpan substr);
    [[nodiscard]] static Optional<size_t> find_first(StrSpan from, char ch);

    Allocator &m_allocator;
    char *m_data{nullptr};
    size_t m_capacity{0};
    size_t m_size{0};
};

inline String::String(Allocator &allocator, size_t initial_capacity)
: m_allocator{allocator}
{
    if (initial_capacity == 0)
        initial_capacity = 16;
    reallocate(initial_capacity + 1);
    m_data[0] = '\0';
}

inline String::String(Allocator &allocator, char const *str)
: m_allocator{allocator}
{
    size_t const len = strlen(str);

    reallocate(len + 1);

    memcpy(m_data, str, len);
    m_data[len] = '\0';

    m_size = len;
}

inline String::String(Allocator &allocator, StrSpan str)
: m_allocator{allocator}
{
    reallocate(str.size() + 1);

    memcpy(m_data, str.data(), str.size());
    m_data[str.size()] = '\0';

    m_size = str.size();
}

inline String::~String() { free(); }

inline String::String(String &&other)
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_capacity{other.m_capacity}
, m_size{other.m_size}
{
    other.m_data = nullptr;
}

inline String &String::operator=(String &&other)
{
    if (this != &other)
    {
        free();

        m_allocator = other.m_allocator;
        m_data = other.m_data;
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        other.m_data = nullptr;
    }
    return *this;
}

inline char &String::operator[](size_t i)
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

inline char const &String::operator[](size_t i) const
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

inline char &String::front()
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[0];
}

inline char const &String::front() const
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[0];
}

inline char &String::back()
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
};

inline char const &String::back() const
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
};

inline char *String::data() { return m_data; }

inline char const *String::data() const { return m_data; }

inline char *String::c_str() { return m_data; }

inline char const *String::c_str() const { return m_data; }

inline char *String::begin() { return m_data; }

inline char const *String::begin() const { return m_data; }

inline char *String::end() { return m_data + m_size; }

inline char const *String::end() const { return m_data + m_size; }

inline bool String::empty() const { return m_size == 0; }

inline size_t String::size() const { return m_size; }

inline void String::reserve(size_t capacity)
{
    if (capacity + 1 > m_capacity)
        reallocate(capacity + 1);
}

inline size_t String::capacity() const
{
    // Don't count the extra null slot
    return m_capacity - 1;
}

inline void String::clear()
{
    m_data[0] = '\0';
    m_size = 0;
}

inline void String::push_back(char ch)
{
    if (m_size == m_capacity - 1)
        reallocate(m_capacity * 2);

    m_data[m_size++] = ch;
    m_data[m_size] = '\0';
}

inline char String::pop_back()
{
    WHEELS_ASSERT(m_size > 0);
    m_size--;
    char const ret = m_data[m_size];
    m_data[m_size] = '\0';
    return ret;
}

inline void String::resize(size_t size, char ch)
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

inline String &String::extend(StrSpan str)
{
    reserve(m_size + str.size());

    memcpy(m_data + m_size, str.data(), str.size());
    m_size = m_size + str.size();
    m_data[m_size] = '\0';

    return *this;
}

inline String &String::extend(char const *str) { return extend(span(str)); }

inline Optional<size_t> String::find_first(StrSpan substr) const
{
    return find_first(StrSpan(*this), substr);
}

inline Optional<size_t> String::find_first(char const *substr) const
{
    return find_first(span(substr));
}

inline Optional<size_t> String::find_first(char ch) const
{
    return find_first(StrSpan{*this}, ch);
}

inline Optional<size_t> String::find_last(StrSpan substr) const
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

inline Optional<size_t> String::find_last(char const *substr) const
{
    return find_last(span(substr));
}

inline Optional<size_t> String::find_last(char ch) const
{
    for (size_t i = m_size; i > 0; --i)
    {
        if (m_data[i - 1] == ch)
            return Optional{i - 1};
    }
    return {};
}

inline bool String::contains(StrSpan substr) const
{
    return find_first(substr).has_value();
}

inline bool String::contains(char const *substr) const
{
    return find_first(substr).has_value();
}

inline bool String::contains(char ch) const
{
    return find_first(ch).has_value();
}

inline bool String::starts_with(StrSpan substr) const
{
    Optional<size_t> const found = find_first(substr);
    if (found.has_value())
        return *found == 0;

    return false;
}

inline bool String::starts_with(char const *substr) const
{
    return starts_with(span(substr));
}

inline bool String::starts_with(char ch) const
{
    Optional<size_t> const found = find_first(ch);
    if (found.has_value())
        return *found == 0;

    return false;
}

inline bool String::ends_with(StrSpan substr) const
{
    Optional<size_t> const found = find_last(substr);
    if (found.has_value())
        return *found == m_size - substr.size();

    return false;
}

inline bool String::ends_with(char const *substr) const
{
    return ends_with(span(substr));
}

inline bool String::ends_with(char ch) const
{
    Optional<size_t> const found = find_last(ch);
    if (found.has_value())
        return *found == m_size - 1;

    return false;
}

inline Array<StrSpan> String::split(Allocator &allocator, StrSpan substr) const
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
    Allocator &allocator, char const *substr) const
{
    return split(allocator, span(substr));
}

inline Array<StrSpan> String::split(Allocator &allocator, char ch) const
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

inline void String::reallocate(size_t capacity)
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

inline void String::free()
{
    if (m_data != nullptr)
    {
        m_allocator.deallocate(m_data);
        m_data = nullptr;
    }
}

inline StrSpan String::span(char const *str) { return StrSpan{str}; }

inline Optional<size_t> String::find_first(StrSpan from, StrSpan substr)
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

inline Optional<size_t> String::find_first(StrSpan from, char ch)
{
    size_t const size = from.size();
    for (size_t i = 0; i < size; ++i)
    {
        if (from.data()[i] == ch)
            return {i};
    }
    return {};
}

inline String::operator StrSpan() const { return StrSpan{m_data, m_size}; }

inline StrSpan String::span(size_t begin, size_t end) const
{
    WHEELS_ASSERT(begin <= end);
    WHEELS_ASSERT(end <= m_size);
    return StrSpan{m_data + begin, end - begin};
}

static inline bool operator==(String const &lhs, String const &rhs)
{
    return StrSpan{lhs.c_str(), lhs.size()} == StrSpan{rhs.c_str(), rhs.size()};
}

static inline bool operator!=(String const &lhs, String const &rhs)
{
    return !(lhs == rhs);
}

static inline bool operator==(String const &lhs, char const *rhs)
{
    return StrSpan{lhs.c_str(), lhs.size()} == StrSpan{rhs};
}

static inline bool operator!=(String const &lhs, char const *rhs)
{
    return !(lhs == rhs);
}

static inline bool operator==(char const *lhs, String const &rhs)
{
    return StrSpan{lhs} == StrSpan{rhs.c_str(), rhs.size()};
}

static inline bool operator!=(char const *lhs, String const &rhs)
{
    return !(lhs == rhs);
}

inline String concat(Allocator &allocator, StrSpan first, StrSpan second)
{
    String ret{allocator, first.size() + second.size()};

    memcpy(ret.m_data, first.data(), first.size());
    memcpy(ret.m_data + first.size(), second.data(), second.size());

    ret.m_size = first.size() + second.size();
    ret.m_data[ret.m_size] = '\0';

    return ret;
}

inline String concat(Allocator &allocator, StrSpan first, char const *second)
{
    return concat(allocator, first, String::span(second));
}

inline String concat(Allocator &allocator, char const *first, StrSpan second)
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
