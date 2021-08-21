#ifndef LEFTIST_HEAP_HPP_INCLUDE_GUARD
#define LEFTIST_HEAP_HPP_INCLUDE_GUARD

#include "macros.hpp"
#include "asserts.hpp"

#include <numeric>
#include <memory>
#include <algorithm>
#include <ranges>
#include <limits>
#include <cstdint>
//#include <execution> //tbb is giving me linker errors

constexpr bool noex_assert(int level) {
  return noexcept([] { ASSERT_HANDLER(false); }) && level <= ASSERT_LEVEL;
}

template<class To, class From>
constexpr To narrow(From const x) noexcept(noex_assert(ASSERT_LEVEL_DEBUG)) {
  To y = static_cast<To>(x);
  ASSERT(x == static_cast<From>(y));
  return y;
}

template<class T>
constexpr bool easy_to_copy =
    std::is_trivially_copyable_v<T> && sizeof(T) <= 4 * sizeof(void*);

template<class T>
using Read = std::conditional_t<easy_to_copy<T>, T const, T const&>;
template<class T>
using ReadReturn = std::conditional_t<easy_to_copy<T>, T, T const&>;

static_assert(std::is_reference_v<Read<std::shared_ptr<void>>>);
static_assert(!std::is_reference_v<Read<int>>);

template<class T,
         class vector = std::vector<T>,
         class Key_   = typename vector::size_type>
// It makes sense to use this with a boost::static_vector for example
// Or to use different allocators.
struct vector_mem {
  using Key = Key_;
  vector* block;

  // noexing this crashes clang too
  constexpr T const& operator[](Key i) const
      noexcept(noex_assert(ASSERT_LEVEL_DEBUG)) {
    ASSERT(!is_null(i));
    ASSERT_PARANOID(0 < i);
    WITH_DIAGNOSTIC_TWEAK(IGNORE_WASSUME,
                          ASSERT_PARANOID(i <= block->size());)
    // why does clang think vector::size() has sideffects?
    return (*block)[i - 1];
  }

  constexpr Key  null() const noexcept { return 0; }
  constexpr bool is_null(Read<Key> i) const noexcept { return i == 0; }

  constexpr Key make_key(auto&&... args)
      NOEX(block->emplace_back(FWD(args)...), block->size())
};

template<class T>
struct shared_ptr_mem {
  using Key = std::shared_ptr<void>;

  // NOEX-ing this crashed clangd and clang-tidy?
  T const& operator[](Key const& i) const
      noexcept(noex_assert(ASSERT_LEVEL_DEBUG)) {
    ASSERT(!is_null(i));
    return *static_cast<T*>(i.get());
  }

  Key  null() const { return {}; }
  bool is_null(Key const& x) const NOEX(x == nullptr)
  auto make_key(auto&&... args)
      ARROW(Key{std::make_shared<T>(FWD(args)...)})
};

// TODO: what is the right semantics for const, thread safety, and mems?

template<class Less = std::less<>>
constexpr auto cmp_by(auto&& f, Less&& less = {}) {
  return [ f = FWD(f), less = FWD(less) ] FN(less(f(_0), f(_1)));
}

template<class T, class key, class Rank = std::uint8_t>
struct Node {
  using element_t = T;
  using Key       = key;
  // TODO: how much can these be compressed?
  // realistically, Key cannot be smaller than uint8: 2^4= 16 very small heap
  // when is it small enough to be worth just copying a vector/vector heap?
  T    elt;
  Key  left;
  Key  right;
  Rank rank; // should we store rank-1 instead? Might increase range but
             // complicates logic
  // rank <= floor(log(n+1))
  // Okasaki, Purely functional data structures Exercise 3.1
  // if key=uint64
  // max # is #uint64s - 1 (-1 for null) = uint64max = 2^64-1
  // so rank <= 64, within uint8_t

  constexpr static Rank rank_of(auto const mem, Read<Key> n)
      NOEX(mem.is_null(n) ? Rank{} : mem[n].rank)

  constexpr static Key
      make(auto mem, T e, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(rank_of(mem, node1),
                   mem.make_key(node1),
                   narrow<Rank>(Rank{} + 1))) {
    auto const& [r, l] =
        std::minmax(node1, node2, cmp_by([=] FN(rank_of(mem, _))));
    auto const old_rank = rank_of(mem, r);
    return mem.template make_key(
        Node{.elt   = std::move(e),
             .left  = l,
             .right = r,
             .rank  = narrow<Rank>(old_rank + 1)});
  }

  constexpr static Key
      merge(auto mem, auto less, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(mem.is_null(node1),
                   less(mem[node2].elt, mem[node1].elt),
                   make(mem, mem[node1].elt, node1, node2))) {
    return mem.is_null(node1) ? node2
         : mem.is_null(node2) ? node1
         : less(mem[node2].elt, mem[node1].elt)
             ? make(mem,
                    mem[node2].elt,
                    mem[node2].left,
                    merge(mem, less, node1, mem[node2].right))
             : make(mem,
                    mem[node1].elt,
                    mem[node1].left,
                    merge(mem, less, node2, mem[node1].right));
    // always merge with the right b/c of leftist property
  }
};

