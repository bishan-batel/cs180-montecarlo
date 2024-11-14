cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  cachelist
  LANGUAGES C
  VERSION 0.1)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
    ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})

add_compile_options(-fdiagnostics-color=always)
add_compile_options(-Wextra -Wall -Wno-unused-result -Wpedantic)

add_executable(montecarlo src/ThreadSafe_PRNG.c src/main.c src/mc-thread.c)
