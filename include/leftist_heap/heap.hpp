#ifndef LEFTIST_HEAP_HPP_INCLUDE_GUARD
#define LEFTIST_HEAP_HPP_INCLUDE_GUARD

#include "cmp.hpp"
#include "read.hpp"
#include "macros.hpp"
#include "accessors.hpp"

#include <numeric>
#include <memory>
#include <algorithm>
#include <ranges>
#include <limits>
#include <cstdint>
//#include <execution> //tbb is giving me linker errors

static constexpr bool noex_assert = LEFTIST_HEAP_ASSERT_NOEXCEPT;

template<class To, class From>
constexpr To narrow(From const x) noexcept(noex_assert) {
  To y = static_cast<To>(x);
  LEFTIST_HEAP_ASSERT(x == static_cast<From>(y));
  return y;
}

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
  constexpr T const& operator[](Key i) const noexcept(noex_assert) {
    LEFTIST_HEAP_ASSERT(!is_null(i));
    LEFTIST_HEAP_ASSERT(0 < i);
    LEFTIST_HEAP_ASSERT(i <= block->size());
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
  T const& operator[](Key const& i) const noexcept(noex_assert) {
    LEFTIST_HEAP_ASSERT(!is_null(i));
    return *static_cast<T*>(i.get());
  }

  Key  null() const { return {}; }
  bool is_null(Key const& x) const NOEX(x == nullptr)
  auto make_key(auto&&... args)
      ARROW(Key{std::make_shared<T>(FWD(args)...)})
};

// TODO: what is the right semantics for const, thread safety, and mems?


// TODO: what about storing nodes as SOA?
// I can make it work as is but mem[key] will need to be a proxy holding (ptr,index)
// this will couple node with mem more tightly for SOA (some of that is the nature of SOA)
// but it will lead to some duplication for make, etc
//
// alternately, factor out a NodeData struct?
template<class T, class key, class Rank = std::uint8_t>
class Node {
 public:
  using element_t = T;
  using Key       = key;

 private:
  T    elt_;
  Key  left_;
  Key  right_;
  Rank rank_; // should we store rank-1 instead? Might increase range but
              // complicates logic
  // rank <= floor(log(n+1))
  // Okasaki, Purely functional data structures Exercise 3.1
  // if key=uint64
  // max # is #uint64s - 1 (-1 for null) = uint64max = 2^64-1
  // so rank <= 64, within uint8_t

  constexpr static Rank rank_of(auto const mem, Read<Key> n)
      NOEX(mem.is_null(n) ? Rank{} : mem[n].rank_)

  struct permission2construct {
    friend Node;

   private:
    permission2construct() = default;
  };

 public:
  // TODO: aggregate vs private
  Node() = default;
  Node(permission2construct, T elt, Key left, Key right, Rank rank)
      : elt_{elt}, left_{left}, right_{right}, rank_{rank} {}

  READER(elt)
  READER(left)
  READER(right)

  constexpr static Key
      make(auto mem, T e, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(rank_of(mem, node1),
                   mem.make_key(node1),
                   narrow<Rank>(Rank{} + 1))) {
    auto const& [r, l] =
        std::minmax(node1, node2, cmp_by([=] FN(rank_of(mem, _))));
    auto const old_rank = rank_of(mem, r);
    auto const new_rank = narrow<Rank>(old_rank + 1);
    return mem.template make_key(
        permission2construct{},
        std::move(e),
        l,
        r,
        new_rank);
  }

  constexpr static auto make1(auto mem, auto e)
      ARROW(make(mem, e, mem.null(), mem.null()))

  constexpr static Key
      merge(auto mem, auto less, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(mem.is_null(node1),
                   less(mem[node2].elt(), mem[node1].elt()),
                   make(mem, mem[node1].elt(), node1, node2))) {
    return mem.is_null(node1) ? node2
         : mem.is_null(node2) ? node1
         : less(mem[node2].elt(),
                mem[node1].elt()) // TODO: what to do for ties?
             ? make(mem,
                    mem[node2].elt(),
                    mem[node2].left(),
                    merge(mem, less, node1, mem[node2].right()))
             : make(mem,
                    mem[node1].elt(),
                    mem[node1].left(),
                    merge(mem, less, node2, mem[node1].right()));
    // always merge with the right b/c of leftist property
  }
};

