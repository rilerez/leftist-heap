#ifndef ACCESSORS_HPP_INCLUDE_GUARD
#define ACCESSORS_HPP_INCLUDE_GUARD

#include "macros.hpp"

#include <type_traits>
#include <utility>

#define TAKE1_(x, ...)    x
#define TAKE2_(x, y, ...) x, y

#define DEFAULT_FIELD_(x) HEDLEY_CONCAT(x, _)
#define CALL_(f, ...)     f(__VA_ARGS__)

#define READER_(name, field)                                              \
  ReadReturn<std::decay_t<decltype(field)>> name() const noexcept {       \
    return field;                                                         \
  }

#define WRITER_(name, field)                                                 \
  auto name(std::decay_t<decltype(field)> value) noexcept->decltype(*this) { \
    field = std::move(value);                                                \
    return *this;                                                            \
  }

#define READER(...)                                                       \
  CALL_(READER_, TAKE2_(__VA_ARGS__, DEFAULT_FIELD_(TAKE1_(__VA_ARGS__))))
#define WRITER(...)                                                       \
  CALL_(WRITER_, TAKE2_(__VA_ARGS__, DEFAULT_FIELD_(TAKE1_(__VA_ARGS__))))
#define ACCESSOR(...) READER(__VA_ARGS__) WRITER(__VA_ARGS__)

#endif // ACCESSORS_HPP_INCLUDE_GUARD
