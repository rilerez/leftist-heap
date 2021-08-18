#ifndef LEFTIST_HEAP_HPP_INCLUDE_GUARD
#define LEFTIST_HEAP_HPP_INCLUDE_GUARD

#include "macros.hpp"
#include "asserts.hpp"

#include <memory>
#include <algorithm>
#include <ranges>

template<class T>
struct vector_mem {
  // probably needs better name. Pointdex? Inder?
  // key handle reference
  using Index = size_t;
  std::vector<T>* block;

  T const& operator[](Index i) const {
    ASSERT(!is_null(i));
    return (*block)[i - 1];
  }

  Index null() const { return 0; }
  bool  is_null(Index i) const { return i == 0; }

  Index make_index(auto&&... args) {
    block->emplace_back(FWD(args)...);
    return block->size();
  }
};

template<class T>
struct shared_ptr_mem {
  using Index = std::shared_ptr<void>;

  T const& operator[](Index const& i) const {
    ASSERT(!is_null(i));
    return *static_cast<T*>(i.get());
  }

  Index null() const { return {}; }
  bool  is_null(Index const& x) const { return x == nullptr; }
  auto make_index(auto&&... args) ARROW(std::make_shared<T>(FWD(args)...))
};

template<class Less = std::less<>>
constexpr auto cmp_by(auto f, Less less = {}) noexcept {
  return [=] FN(less(f(_0), f(_1)));
}

template<class T, class index, class Rank = int>
struct Node {
  using Index = index;
  T     elt;
  Rank  rank;
  Index left;
  Index right;

  static Rank rank_of(auto& mem, Index n) {
    return mem.is_null(n) ? Rank{} : mem[n].rank;
  }

  static Index make(auto& mem, T e, Index node1, Index node2) {
    auto [r, l] =
        std::minmax(node1, node2, cmp_by([&] FN(rank_of(mem, _))));
    auto old_rank = rank_of(mem, r);
    return mem.template make_index(
        Node{.elt = e, .rank = old_rank + 1, .left = l, .right = r});
  }

  static Index merge(auto& mem, auto less, Index node1, Index node2) {
    if(mem.is_null(node1)) return node2;
    else if(mem.is_null(node2))
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

  template<class size_type>
  static size_type count(auto const& mem, Index node) {
    return mem.is_null(node) ? size_type{}
                             : (size_type{1} + count(mem[node].left)
                                + count(mem[node].right));
  }
};

template<class T, class Less, template<class> class Mem_, class Node>
class Heap {
  using node      = Node;
  using Mem       = Mem_<node>;
  using idx       = typename node::Index;
  using size_type = std::size_t;

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
    return {mem_,
            less_,
            node::merge(mem_, less_, mem_[root_].left, mem_[root_].right)};
  }

  Heap cons(T e) const {
    return {mem_,
            less_,
            node::merge(mem_,
                        less_,
                        root_,
                        node::make(mem_, e, mem_.null(), mem_.null()))};
  }

  size_type size() const {
    return node::template count<size_type>(root_);
  }
};

template<class Coll, std::ranges::range Rng>
inline auto into(Coll coll, Rng const data) {
  for(auto x : data) coll = coll.cons(x);
  return coll;
}
#endif // LEFTIST_HEAP_HPP_INCLUDE_GUARD
