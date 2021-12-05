find_package(abseil REQUIRED)
find_package(fmt REQUIRED)
find_package(lyra REQUIRED)
find_package(range-v3 REQUIRED)
find_package(stb REQUIRED)

add_library(pa171)
target_compile_features(
  pa171
  PUBLIC
  cxx_std_20
)
target_include_directories(
  pa171
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
  pa171
  PUBLIC
  abseil::absl_flat_hash_map
  range-v3::range-v3
  stb::stb
)

add_executable(pa171_compress)
target_link_libraries(
  pa171_compress
  PRIVATE
  pa171
  fmt::fmt
  lyra::lyra
)

add_executable(pa171_decompress)
target_link_libraries(
  pa171_decompress
  PRIVATE
  pa171
  fmt::fmt
  lyra::lyra
)

add_subdirectory(pa171)