template<class T, class key, class Weight = std::size_t>
class WeightNode {
 public:
  using element_t = T;
  using Key       = key;

 private:
  T      elt_;
  Key    left_;
  Key    right_;
  Weight weight_;

  struct permission2construct {
    friend WeightNode;

   private:
    permission2construct() = default;
  };

  constexpr static Weight weight_of(auto const mem, Read<Key> n)
      NOEX(mem.is_null(n) ? Weight{} : mem[n].weight)

  constexpr static Key
      make_ug(auto mem, T e, Read<Key> l, Read<Key> r, Weight w) NOEX(
          mem.template make_key(permission2construct{}, std::move(e), l, r, w))

 public:
  WeightNode() = default;
  WeightNode(permission2construct, T elt, Key left, Key right, Weight weight)
      : elt_{elt}, left_{left}, right_{right}, weight_{weight} {}

  READER(elt)
  READER(left)
  READER(right)

  constexpr static auto make1(auto mem, auto e) ARROW(mem.template make_key(
      permission2construct{},
      e,
      mem.null(),
      mem.null(),
      1))

  // TODO: local copy the node, or all the node but the element, to hint to
  // the compiler it won't change?
  constexpr static Key
      merge(auto mem, auto less, Read<Key> node1, Read<Key> node2) noexcept(
          noexcept(mem.is_null(node1),
                   less(mem[node2].elt(), mem[node1].elt()),
                   make(mem, mem[node1].elt(), node1, node2))) {
    if(mem.is_null(node1)) return node2;
    if(mem.is_null(node2)) return node1;

    auto const w = weight_of(mem, node1) + weight_of(mem, node2);
    auto const& [small, big] =
        std::minmax(node1, node2, cmp_by([=] FN(_.elt()), less));
    auto const  mixed = merge(mem, less, mem[small].right, mem[big]);
    auto const& same  = mem[small].left;
    auto const  mixed_weight =
        weight_of(mem, mem[small].right) + weight_of(mem, big);
    auto const same_weight = weight_of(mem, same);

    auto const& [light, heavy] =
        (same_weight < mixed_weight)
            ? std::pair{same, mixed}
            : std::pair{mixed, same};
    return make_ug(mem,
                   less,
                   mem[small].elt,
                   light,
                   heavy,
                   mixed_weight + same_weight + 1);
  }

  constexpr static auto count(auto const mem, Read<Key> node)
      ARROW(weight_of(mem, node))
};

template<class Node_>
struct NodeUtil {
  using Node = Node_;
  using Key  = typename Node::Key;
  using T    = typename Node::element_t;

  constexpr static ReadReturn<T> peek(auto const mem, Read<Key> k)
      NOEX(mem[k].elt())
  constexpr static Key pop(auto mem, auto less, Read<Key> k)
      NOEX(Node::merge(mem, less, mem[k].left(), mem[k].right()))

  constexpr static auto cons(auto mem, auto less, T e, Read<Key> node1)
      ARROW(Node::merge(mem, less, node1, Node::make1(mem, std::move(e))))

  template<class Mem>
  static constexpr bool is_counted_node = requires(Mem mem, Node node) {
    Node::count(mem, node);
  };

  template<class Mem, class _>
  requires(is_counted_node<Mem>) //
      constexpr static auto count(Mem const mem, Read<Key> node)
          NOEX(Node::count(mem, node))

  template<class size_type>
  constexpr static size_type count(auto const mem, Read<Key> node)
      NOEX(mem.is_null(node)
               ? size_type{}
               : (size_type{1} + count(mem[node].left())
                  + count(mem[node].right())))
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

  constexpr auto peek() const ARROW(NodeU::peek(mem_, root_))

  constexpr Heap pop() const
      NOEX(Heap{mem_, less_, NodeU::pop(mem_, less_, root_)})

  constexpr Heap cons(T e) const
      NOEX(Heap{mem_, less_, NodeU::cons(mem_, less_, std::move(e), root_)})

  constexpr size_type size() const
      NOEX(NodeU::template count<size_type>(root_))
};

auto into(auto coll, auto data) {
  for(auto&& d : data) coll = coll.cons(d);
  return coll;
}
#endif // LEFTIST_HEAP_HPP_INCLUDE_GUARD
