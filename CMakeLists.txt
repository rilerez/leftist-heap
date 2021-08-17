cmake_minimum_required(VERSION 3.20)

project(
  leftist_heap
  DESCRIPTION "Proof of concept for functional data structures with \
 pluggable memory management schemes"
  VERSION 0.0.0
  LANGUAGES CXX)

include(early_hook.cmake)

add_library(leftist_heap INTERFACE)
add_library(leftist_heap::leftist_heap ALIAS leftist_heap)

# find_package(hedley)

target_include_directories(leftist_heap INTERFACE include)
target_compile_features(leftist_heap INTERFACE cxx_std_20)

add_subdirectory(test)
