
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
    Array(Allocator &allocator, size_t initial_capacity = 0) noexcept;
    ~Array();

    Array(Array<T> const &) = delete;
    Array(Array<T> &&other) noexcept;
    Array<T> &operator=(Array<T> const &) = delete;
    Array<T> &operator=(Array<T> &&other) noexcept;

    [[nodiscard]] T &operator[](size_t i) noexcept;
    [[nodiscard]] T const &operator[](size_t i) const noexcept;
    [[nodiscard]] T &front() noexcept;
    [[nodiscard]] T const &front() const noexcept;
    [[nodiscard]] T &back() noexcept;
    [[nodiscard]] T const &back() const noexcept;
    [[nodiscard]] T *data() noexcept;
    [[nodiscard]] T const *data() const noexcept;

    [[nodiscard]] T *begin() noexcept;
    [[nodiscard]] T const *begin() const noexcept;
    [[nodiscard]] T *end() noexcept;
    [[nodiscard]] T const *end() const noexcept;

    // Template type inference can't seem to follow T with the inner const so
    // let's have explicit methods for const and mutable spans

    [[nodiscard]] Span<T> mut_span() noexcept;
    [[nodiscard]] Span<T const> span() const noexcept;
    [[nodiscard]] Span<T> mut_span(size_t begin_i, size_t end_i) noexcept;
    [[nodiscard]] Span<T const> span(
        size_t begin_i, size_t end_i) const noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    void reserve(size_t capacity) noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    void clear() noexcept;

    template <typename U>
    // Let's be pedantic and disallow implicit conversions
        requires SameAs<U, T>
    void push_back(U &&value) noexcept;

    template <typename... Args> void emplace_back(Args &&...args) noexcept;

    void extend(Span<const T> values) noexcept;

    T pop_back() noexcept;
    // Preserves the order, takes O(n) for n elements after index
    void erase(size_t index) noexcept;
    // Doesn't preserve the order, runs in O(1)
    void erase_swap_last(size_t index) noexcept;

    void resize(size_t size) noexcept;
    void resize(size_t size, T const &value) noexcept;

    operator Span<T const>() const noexcept;

  private:
    void reallocate(size_t capacity) noexcept;
    void destroy() noexcept;

    Allocator &m_allocator;
    T *m_data{nullptr};
    size_t m_capacity{0};
    size_t m_size{0};
};

template <typename T>
Array<T>::Array(Allocator &allocator, size_t initial_capacity) noexcept
: m_allocator{allocator}
{
    static_assert(
        alignof(T) <= alignof(std::max_align_t) &&
        "Aligned allocations beyond std::max_align_t aren't supported");

    if (initial_capacity > 0)
        reallocate(initial_capacity);
}

template <typename T> Array<T>::~Array() { destroy(); }

template <typename T>
Array<T>::Array(Array<T> &&other) noexcept
: m_allocator{other.m_allocator}
, m_data{other.m_data}
, m_capacity{other.m_capacity}
, m_size{other.m_size}
{
    other.m_data = nullptr;
}

template <typename T> Array<T> &Array<T>::operator=(Array<T> &&other) noexcept
{
    WHEELS_ASSERT(
        &m_allocator == &other.m_allocator &&
        "Move assigning a container with different allocators can lead to "
        "nasty bugs. Use the same allocator or copy the content instead.");

    if (this != &other)
    {
        destroy();

        m_data = other.m_data;
        m_capacity = other.m_capacity;
        m_size = other.m_size;

        other.m_data = nullptr;
    }
    return *this;
}

template <typename T> T &Array<T>::operator[](size_t i) noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> T const &Array<T>::operator[](size_t i) const noexcept
{
    WHEELS_ASSERT(i < m_size);
    return m_data[i];
}

template <typename T> T &Array<T>::front() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return *m_data;
}

template <typename T> T const &Array<T>::front() const noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return *m_data;
}

