# test/CMakeLists.txt

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.13.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(ntd_tests
  test_safe_mutex.cpp
  test_namedthread.cpp
  test_safe_mutex.cpp
)

target_link_libraries(ntd_tests PRIVATE
  ntd::ntd
  Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(ntd_tests)
