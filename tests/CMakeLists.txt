enable_testing()

find_package(Catch2 REQUIRED)

include(Catch)

add_executable(pa171_tests)
catch_discover_tests(pa171_tests)
target_link_libraries(
  pa171_tests
  PRIVATE
  pa171_lib
  catch2::catch2
)

add_subdirectory(pa171)
