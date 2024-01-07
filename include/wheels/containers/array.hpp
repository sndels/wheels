
#ifndef WHEELS_CONTAINERS_ARRAY_HPP
#define WHEELS_CONTAINERS_ARRAY_HPP

#include "../allocators/allocator.hpp"
#include "../assert.hpp"
#include "../utils.hpp"
#include "concepts.hpp"
#include "span.hpp"

#include <cstring>

namespace wheels
{

template <typename T> class Array
{
    // Use a static assert instead of a concepts constraint as this will produce
    // a more understandable error message
    static_assert(
        (std::is_trivially_copyable_v<T> || std::move_constructible<T>),
        "Reallocation requires T to be either trivially copyable or move "
        "constructible");

  public:
    Array(Allocator &allocator, size_t initial_capacity = 0);
    ~Array();

    Array(Array<T> const &) = delete;
    Array(Array<T> &&other);
    Array<T> &operator=(Array<T> const &) = delete;
    Array<T> &operator=(Array<T> &&other);

    [[nodiscard]] T &operator[](size_t i);
    [[nodiscard]] T const &operator[](size_t i) const;
    [[nodiscard]] T &front();
    [[nodiscard]] T const &front() const;
    [[nodiscard]] T &back();
    [[nodiscard]] T const &back() const;
    [[nodiscard]] T *data();
    [[nodiscard]] T const *data() const;

    [[nodiscard]] T *begin();
    [[nodiscard]] T const *begin() const;
    [[nodiscard]] T *end();
    [[nodiscard]] T const *end() const;

    [[nodiscard]] Span<T> span(size_t begin_i, size_t end_i);
    [[nodiscard]] Span<T const> span(size_t begin_i, size_t end_i) const;

    [[nodiscard]] bool empty() const;
    [[nodiscard]] size_t size() const;
    void reserve(size_t capacity);
    [[nodiscard]] size_t capacity() const;

    void clear();

    template <typename U>
    // Let's be pedantic and disallow implicit conversions
        requires SameAs<U, T>
    void push_back(U &&value);

    template <typename... Args> void emplace_back(Args &&...args);

    void extend(Span<const T> values);

    T pop_back();
    // Preserves the order, takes O(n) for n elements after index
    void erase(size_t index);
    // Doesn't preserve the order, runs in O(1)
    void erase_swap_last(size_t index);

    void resize(size_t size);
    void resize(size_t size, T const &value);

    operator Span<T>();
    operator Span<T const>() const;

  private:
    void reallocate(size_t capacity);
    void free();

    Allocator &m_allocator;
    T *m_data{nullptr};
    size_t m_capacity{0};
    size_t m_size{0};
};

template <typename T>
Array<T>::Array(Allocator &allocator, size_t initial_capacity)
: m_allocator{allocator}
{
    static_assert(
        alignof(T) <= alignof(std::max_align_t) &&
        "Aligned allocations beyond std::max_align_t aren't supported");

    if (initial_capacity > 0)
        reallocate(initial_capacity);
}

template <typename T> Array<T>::~Array() { free(); }

template <typename T>
Array<T>::Array(Array<T> &&other)
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_capacity{other.m_capacity}
, m_size{other.m_size}
{
    other.m_data = nullptr;
}

template <typename T> Array<T> &Array<T>::operator=(Array<T> &&other)
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

template <typename T> T &Array<T>::operator[](size_t i)
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> T const &Array<T>::operator[](size_t i) const
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> T &Array<T>::front()
{
    WHEELS_ASSERT(m_size > 0);
    return *m_data;
}

template <typename T> T const &Array<T>::front() const
{
    WHEELS_ASSERT(m_size > 0);
    return *m_data;
}

template <typename T> T &Array<T>::back()
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T const &Array<T>::back() const
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T *Array<T>::data() { return m_data; }

template <typename T> T const *Array<T>::data() const { return m_data; }

template <typename T> T *Array<T>::begin() { return m_data; }

template <typename T> T const *Array<T>::begin() const { return m_data; }

template <typename T> T *Array<T>::end() { return m_data + m_size; }

template <typename T> T const *Array<T>::end() const { return m_data + m_size; }

template <typename T> Span<T> Array<T>::span(size_t begin_i, size_t end_i)
{
    WHEELS_ASSERT(begin_i < m_size);
    WHEELS_ASSERT(end_i <= m_size);
    return Span{begin() + begin_i, end_i - begin_i};
}

template <typename T>
Span<T const> Array<T>::span(size_t begin_i, size_t end_i) const
{
    WHEELS_ASSERT(begin_i < m_size);
    WHEELS_ASSERT(end_i <= m_size);
    return Span{begin() + begin_i, end_i - begin_i};
}

template <typename T> bool Array<T>::empty() const { return m_size == 0; }

template <typename T> size_t Array<T>::size() const { return m_size; }

template <typename T> void Array<T>::reserve(size_t capacity)
{
    if (capacity > m_capacity)
        reallocate(capacity);
}

template <typename T> size_t Array<T>::capacity() const { return m_capacity; }

template <typename T> void Array<T>::clear()
{
    if constexpr (!std::is_trivially_destructible_v<T>)
    {
        for (auto &v : *this)
            v.~T();
    }
    m_size = 0;
}

template <typename T>
template <typename U>
    requires SameAs<U, T>
void Array<T>::push_back(U &&value)
{
    if (m_size == m_capacity)
        reallocate(m_capacity * 2);

    new (m_data + m_size++) T{WHEELS_FWD(value)};
}

template <typename T>
template <typename... Args>
void Array<T>::emplace_back(Args &&...args)
{
    if (m_size == m_capacity)
        reallocate(m_capacity * 2);

    new (m_data + m_size++) T{WHEELS_FWD(args)...};
}

template <typename T> void Array<T>::extend(Span<const T> values)
{
    const size_t required_size = m_size + values.size();
    if (required_size > m_capacity)
    {
        if (required_size <= m_capacity * 2)
            reallocate(m_capacity * 2);
        else
            reallocate(required_size);
    }

    if constexpr (std::is_trivially_copyable_v<T>)
        memcpy(m_data + m_size, values.data(), values.size() * sizeof(T));
    else
    {
        for (size_t i = 0; i < values.size(); ++i)
            new (m_data + m_size + i) T{WHEELS_MOV(values[i])};
    }
    m_size += values.size();
}

template <typename T> T Array<T>::pop_back()
{
    WHEELS_ASSERT(m_size > 0);
    m_size--;
    return WHEELS_MOV(m_data[m_size]);
}

template <typename T> void Array<T>::erase(size_t index)
{
    WHEELS_ASSERT(index < m_size);

    m_data[index].~T();

    if constexpr (std::is_trivially_copyable_v<T>)
        memcpy(
            m_data + index, m_data + index + 1, (m_size - index) * sizeof(T));
    else
    {
        for (size_t i = index + 1; i < m_size; ++i)
            new (m_data + i - 1) T{WHEELS_MOV(m_data[i])};
    }
    m_size--;
}

template <typename T> void Array<T>::erase_swap_last(size_t index)
{
    WHEELS_ASSERT(index < m_size);

    m_data[index].~T();

    if (m_size > 1 && index < m_size - 1)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
            memcpy(m_data + index, m_data + m_size - 1, sizeof(T));
        else
        {
            new (m_data + index) T{WHEELS_MOV(m_data[m_size - 1])};
        }
    }
    m_size--;
}

