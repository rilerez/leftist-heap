#ifndef SORT_TUPLE_HPP_INCLUDE_GUARD
#define SORT_TUPLE_HPP_INCLUDE_GUARD

namespace say_impl {
template<class>
struct say_t;
template<auto>
struct say_v;
} // namespace say_impl

#define SAY_V(...) static_assert(::say_impl::say_v<(__VA_ARGS__)>{})
#define SAY_T(...) static_assert(::say_impl::say_t<(__VA_ARGS__)>{})

#include "macros.hpp"
#include "cmp.hpp"

#include <tuple>
#include <array>
#include <numeric>
#include <algorithm>

template<size_t N>
using permutation = std::array<size_t, N>;

template<size_t N>
constexpr permutation<N> invert(permutation<N> permutation) {
  std::array<size_t, N> inv;
  for(size_t i = 0; i < N; ++i) inv[permutation[i]] = i;
  return inv;
}

// σ ∈ Sn
// (σx)[i] = x[σ⁻¹i]
template<auto invperm, auto... Is>
constexpr auto permute_tuple_by_inv_(auto tup, std::index_sequence<Is...>)
    ARROW(std::tuple{std::get<invperm[Is]>(tup)...})
template<auto invperm>
constexpr auto permute_tuple_by_inv(auto tup)
    ARROW(permute_tuple_by_inv_<invperm>(
        tup,
        std::make_index_sequence<std::tuple_size_v<decltype(tup)>>{}));

template<auto perm>
constexpr auto permute_tuple(auto tup)
    ARROW(permute_tuple_by_inv<invert(perm)>(tup))

template<class... Args>
class arg_list_sorter {
 private:
  using unsorted_tuple = std::tuple<Args...>;
  static constexpr std::array sizes{sizeof(Args)...};

  static constexpr auto count = sizeof...(Args);

 public:
  static constexpr std::array sort2ext_indices = [] {
    std::array<size_t, count> inds;
    std::iota(std::begin(inds), std::end(inds), 0);
    std::sort(std::begin(inds),
              std::end(inds),
              cmp_by([] FN(sizes[_]), std::greater<> {}));
    return inds;
  }();

  static constexpr std::array ext2sort_indices = invert(sort2ext_indices);

 public:
  static constexpr auto sort(unsorted_tuple tup)
      NOEX(permute_tuple_by_inv<sort2ext_indices>(tup))

  using sorted_tuple = decltype(sort(std::declval<unsorted_tuple>()));
};
static_assert(
    std::is_same_v<
        std::tuple_element_t<0, arg_list_sorter<int, char>::sorted_tuple>,
        int>);

template<class... Ts>
class size_sorted_tuple {
  using sorter = arg_list_sorter<Ts...>;
  typename sorter::sorted_tuple underlying_;

 public:
  // do I need to overload on const myself?
  template<auto n>
  constexpr auto get()
      ARROW(std::get<sorter::ext2sort_indices[n]>(underlying_))
  template<class T>
  constexpr auto get() ARROW(std::get<T>(underlying_))

  constexpr static auto size = sizeof...(Ts);

  constexpr size_sorted_tuple(std::tuple<Ts...> args)
      : underlying_{sorter::sort(args)} {}

  constexpr size_sorted_tuple(Ts const&... args)
      : size_sorted_tuple(std::tuple{args...}) {}
};

template<class... Ts>
struct std::tuple_size<size_sorted_tuple<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

struct test_node {
  int        elt;
  test_node* left;
  test_node* right;
  int        rank;
};
// SAY_V(sizeof(test_node));
// SAY_V(sizeof(size_sorted_tuple<int, void*, void*, int>));

class packed_test_node {
  // int elt;
  // void* left;
  // void* right;
  // int rank;

  // clang-format off
  enum             { elt_, left_, right_, rank_ };
  size_sorted_tuple<int  , void*, void* , int   > data_;
  // clang-format on

 public:
#define TUPLE_READER(name) constexpr auto name() NOEX(data_.get<name##_>())
  TUPLE_READER(elt)
  TUPLE_READER(left)
  TUPLE_READER(right)
  TUPLE_READER(rank)

  constexpr packed_test_node(int elt, void* left, void* right, int rank)
      : data_{elt, left, right, rank} {}
};
static_assert(packed_test_node{1, 0, 0, 2}.rank() == 2);
#endif // SORT_TUPLE_HPP_INCLUDE_GUARD
