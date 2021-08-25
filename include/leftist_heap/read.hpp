#ifndef READ_HPP_INCLUDE_GUARD
#define READ_HPP_INCLUDE_GUARD

#include <type_traits>

template<class T>
constexpr bool easy_to_copy =
    std::is_trivially_copyable_v<T> && sizeof(T) <= 4 * sizeof(void*);

template<class T>
using Read = std::conditional_t<easy_to_copy<T>, T const, T const&>;
template<class T>
using ReadReturn = std::conditional_t<easy_to_copy<T>, T, T const&>;



#endif // READ_HPP_INCLUDE_GUARD
