project(coder)

cmake_minimum_required (VERSION 3.0)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif ()

set(CODER_SOURCES
  buffer.c
  dct.c
  jpeg.c
  ppmm.c
  timer.c
  CACHE INTERNAL "")

add_executable(coder main.c ${CODER_SOURCES})
target_link_libraries(coder m)