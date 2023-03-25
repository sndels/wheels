#ifndef WHEELS_CONTAINERS_CONCEPTS
#define WHEELS_CONTAINERS_CONCEPTS

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace wheels
{

template <typename T, typename U>
concept SameAs = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template <class Hasher, typename Key>
concept InvocableHash = std::is_invocable_v<Hasher, Key>;

template <class Hasher, typename Key>
concept CorrectHashRetVal = std::is_same_v<
    typename std::invoke_result_t<Hasher, Key const &>, uint64_t>;

} // namespace wheels

#endif // WHEELS_CONTAINERS_CONCEPTS
