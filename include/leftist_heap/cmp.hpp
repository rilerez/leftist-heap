#ifndef UTIL_HPP_INCLUDE_GUARD
#define UTIL_HPP_INCLUDE_GUARD

#include "macros.hpp"

#include <functional>
template<class Less = std::less<>>
constexpr auto cmp_by(auto f, Less less = {}) {
  return
      [ f = std::move(f), less = std::move(less) ] FN(less(f(_0), f(_1)));
}

#endif // UTIL_HPP_INCLUDE_GUARD
