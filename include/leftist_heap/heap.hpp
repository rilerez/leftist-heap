#ifndef LEFTIST_HEAP_HPP_INCLUDE_GUARD
#define LEFTIST_HEAP_HPP_INCLUDE_GUARD

#include "macros.hpp"
#include "asserts.hpp"

#include <memory>
#include <algorithm>
#include <ranges>

template<class T>
struct vecmem {
  using Index = unsigned int;
  std::vector<T>* block;

  T const& operator[](Index i) const {
    ASSERT(!is_null(i));
    return (*block)[i - 1];
  }

  bool is_null(Index i) const { return i; }

  Index make_index(auto&&... args) {
   block->emplace_back(FWD(args)...);
   return block->size();
  }
};

template<class T>
struct shared_ptr_mem {
  using Index = std::shared_ptr<T>;

  T const& operator[](Index i) const {
    ASSERT(!is_null(i));
    return *i;
  }

  bool is_null(Index x) const { return x == nullptr; }
  auto make_index(auto&&... args) ARROW(std::make_shared<T>(FWD(args)...))
};

template<class Less = std::less<>>
constexpr auto cmp_by(auto f, Less less = {}) noexcept {
  return [=] FN(less(f(_0), f(_1)));
}

template<class T, template<class> class Mem, class N = int>
struct Node {
  using Index = typename Mem<Node>::Index;
  T     elt;
  N     rank;
  Index left;
  Index right;

  static N rank_of(auto& mem, Index n) {
    return mem.is_null(n) ? 0 : mem[n].rank;
  }

  static Index make(auto& mem, T e, Index node1, Index node2) {
    auto [r, l] =
        std::minmax(node1, node2, cmp_by([&] FN(rank_of(mem, _))));
    auto old_rank = rank_of(mem, r);

    return mem.template make_index(
        Node{.elt = e, .rank = old_rank + 1, .left = l, .right = r});
  }

  static Index merge(auto& mem, auto less, Index node1, Index node2) {
    if(!node1) return node2;
    else if(!node2)
      return node1;
    else if(less(mem[node2].elt, mem[node1].elt))
      return make(mem,
                  mem[node2].elt,
                  mem[node2].left,
                  merge(mem, less, node1, mem[node2].left));
    else
      return make(mem,
                  mem[node1].elt,
                  mem[node1].left,
                  merge(mem, less, mem[node1].right, node2));
  }

  static size_t count(auto const& mem, Index node) {
    if(!node) return 0;
    else
      return 1 + count(mem[node].left) + count(mem[node].right);
  }
};

template<class T, class Less=std::less<>, template<class> class Mem_ = shared_ptr_mem>
class Heap {
  using node = Node<T, Mem_>;
  using Mem  = Mem_<node>;
  using idx  = typename node::Index;

  [[no_unique_address]] Less        less_;
  [[no_unique_address]] mutable Mem mem_;
  idx                               root_{};

  Heap(Mem mem, Less less, idx h) : less_{less}, mem_{mem}, root_{h} {}

 public:
  explicit Heap(Mem mem = {}, Less less = {})
      : less_{less}, mem_{mem} {}

  bool empty() const { return mem_.is_null(root_); }

  T const& peek() const {
    ASSERT(!empty());
    return mem_[root_].elt;
  }

  Heap pop() const {
    ASSERT(!empty());
    return Heap{
        mem_,
        less_,
        node::merge(mem_, less_, mem_[root_].left, mem_[root_].right)};
  }

  Heap cons(T e) const {
    return {
        mem_,
        less_,
        node::merge(mem_, less_, root_, node::make(mem_, e, idx{}, idx{}))};
  }
};

template<class Coll, class T>
concept CollOf = requires(Coll c, T x) {
  { c.cons(x) } -> std::same_as<Coll>;
};

template<class Coll, std::ranges::range Rng>
requires CollOf<Coll, std::ranges::range_value_t<Rng>>
inline auto into(Coll coll, Rng const data) {
  for(auto x : data) coll = coll.cons(x);
  return coll;
}
#endif // LEFTIST_HEAP_HPP_INCLUDE_GUARD
