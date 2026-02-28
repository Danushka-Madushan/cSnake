cmake_minimum_required(VERSION 3.10.0)
project(cSnake VERSION 1.0.0 LANGUAGES C CXX)

set(SRC_FILES
  csnake.cpp
)

# Include Windows-only files
if(WIN32)
  list(APPEND SRC_FILES resource.rc)
endif()

add_executable(cSnake ${SRC_FILES})

target_include_directories(cSnake
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
