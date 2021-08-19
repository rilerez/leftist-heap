#ifndef ASSERTS_INCLUDE_GUARD
#define ASSERTS_INCLUDE_GUARD

#include "hedley.h"

#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/comparison/less_equal.hpp>

#define ASSERT_LEVEL_NONE     0
#define ASSERT_LEVEL_RELEASE  1
#define ASSERT_LEVEL_DEBUG    2
#define ASSERT_LEVEL_PARANOID 3

#ifndef ASSERT_LEVEL
#  ifdef NDEBUG
#    define ASSERT_LEVEL ASSERT_LEVEL_RELEASE
#  else
#    define ASSERT_LEVEL ASSERT_LEVEL_DEBUG
#  endif
#endif

#ifndef ASSERT_HANDLER
#  include <assert.h>
#  define ASSERT_HANDLER(...) assert(false)
#endif

#ifndef ASSERTER
#  define ASSERTER(...)                                                \
    [&] {                                                               \
      if(!(__VA_ARGS__)) ASSERT_HANDLER(__VA_ARGS__);                  \
    }()
#endif

#ifndef ASSERT_ENABLE_ASSUME
#  define ASSERT_ENABLE_ASSUME true
#endif

#define ASSERT_ENABLE_INTIFY_0     0
#define ASSERT_ENABLE_INTIFY_1     1
#define ASSERT_ENABLE_INTIFY_false 0
#define ASSERT_ENABLE_INTIFY_true  1

#define ASSERT_ENABLE_ASSUME_INTED                                     \
  HEDLEY_CONCAT(ASSERT_ENABLE_INTIFY_, ASSERT_ENABLE_ASSUME)

#define ASSERT_DISCARD_(...) ((void)0)
#define ASSERT_ASSUME_(...)                                            \
  BOOST_PP_IIF(ASSERT_ENABLE_ASSUME_INTED,                             \
               HEDLEY_ASSUME(__VA_ARGS__),                             \
               ASSERT_DISCARD_(__VA_ARGS__))

#define ASSERT_ATLEAST(min_level, ...)                                 \
  BOOST_PP_IIF(BOOST_PP_LESS_EQUAL(min_level, ASSERT_LEVEL),           \
               ASSERTER(__VA_ARGS__),                                  \
               ASSERT_ASSUME_(__VA_ARGS__))

#define ASSERT_RELEASE(...)                                            \
  ASSERT_ATLEAST(ASSERT_LEVEL_RELEASE, __VA_ARGS__)
#define ASSERT(...) ASSERT_ATLEAST(ASSERT_LEVEL_DEBUG, __VA_ARGS__)
#define ASSERT_PARANOID(...)                                           \
  ASSERT_ATLEAST(ASSERT_LEVEL_PARANOID, __VA_ARGS__)

#endif
