#ifndef WHEELS_CONTAINERS_CONCEPTS
#define WHEELS_CONTAINERS_CONCEPTS

#include <concepts>
#include <type_traits>

namespace wheels
{

template <typename T, typename U>
concept SameAs = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

} // namespace wheels

#endif // WHEELS_CONTAINERS_CONCEPTS
