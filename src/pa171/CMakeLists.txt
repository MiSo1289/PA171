add_subdirectory(coding)
add_subdirectory(transform)
add_subdirectory(utils)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  image_decoder.cpp
  #image_compressor_lossless.cpp
  image_compressor_lossy.cpp
  image_io.cpp
  main.cpp
)
