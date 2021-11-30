find_package(absl REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
find_package(lyra REQUIRED)
find_package(stb REQUIRED)

add_library(${PROJECT_NAME}_lib)
target_compile_features(
  ${PROJECT_NAME}_lib
  PUBLIC
  cxx_std_20
)
target_include_directories(
  ${PROJECT_NAME}_lib
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  ${PROJECT_NAME}_lib
  PUBLIC
  absl::flat_hash_map
  Boost::Boost
  fmt::fmt
  stb::stb
)

add_executable(${PROJECT_NAME})
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}_lib
)

add_subdirectory(pa171)
