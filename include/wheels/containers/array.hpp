
#ifndef WHEELS_ARRAY_HPP
#define WHEELS_ARRAY_HPP

#include "../allocators/allocator.hpp"
#include "container_utils.hpp"

#include <cstring>

namespace wheels
{

template <typename T> class Array
{
  public:
    Array(Allocator &allocator, size_t initial_capacity = 4);
    ~Array();

    Array(Array<T> const &) = delete;
    Array(Array<T> &&other);
    Array<T> &operator=(Array<T> const &) = delete;
    Array<T> &operator=(Array<T> &&other);

    T &operator[](size_t i);
    T const &operator[](size_t i) const;
    T &front();
    T const &front() const;
    T &back();
    T const &back() const;
    T *data();
    T const *data() const;

    T *begin();
    T const *begin() const;
    T *end();
    T const *end() const;

    bool empty() const;
    size_t size() const;
    void reserve(size_t capacity);
    size_t capacity() const;

    void clear();
    void push_back(T const &value);
    void push_back(T &&value);
    template <typename... Args> void emplace_back(Args &&...args);
    T pop_back();
    void resize(size_t size);
    void resize(size_t size, T const &value);

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

    if (initial_capacity == 0)
        initial_capacity = 4;
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

template <typename T> T &Array<T>::operator[](size_t i)
{
    assert(i < m_size);
    return m_data[i];
}

template <typename T> T const &Array<T>::operator[](size_t i) const
{
    assert(i < m_size);
    return m_data[i];
}

template <typename T> T &Array<T>::front()
{
    assert(m_size > 0);
    return *m_data;
}

template <typename T> T const &Array<T>::front() const
{
    assert(m_size > 0);
    return *m_data;
}

template <typename T> T &Array<T>::back()
{
    assert(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T const &Array<T>::back() const
{
    assert(m_size > 0);
    return m_data[m_size - 1];
}

template <typename T> T *Array<T>::data() { return m_data; }

template <typename T> T const *Array<T>::data() const { return m_data; }

template <typename T> T *Array<T>::begin() { return m_data; }

template <typename T> T const *Array<T>::begin() const { return m_data; }

template <typename T> T *Array<T>::end() { return m_data + m_size; }

template <typename T> T const *Array<T>::end() const { return m_data + m_size; }

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
    for (auto &v : *this)
        v.~T();
    m_size = 0;
}

template <typename T> void Array<T>::push_back(T const &value)
{
    if (m_size == m_capacity)
        reallocate(m_capacity * 2);

    new (m_data + m_size++) T{value};
}

template <typename T> void Array<T>::push_back(T &&value)
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

template <typename T> T Array<T>::pop_back()
{
    assert(m_size > 0);
    m_size--;
    return WHEELS_MOV(m_data[m_size]);
}

template <typename T> void Array<T>::resize(size_t size)
{
    if (size < m_size)
    {
        for (size_t i = size; i < m_size; ++i)
            m_data[i].~T();
        m_size = size;
    }
    else
    {
        reserve(size);
        for (size_t i = m_size; i < size; ++i)
            new (m_data + i) T{};
        m_size = size;
    }
}

template <typename T> void Array<T>::resize(size_t size, T const &value)
{
    if (size < m_size)
    {
        for (size_t i = size; i < m_size; ++i)
            m_data[i].~T();
        m_size = size;
    }
    else
    {
        reserve(size);
        for (size_t i = m_size; i < size; ++i)
            new (m_data + i) T{value};
        m_size = size;
    }
}

template <typename T> void Array<T>::reallocate(size_t capacity)
{
    T *data = (T *)m_allocator.allocate(capacity * sizeof(T));
    assert(data != nullptr);

    if (m_data != nullptr)
    {
        for (size_t i = 0; i < m_size; ++i)
            new (data + i) T{WHEELS_MOV(m_data[i])};
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

} // namespace wheels

#endif // WHEELS_ARRAY_HPP