#ifndef WHEELS_OWNING_PTR_HPP
#define WHEELS_OWNING_PTR_HPP

#include "allocators/allocator.hpp"
#include "utils.hpp"

#include <cstddef>

namespace wheels
{

// Basically unique_ptr with an explicit allocator.
template <typename T> class OwningPtr
{
  public:
    OwningPtr() noexcept = default;
    // alloc needs to live as long as this ptr
    template <typename... Args>
    OwningPtr(Allocator &alloc, Args &&...args) noexcept;
    ~OwningPtr();

    OwningPtr(OwningPtr<T> const &other) = delete;
    OwningPtr(OwningPtr<T> &&other) noexcept;
    OwningPtr<T> &operator=(OwningPtr<T> const &other) = delete;
    OwningPtr<T> &operator=(OwningPtr<T> &&other) noexcept;

    [[nodiscard]] T *get() noexcept;
    [[nodiscard]] T const *get() const noexcept;

    [[nodiscard]] T &operator*() noexcept;
    [[nodiscard]] T const &operator*() const noexcept;
    [[nodiscard]] T *operator->() noexcept;
    [[nodiscard]] T const *operator->() const noexcept;

    void reset() noexcept;
    void swap(OwningPtr<T> &other) noexcept;

  private:
    Allocator *_alloc{nullptr};
    T *_data{nullptr};
};

template <typename T>
template <typename... Args>
OwningPtr<T>::OwningPtr(Allocator &alloc, Args &&...args) noexcept
: _alloc{&alloc}
{
    _data = reinterpret_cast<T *>(_alloc->allocate(sizeof(T)));
    new (_data) T{WHEELS_FWD(args)...};
}

template <typename T> OwningPtr<T>::~OwningPtr() { reset(); }

template <typename T>
OwningPtr<T>::OwningPtr(OwningPtr<T> &&other) noexcept
: _alloc{other._alloc}
, _data{other._data}
{
    other._alloc = nullptr;
    other._data = nullptr;
}

template <typename T>
OwningPtr<T> &OwningPtr<T>::operator=(OwningPtr<T> &&other) noexcept
{
    if (this != &other)
    {
        // Things will get very confusing if moves can change existing
        // allocators
        WHEELS_ASSERT(_alloc == nullptr || _alloc == other._alloc);

        if (_alloc != nullptr)
            reset();

        _alloc = other._alloc;
        _data = other._data;

        other._alloc = nullptr;
        other._data = nullptr;
    }
    return *this;
}

template <typename T> [[nodiscard]] T *OwningPtr<T>::get() noexcept
{
    return _data;
}

template <typename T> [[nodiscard]] T const *OwningPtr<T>::get() const noexcept
{
    return _data;
}

template <typename T> [[nodiscard]] T &OwningPtr<T>::operator*() noexcept
{
    WHEELS_ASSERT(_data != nullptr);
    return *_data;
}

template <typename T>
[[nodiscard]] T const &OwningPtr<T>::operator*() const noexcept
{
    WHEELS_ASSERT(_data != nullptr);
    return *_data;
}

template <typename T> [[nodiscard]] T *OwningPtr<T>::operator->() noexcept
{
    WHEELS_ASSERT(_data != nullptr);
    return _data;
}

template <typename T>
[[nodiscard]] T const *OwningPtr<T>::operator->() const noexcept
{
    WHEELS_ASSERT(_data != nullptr);
    return _data;
}

template <typename T> void OwningPtr<T>::reset() noexcept
{
    if (_alloc != nullptr)
    {
        if (_data != nullptr)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                _data->~T();
            _alloc->deallocate(_data);
            _data = nullptr;
        }
    }
    else
        WHEELS_ASSERT(_data == nullptr);
}

template <typename T> void OwningPtr<T>::swap(OwningPtr<T> &other) noexcept
{
    WHEELS_ASSERT(_alloc == other._alloc);
    T *tmp = _data;
    _data = other._data;
    other._data = tmp;
}

template <typename T>
bool operator==(OwningPtr<T> const &ptr, std::nullptr_t) noexcept
{
    return ptr.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, OwningPtr<T> const &ptr) noexcept
{
    return ptr.get() == nullptr;
}

template <typename T>
bool operator!=(OwningPtr<T> const &ptr, std::nullptr_t) noexcept
{
    return ptr.get() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, OwningPtr<T> const &ptr) noexcept
{
    return ptr.get() != nullptr;
}

} // namespace wheels

#endif // WHEELS_OWNING_PTR_HPP
