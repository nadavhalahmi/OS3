cmake_minimum_required(VERSION 3.7)
project(HW3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES Factory.h Product.h test_utilities.h Factory.cxx test_din.cpp Factory_test.cpp test.cxx)
add_executable(HW3 ${SOURCE_FILES})