project(jpeg_qoi_png_test)
cmake_minimum_required (VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)

set(CODER_SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
${CMAKE_CURRENT_SOURCE_DIR}/jpeg_custom_coder/buffer.c
${CMAKE_CURRENT_SOURCE_DIR}/jpeg_custom_coder/dct.c
${CMAKE_CURRENT_SOURCE_DIR}/jpeg_custom_coder/jpeg.c
${CMAKE_CURRENT_SOURCE_DIR}/jpeg_custom_coder/ppmm.c
  CACHE INTERNAL "")

message(">>>>>>>>>>>>>>>>>>>>>")
message(${CODER_SOURCES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(coder PRIVATE
    -O3
)
endif()

add_executable(coder ${CODER_SOURCES})