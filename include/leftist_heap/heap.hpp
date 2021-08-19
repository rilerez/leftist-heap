#ifndef LEFTIST_HEAP_HPP_INCLUDE_GUARD
#define LEFTIST_HEAP_HPP_INCLUDE_GUARD

#include "macros.hpp"
#include "asserts.hpp"

#include <memory>
#include <algorithm>
#include <ranges>

#include <limits>
#include <cstdint>

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
constexpr bool easy_to_copy<std::shared_ptr<T>> = false;

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

  Key  null() const noexcept { return {}; }
  bool is_null(Key const& x) const NOEX(x == nullptr)
  auto make_key(auto&&... args) ARROW(std::make_shared<T>(FWD(args)...))
};

// TODO: what is the right semantics for const, thread safety, and mems?

template<class Less = std::less<>>
constexpr auto cmp_by(auto&& f, Less&& less = {}) noexcept {
  return [ f = FWD(f), less = FWD(less) ] FN(less(f(_0), f(_1)));
}

template<class T, class key, class Rank = std::uint8_t>
struct Node {
  using Key = key;
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
      make(auto mem, T e, Read<Key> node1, Read<Key> node2) //
      noexcept(noex_assert(ASSERT_LEVEL_DEBUG)) {
    auto const& [r, l] =
        std::minmax(node1, node2, cmp_by([&] FN(rank_of(mem, _))));
    auto const old_rank = rank_of(mem, r);
    return mem.template make_key(
        Node{.elt   = std::move(e),
             .left  = l,
             .right = r,
             .rank  = narrow<Rank>(old_rank + 1)});
  }

  constexpr static Key
      merge(auto mem, auto less, Read<Key> node1, Read<Key> node2) //
      noexcept(noexcept(mem.is_null(node1)) &&                     //
               noexcept(less(mem[node2].elt, mem[node1].elt)) &&   //
               noexcept(make(mem, mem[node1].elt, node1, node2))) {
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

  [[no_unique_address]] Less        less_;
  [[no_unique_address]] mutable Mem mem_;
  Key                               root_{};

  constexpr Heap(Mem mem, Less less, Key h)
      : less_{std::move(less)}, mem_{std::move(mem)}, root_{std::move(h)} {}

 public:
  using size_type = std::size_t;
  constexpr explicit Heap(Mem mem = {}, Less less = {})
      : less_{std::move(less)}, mem_{std::move(mem)} {}

  constexpr bool empty() const NOEX(mem_.is_null(root_))

  constexpr ReadReturn<T> peek() const
      NOEX(ASSERT(!empty()), mem_[root_].elt)

  constexpr Heap pop() const NOEX(
      ASSERT(!empty()),
      Heap{mem_,
           less_,
           Node::merge(mem_, less_, mem_[root_].left, mem_[root_].right)})

  constexpr Heap cons(T e) const
      NOEX(Heap{mem_,
                less_,
                Node::merge(mem_,
                            less_,
                            root_,
                            Node::make(mem_, e, mem_.null(), mem_.null()))})

  constexpr size_type size() const
      NOEX(Node::template count<size_type>(root_))
};

template<class Coll, std::ranges::range Rng>
constexpr inline auto into(Coll coll, Rng const data) {
  for(auto x : data) coll = coll.cons(x);
  return coll;
}
#endif // LEFTIST_HEAP_HPP_INCLUDE_GUARD
