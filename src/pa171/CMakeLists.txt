add_subdirectory(coding)
add_subdirectory(quantization)
add_subdirectory(transform)
add_subdirectory(utils)

target_sources(
  pa171
  PRIVATE
  compressed_image_io.cpp
  compression_options.cpp
  image_decoder.cpp
  image_encoder.cpp
  image_io.cpp
)

target_sources(
  pa171_compress
  PRIVATE
  compress.cpp
)

target_sources(
  pa171_decompress
  PRIVATE
  decompress.cpp
)
