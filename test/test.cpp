#define CATCH_CONFIG_MAIN

#include <leftist_heap/heap.hpp>

#include <catch2/catch.hpp>

TEST_CASE("A new Heap is empty"){
  Heap<int,std::less<>, shared_ptr_mem> h{};
  REQUIRE(h.empty());
}

TEST_CASE("A heap with one element is not empty"){
  Heap<int,std::less<>, shared_ptr_mem> h0{};
  auto h1 = into(h0, std::initializer_list<int>{3});
  REQUIRE(!h1.empty());
}

TEST_CASE("Peeking a heap with one element gives you that element"){
  Heap<int,std::less<>, shared_ptr_mem> h0{};
  auto h1 = into(h0, std::initializer_list<int>{3});
  REQUIRE(h1.peek()==3);
}

TEST_CASE("Popping a heap with one element gives you the empty heap"){
  Heap<int,std::less<>, shared_ptr_mem> h0{};
  auto h1 = into(h0, std::initializer_list<int>{3});
  REQUIRE(h1.pop().empty());
}
