cmake_minimum_required(VERSION 4.3.0)

project(ntd VERSION 1.0.0 LANGUAGES CXX)

add_library(ntd INTERFACE)
add_library(ntd::ntd ALIAS ntd)

target_compile_features(ntd INTERFACE cxx_std_23)

target_sources(ntd INTERFACE
  FILE_SET HEADERS
  BASE_DIRS include
  FILES
    include/ntd/config.hpp
    include/ntd/namedthread.hpp
    include/ntd/safe_mutex.hpp
    include/ntd/sync_queue.hpp
    include/ntd/detail/thread_utils.hpp
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_link_libraries(ntd INTERFACE stdc++exp)
endif()

option(NTD_BUILD_TESTS "Build the NTD library tests" OFF)
if(NTD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

install(TARGETS ntd
    EXPORT ntd_targets
    FILE_SET HEADERS DESTINATION include
)

install(EXPORT ntd_targets
    FILE ntdConfig.cmake
    NAMESPACE ntd::
    DESTINATION lib/cmake/ntd
)
