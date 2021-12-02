enable_testing()

find_package(Catch2 REQUIRED)

include(Catch)

add_executable(${PROJECT_NAME}_tests)
catch_discover_tests(${PROJECT_NAME}_tests)
target_link_libraries(
  ${PROJECT_NAME}_tests
  PRIVATE
  ${PROJECT_NAME}_lib
  catch2::catch2
)

add_subdirectory(pa171)
