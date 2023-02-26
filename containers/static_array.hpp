
#ifndef WHEELS_STATIC_ARRAY_HPP
#define WHEELS_STATIC_ARRAY_HPP

namespace wheels
{

template <typename T, size_t N> class StaticArray
{
  public:
    StaticArray(){};
    ~StaticArray();

    StaticArray(StaticArray<T, N> const &other);
    StaticArray(StaticArray<T, N> &&other);
    StaticArray<T, N> &operator=(StaticArray<T, N> const &other);
    StaticArray<T, N> &operator=(StaticArray<T, N> &&other);

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
    size_t capacity() const;

    void clear();
    void push_back(const T &value);
    template <typename... Args> void emplace_back(Args const &...args);
    T pop_back();
    void resize(size_t size);
    void resize(size_t size, T const &value);

  private:
    alignas(T) uint8_t m_data[N * sizeof(T)];
    size_t m_size{0};
};

template <typename T, size_t N> StaticArray<T, N>::~StaticArray() { clear(); }

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(StaticArray<T, N> const &other)
: m_size{other.m_size}
{
    memcpy(m_data, other.m_data, other.m_size * sizeof(T));
}

template <typename T, size_t N>
StaticArray<T, N>::StaticArray(StaticArray<T, N> &&other)
: m_size{other.m_size}
{
    memcpy(m_data, other.m_data, other.m_size * sizeof(T));
    other.m_size = 0;
}

template <typename T, size_t N>
StaticArray<T, N> &StaticArray<T, N>::operator=(StaticArray<T, N> const &other)
{
    if (this != &other)
    {
        clear();

        memcpy(m_data, other.m_data, other.m_size * sizeof(T));
        m_size = other.m_size;
    }
    return *this;
}

template <typename T, size_t N>
StaticArray<T, N> &StaticArray<T, N>::operator=(StaticArray<T, N> &&other)
{
    if (this != &other)
    {
        clear();

        memcpy(m_data, other.m_data, other.m_size * sizeof(T));
        m_size = other.m_size;

        other.m_size = 0;
    }
    return *this;
}

template <typename T, size_t N> T &StaticArray<T, N>::operator[](size_t i)
{
    assert(i < m_size);
    return ((T *)m_data)[i];
}

template <typename T, size_t N>
T const &StaticArray<T, N>::operator[](size_t i) const
{
    assert(i < m_size);
    return ((T *)m_data)[i];
}

template <typename T, size_t N> T &StaticArray<T, N>::front()
{
    assert(m_size > 0);
    return *((T *)m_data);
}

template <typename T, size_t N> T const &StaticArray<T, N>::front() const
{
    assert(m_size > 0);
    return *((T *)m_data);
}

template <typename T, size_t N> T &StaticArray<T, N>::back()
{
    assert(m_size > 0);
    return ((T *)m_data)[m_size - 1];
}

template <typename T, size_t N> T const &StaticArray<T, N>::back() const
{
    assert(m_size > 0);
    return ((T *)m_data)[m_size - 1];
}

template <typename T, size_t N> T *StaticArray<T, N>::data()
{
    assert(m_size > 0);
    return ((T *)m_data);
}

template <typename T, size_t N> T const *StaticArray<T, N>::data() const
{
    assert(m_size > 0);
    return ((T *)m_data);
}

template <typename T, size_t N> T *StaticArray<T, N>::begin()
{
    return ((T *)m_data);
}

template <typename T, size_t N> T const *StaticArray<T, N>::begin() const
{
    return ((T *)m_data);
}

template <typename T, size_t N> T *StaticArray<T, N>::end()
{
    return ((T *)m_data) + m_size;
}

template <typename T, size_t N> T const *StaticArray<T, N>::end() const
{
    return ((T *)m_data) + m_size;
}

template <typename T, size_t N> bool StaticArray<T, N>::empty() const
{
    return m_size == 0;
}

template <typename T, size_t N> size_t StaticArray<T, N>::size() const
{
    return m_size;
}

template <typename T, size_t N> size_t StaticArray<T, N>::capacity() const
{
    return N;
}

template <typename T, size_t N> void StaticArray<T, N>::clear()
{
    for (auto &v : *this)
        v.~T();
    m_size = 0;
}

template <typename T, size_t N>
void StaticArray<T, N>::push_back(const T &value)
{
    assert(m_size < N);
    ((T *)m_data)[m_size++] = value;
}

template <typename T, size_t N>
template <typename... Args>
void StaticArray<T, N>::emplace_back(Args const &...args)
{
    assert(m_size < N);
    new (((T *)m_data) + m_size++) T{args...};
}

template <typename T, size_t N> T StaticArray<T, N>::pop_back()
{
    assert(m_size > 0);
    m_size--;
    return std::move(((T *)m_data)[m_size]);
}

template <typename T, size_t N> void StaticArray<T, N>::resize(size_t size)
{
    if (size < m_size)
    {
        for (size_t i = size; i < m_size; ++i)
            ((T *)m_data)[i].~T();
        m_size = size;
    }
    else
    {
        assert(size <= N);
        for (size_t i = m_size; i < size; ++i)
            new (((T *)m_data) + i) T{};
        m_size = size;
    }
}

template <typename T, size_t N>
void StaticArray<T, N>::resize(size_t size, T const &value)
{
    if (size < m_size)
    {
        for (size_t i = size; i < m_size; ++i)
            ((T *)m_data)[i].~T();
        m_size = size;
    }
    else
    {
        assert(size <= N);
        for (size_t i = m_size; i < size; ++i)
            ((T *)m_data)[i] = value;
        m_size = size;
    }
}

} // namespace wheels

#endif // WHEELS_STATIC_ARRAY_HPP