template<class T, class key, class Weight = std::size_t>
struct WeightNode {
  using element_t = T;
  using Key       = key;
  // TODO: how much can these be compressed?
  // realistically, Key cannot be smaller than uint8: 2^4= 16 very small heap
  // when is it small enough to be worth just copying a vector/vector heap?
  T      elt;
  Key    left;
  Key    right;
  Weight weight; // should we store rank-1 instead? Might increase range
                 // but complicates logic
  // rank <= floor(log(n+1))
  // Okasaki, Purely functional data structures Exercise 3.1
  // if key=uint64
  // max # is #uint64s - 1 (-1 for null) = uint64max = 2^64-1
  // so rank <= 64, within uint8_t

  constexpr static Weight weight_of(auto const mem, Read<Key> n)
      NOEX(mem.is_null(n) ? Weight{} : mem[n].weight)

  constexpr static Key
      make(auto mem, T e, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(rank_of(mem, node1),
                   mem.make_key(node1),
                   narrow<Weight>(Weight{} + 1))) {
    auto const& [r, l] =
        std::minmax(node1, node2, cmp_by([=] FN(weight_of(mem, _))));
    return mem.template make_key(
        Node{.elt    = std::move(e),
             .left   = l,
             .right  = r,
             .weight = narrow<Weight>(1 + l.weight + r.weight)});
  }

  constexpr static Key
      merge(auto mem, auto less, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(mem.is_null(node1),
                   less(mem[node2].elt, mem[node1].elt),
                   make(mem, mem[node1].elt, node1, node2))) {
    return mem.is_null(node1) ? node2
         : mem.is_null(node2) ? node1
         : less(mem[node2].elt, mem[node1].elt)
             ? make(mem,
                    mem[node2].elt,
                    mem[node2].left,
                    merge(mem, less, node1, mem[node2].right))
             : make(mem,
                    mem[node1].elt,
                    mem[node1].left,
                    merge(mem, less, node2, mem[node1].right));
    // always merge with the right b/c of leftist property
  }
};

template<class Node_>
struct NodeUtil {
  using Node = Node_;
  using Key  = typename Node::Key;
  using T    = typename Node::element_t;

  constexpr static auto make1(auto mem, auto e)
      ARROW(Node::make(mem, e, mem.null(), mem.null()))

  constexpr static ReadReturn<T> peek(auto const mem, Read<Key> k)
      NOEX(mem[k].elt)
  constexpr static Key pop(auto mem, auto less, Read<Key> k)
      NOEX(Node::merge(mem, less, mem[k].left, mem[k].right))

  constexpr static auto cons(auto mem, auto less, T e, Read<Key> node1)
      ARROW(Node::merge(mem, less, node1, make1(mem, e)))

  template<class Mem>
  static constexpr bool is_counted_node = requires(Mem mem, Node node) {
    Node::counted(mem, node);
  };

  template<class Mem, class _>
  requires(is_counted_node<Mem>) //
      constexpr static auto count(Mem const mem, Read<Key> node)
          NOEX(Node::count(mem, node))

  template<class size_type>
  constexpr static size_type count(auto const mem, Read<Key> node) NOEX(
      mem.is_null(node)
          ? size_type{}
          : (size_type{1} + count(mem[node].left) + count(mem[node].right)))
};

// we already need to say the mem and node types in order to construct
// the vector for the vector memory
template<class T, class Less, class Mem_, class Node_>
// TODO: Are there other useful definitions of Node?
// if element has ~6 unused bits, we can put rank in it
// We can also implement a weighted leftist heap by only changing Node_
class Heap {
  using Node = Node_;
  using Mem  = Mem_;
  using Key  = typename Node::Key;

  using NodeU = NodeUtil<Node>;

  [[no_unique_address]] Less        less_;
  [[no_unique_address]] mutable Mem mem_;
  Key                               root_;

  constexpr Heap(Mem mem, Less less, Key h)
      : less_{std::move(less)}, mem_{std::move(mem)}, root_{std::move(h)} {}

 public:
  using size_type = std::size_t;
  constexpr explicit Heap(Mem mem = {}, Less less = {})
      : Heap(mem, std::move(less), mem.null()) {}

  constexpr bool empty() const NOEX(mem_.is_null(root_))

  constexpr ReadReturn<T> peek() const NOEX(NodeU::peek(mem_, root_))

  constexpr Heap pop() const
      NOEX(Heap{mem_, less_, NodeU::pop(mem_, less_, root_)})

  constexpr Heap cons(T e) const
      RET(Heap{mem_, less_, NodeU::cons(mem_, less_, e, root_)})

  constexpr size_type size() const
      NOEX(NodeU::template count<size_type>(root_))
};

auto into(auto coll, auto data) {
  for(auto&& d : data) coll = coll.cons(d);
  return coll;
}
#endif // LEFTIST_HEAP_HPP_INCLUDE_GUARD