template <typename T> void Array<T>::resize(size_t size)
{
    if (size < m_size)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = size; i < m_size; ++i)
                m_data[i].~T();
        }
        m_size = size;
    }
    else
    {
        reserve(size);
        if constexpr (std::is_class_v<T>)
        {
            for (size_t i = m_size; i < size; ++i)
                new (m_data + i) T{};
        }
        m_size = size;
    }
}

template <typename T> void Array<T>::resize(size_t size, T const &value)
{
    if (size < m_size)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = size; i < m_size; ++i)
                m_data[i].~T();
        }
        m_size = size;
    }
    else
    {
        reserve(size);
        // TODO:
        // Is it faster to init a bunch of non-class values by assigning
        // directly instead of placement new?
        for (size_t i = m_size; i < size; ++i)
            new (m_data + i) T{value};
        m_size = size;
    }
}

template <typename T> void Array<T>::reallocate(size_t capacity)
{
    if (capacity == 0)
        capacity = 4;

    T *data = (T *)m_allocator.allocate(capacity * sizeof(T));
    WHEELS_ASSERT(data != nullptr);

    if (m_data != nullptr)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
            memcpy(data, m_data, m_size * sizeof(T));
        else
        {
            for (size_t i = 0; i < m_size; ++i)
                new (data + i) T{WHEELS_MOV(m_data[i])};
        }
        m_allocator.deallocate(m_data);
    }

    m_data = data;
    m_capacity = capacity;
}

template <typename T> void Array<T>::free()
{
    if (m_data != nullptr)
    {
        clear();
        m_allocator.deallocate(m_data);
        m_data = nullptr;
    }
}

template <typename T> Array<T>::operator Span<T>()
{
    return Span{m_data, m_size};
}

template <typename T> Array<T>::operator Span<T const>() const
{
    return Span<T const>{m_data, m_size};
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_ARRAY_HPP