template <typename T> T &Array<T>::back() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T const &Array<T>::back() const noexcept
{
    WHEELS_ASSERT(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T *Array<T>::data() noexcept { return m_data; }

template <typename T> T const *Array<T>::data() const noexcept
{
    return m_data;
}

template <typename T> T *Array<T>::begin() noexcept { return m_data; }

template <typename T> T const *Array<T>::begin() const noexcept
{
    return m_data;
}

template <typename T> T *Array<T>::end() noexcept { return m_data + m_size; }

template <typename T> T const *Array<T>::end() const noexcept
{
    return m_data + m_size;
}

template <typename T> Span<T> Array<T>::mut_span() noexcept
{
    return Span{begin(), m_size};
}

template <typename T> Span<T const> Array<T>::span() const noexcept
{
    return Span<T const>{begin(), m_size};
}

template <typename T>
Span<T> Array<T>::mut_span(size_t begin_i, size_t end_i) noexcept
{
    WHEELS_ASSERT(begin_i < m_size);
    WHEELS_ASSERT(end_i <= m_size);
    return Span{begin() + begin_i, end_i - begin_i};
}

template <typename T>
Span<T const> Array<T>::span(size_t begin_i, size_t end_i) const noexcept
{
    WHEELS_ASSERT(begin_i < m_size);
    WHEELS_ASSERT(end_i <= m_size);
    return Span{begin() + begin_i, end_i - begin_i};
}

template <typename T> bool Array<T>::empty() const noexcept
{
    return m_size == 0;
}

template <typename T> size_t Array<T>::size() const noexcept { return m_size; }

template <typename T> void Array<T>::reserve(size_t capacity) noexcept
{
    if (capacity > m_capacity)
        reallocate(capacity);
}

template <typename T> size_t Array<T>::capacity() const noexcept
{
    return m_capacity;
}

template <typename T> void Array<T>::clear() noexcept
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
void Array<T>::push_back(U &&value) noexcept
{
    if (m_size == m_capacity)
        reallocate(m_capacity * 2);

    new (m_data + m_size++) T{WHEELS_FWD(value)};
}

template <typename T>
template <typename... Args>
void Array<T>::emplace_back(Args &&...args) noexcept
{
    if (m_size == m_capacity)
        reallocate(m_capacity * 2);

    new (m_data + m_size++) T{WHEELS_FWD(args)...};
}

template <typename T> void Array<T>::extend(Span<const T> values) noexcept
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
            new (m_data + m_size + i) T{values[i]};
    }
    m_size += values.size();
}

template <typename T> T Array<T>::pop_back() noexcept
{
    WHEELS_ASSERT(m_size > 0);
    m_size--;

    T ret = WHEELS_MOV(m_data[m_size]);
    if constexpr (!std::is_trivially_destructible_v<T>)
        // Moved from value might still require dtor
        m_data[m_size].~T();

    return ret;
}

template <typename T> void Array<T>::erase(size_t index) noexcept
{
    WHEELS_ASSERT(index < m_size);

    m_data[index].~T();

    if constexpr (std::is_trivially_copyable_v<T>)
    {
        static_assert(
            std::is_trivially_destructible_v<T>,
            "No dtors are called in this path");
        memcpy(
            m_data + index, m_data + index + 1,
            (m_size - index - 1) * sizeof(T));
    }
    else
    {
        for (size_t i = index + 1; i < m_size; ++i)
        {
            new (m_data + i - 1) T{WHEELS_MOV(m_data[i])};
            if constexpr (!std::is_trivially_destructible_v<T>)
                // Moved from value might still require dtor
                m_data[i].~T();
        }
    }
    m_size--;
}

template <typename T> void Array<T>::erase_swap_last(size_t index) noexcept
{
    WHEELS_ASSERT(index < m_size);

    m_data[index].~T();

    if (m_size > 1 && index < m_size - 1)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            static_assert(
                std::is_trivially_destructible_v<T>,
                "No dtors are called in this path");
            memcpy(m_data + index, m_data + m_size - 1, sizeof(T));
        }
        else
        {
            new (m_data + index) T{WHEELS_MOV(m_data[m_size - 1])};
            if constexpr (!std::is_trivially_destructible_v<T>)
                // Moved from value might still require dtor
                m_data[m_size - 1].~T();
        }
    }
    m_size--;
}

template <typename T> void Array<T>::resize(size_t size) noexcept
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

template <typename T>
void Array<T>::resize(size_t size, T const &value) noexcept
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

template <typename T> void Array<T>::reallocate(size_t capacity) noexcept
{
    if (capacity == 0)
        capacity = 4;

    T *data = (T *)m_allocator.allocate(capacity * sizeof(T));
    WHEELS_ASSERT(data != nullptr);

    if (m_data != nullptr)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            static_assert(
                std::is_trivially_destructible_v<T>,
                "No dtors are called in this path");
            memcpy(data, m_data, m_size * sizeof(T));
        }
        else
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                new (data + i) T{WHEELS_MOV(m_data[i])};
                if constexpr (!std::is_trivially_destructible_v<T>)
                    // Moved from value might still require dtor
                    m_data[i].~T();
            }
        }
        m_allocator.deallocate(m_data);
    }

    m_data = data;
    m_capacity = capacity;
}

template <typename T> void Array<T>::destroy() noexcept
{
    if (m_data != nullptr)
    {
        clear();
        m_allocator.deallocate(m_data);
        m_data = nullptr;
    }
}

template <typename T> Array<T>::operator Span<T const>() const noexcept
{
    return Span<T const>{m_data, m_size};
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_ARRAY_HPP
