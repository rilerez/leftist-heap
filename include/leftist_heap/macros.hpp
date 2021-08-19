#ifndef LEFTIST_HEAP_MACROS_INCLUDE_GUARD
#define LEFTIST_HEAP_MACROS_INCLUDE_GUARD

#include "hedley.h"

#define FWD(...) (static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__))
#define ARROW(...)                                                     \
  noexcept(noexcept(__VA_ARGS__))->decltype(__VA_ARGS__) {             \
    return __VA_ARGS__;                                                \
  }

#define NOEX(...)                                                      \
  noexcept(noexcept(__VA_ARGS__)) { return __VA_ARGS__; }

namespace leftist_heap { namespace fn_impl {
template<class T>
auto depend_id(auto x) {
  return x;
}

// A dummy type that cannot be used.
// This is for catching arity mismatches with FN expressions
struct fake {
  template<class T>
  operator T() {
    static_assert(depend_id<T>(false), "Arity mismatch for FN expression.");
  }
};
}} // namespace leftist_heap::fn_impl

#define GENSYM(name)                                                   \
  HEDLEY_CONCAT3(leftist_heap_gensym, __COUNTER__, name)

#define LEFTIST_HEAP_FN_(name, ...)                                    \
  <class HEDLEY_CONCAT(name, 0) = ::leftist_heap::fn_impl::fake,       \
   class HEDLEY_CONCAT(name, 1) = ::leftist_heap::fn_impl::fake,       \
   class HEDLEY_CONCAT(name, 2) = ::leftist_heap::fn_impl::fake,       \
   class HEDLEY_CONCAT(name, 3) = ::leftist_heap::fn_impl::fake,       \
   class HEDLEY_CONCAT(name, 4) = ::leftist_heap::fn_impl::fake>(      \
      [[maybe_unused]] HEDLEY_CONCAT(name, 0) _0 = {},                 \
      [[maybe_unused]] HEDLEY_CONCAT(name, 1) _1 = {},                 \
      [[maybe_unused]] HEDLEY_CONCAT(name, 2) _2 = {},                 \
      [[maybe_unused]] HEDLEY_CONCAT(name, 3) _3 = {},                 \
      [[maybe_unused]] HEDLEY_CONCAT(name, 4) _4 = {}) {               \
    [[maybe_unused]] auto& _ = _0;                                     \
    return __VA_ARGS__;                                                \
  }

#define FN(...) LEFTIST_HEAP_FN_(GENSYM(fn_type), __VA_ARGS__)

#if defined(__clang__)
#  define IGNORE_WASSUME                                               \
    _Pragma("clang diagnostic ignored \"-Wassume\"")
#else
#  define IGNORE_WASSUME
#endif

#define WITH_DIAGNOSTIC_TWEAK(diagnostic, ...)                         \
  HEDLEY_DIAGNOSTIC_PUSH diagnostic __VA_ARGS__ HEDLEY_DIAGNOSTIC_POP

#endif // LEFTIST_HEAP_MACROS_INCLUDE_GUARD
