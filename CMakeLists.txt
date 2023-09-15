cmake_minimum_required(VERSION 3.17)
PROJECT(excel2csv CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
# Keep version info in one central spot, accessible by all other code as a library
include(product_version)

include_directories(${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(Foundation)

add_executable(excel2csv
  Excel2csv.cpp
  CellRowIterator.cpp
  Excel.cpp
)
target_include_directories(excel2csv
  PRIVATE
    $<TARGET_PROPERTY:Foundation,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(excel2csv
  PRIVATE
    Foundation
    comsuppw
    ole32
    oleaut32
    uuid
)

