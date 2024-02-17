#ifndef WHEELS_CONTAINERS_OPTIONAL_HPP
#define WHEELS_CONTAINERS_OPTIONAL_HPP

#include "../assert.hpp"
#include "../utils.hpp"
#include <new>

namespace wheels
{

template <typename T> class Optional
{
    // TODO: Constrain stored type ctor, dtor to noexcept?
  public:
    Optional() noexcept;
    Optional(T const &value) noexcept;
    Optional(T &&value) noexcept;
    ~Optional();

    Optional(Optional<T> const &other) noexcept;
    Optional(Optional<T> &&other) noexcept;
    Optional<T> &operator=(Optional<T> const &other) noexcept;
    Optional<T> &operator=(Optional<T> &&other) noexcept;

    [[nodiscard]] bool has_value() const noexcept;
    void reset() noexcept;
    T &&take() noexcept;
    [[nodiscard]] Optional<T> swap(T &&value) noexcept;
    template <typename... Args> void emplace(Args &&...args) noexcept;

    [[nodiscard]] T &operator*() noexcept;
    [[nodiscard]] T const &operator*() const noexcept;
    [[nodiscard]] T *operator->() noexcept;
    [[nodiscard]] T const *operator->() const noexcept;

  private:
#ifndef NDEBUG
    const T *m_debug{nullptr};
#endif // NDEBUG
    alignas(T) uint8_t m_data[sizeof(T)];
    bool m_has_value{false};
};

template <typename T>
Optional<T>::Optional() noexcept
#ifndef NDEBUG
: m_debug{reinterpret_cast<const T *>(&m_data)}
#endif // NDEBUG
{
}

template <typename T>
Optional<T>::Optional(T const &value) noexcept
#ifndef NDEBUG
: m_debug{reinterpret_cast<const T *>(&m_data)}
#endif // NDEBUG
{
    m_has_value = true;
    new (m_data) T{value};
}

template <typename T>
Optional<T>::Optional(T &&value) noexcept
#ifndef NDEBUG
: m_debug{reinterpret_cast<const T *>(&m_data)}
#endif // NDEBUG
{
    m_has_value = true;
    new (m_data) T{WHEELS_MOV(value)};
}

template <typename T> Optional<T>::~Optional() { reset(); }

template <typename T>
Optional<T>::Optional(Optional<T> const &other) noexcept
: m_has_value{other.m_has_value}
{
    if (m_has_value)
        new (m_data) T{*(T *)other.m_data};
}

template <typename T>
Optional<T>::Optional(Optional<T> &&other) noexcept
: m_has_value{other.m_has_value}
{
    if (m_has_value)
        new (m_data) T{WHEELS_MOV(*(T *)other.m_data)};
    other.m_has_value = false;
}

template <typename T>
Optional<T> &Optional<T>::operator=(Optional<T> const &other) noexcept
{
    if (this != &other)
    {
        reset();

        if (other.m_has_value)
            new (m_data) T{*(T *)other.m_data};
        m_has_value = other.m_has_value;
    }
    return *this;
}

template <typename T>
Optional<T> &Optional<T>::operator=(Optional<T> &&other) noexcept
{
    if (this != &other)
    {
        reset();

        if (other.m_has_value)
            new (m_data) T{WHEELS_MOV(*(T *)other.m_data)};
        m_has_value = other.m_has_value;
        other.m_has_value = false;
    }
    return *this;
}

template <typename T> bool Optional<T>::has_value() const noexcept
{
    // TODO:
    // Does prefetching make a difference when data resides before this flag?
    // Caller likely uses the data when we return true here
    return m_has_value;
}

template <typename T> void Optional<T>::reset() noexcept
{
    if (m_has_value)
    {
        (*(T *)m_data).~T();
        m_has_value = false;
    }
}

template <typename T> T &&Optional<T>::take() noexcept
{
    WHEELS_ASSERT(has_value());
    m_has_value = false;

    return WHEELS_MOV(*(T *)m_data);
}

template <typename T> Optional<T> Optional<T>::swap(T &&value) noexcept
{
    Optional<T> ret;
    if (has_value())
    {
        new (ret.m_data) T{WHEELS_MOV(*(T *)m_data)};
        ret.m_has_value = true;
    }

    new (m_data) T{WHEELS_FWD(value)};
    m_has_value = true;

    return ret;
}

template <typename T>
template <typename... Args>
void Optional<T>::emplace(Args &&...args) noexcept
{
    reset();

    new (m_data) T{WHEELS_FWD(args)...};
    m_has_value = true;
}

template <typename T> T &Optional<T>::operator*() noexcept
{
    WHEELS_ASSERT(has_value());
    return *(T *)m_data;
}

template <typename T> T const &Optional<T>::operator*() const noexcept
{
    WHEELS_ASSERT(has_value());
    return *(T *)m_data;
}

template <typename T> T *Optional<T>::operator->() noexcept
{
    WHEELS_ASSERT(has_value());
    return (T *)m_data;
}

template <typename T> T const *Optional<T>::operator->() const noexcept
{
    WHEELS_ASSERT(has_value());
    return (T const *)m_data;
}

template <typename T>
bool operator==(Optional<T> const &lhs, Optional<T> const &rhs) noexcept
{
    // Both empty
    if (!lhs.has_value() && !rhs.has_value())
        return true;

    // One empty
    if (lhs.has_value() != rhs.has_value())
        return false;

    return *lhs == *rhs;
}

template <typename T>
bool operator!=(Optional<T> const &lhs, Optional<T> const &rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace wheels

#endif // WHEELS_CONTAINERS_OPTIONAL_HPP
