
#ifndef WHEELS_CONTAINERS_SPAN_HPP
#define WHEELS_CONTAINERS_SPAN_HPP

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

} // namespace wheels

#endif // WHEELS_CONTAINERS_SPAN_HPP
