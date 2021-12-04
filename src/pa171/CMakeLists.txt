add_subdirectory(coding)
add_subdirectory(quantization)
add_subdirectory(transform)
add_subdirectory(utils)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  image_decoder.cpp
  image_encoder.cpp
  image_io.cpp
  main.cpp
)
