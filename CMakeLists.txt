cmake_minimum_required(VERSION 3.14)
project(orderbook_forge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Main library target ---
add_library(orderbook_lib
    src/OrderBook.cpp
)
target_include_directories(orderbook_lib PUBLIC include)

# --- GoogleTest via FetchContent (no manual install needed) ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent GoogleTest from overriding our compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# --- Test target ---
add_executable(orderbook_tests
    tests/test_order_book.cpp
)
target_link_libraries(orderbook_tests
    orderbook_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)