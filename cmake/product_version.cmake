set(Product_VERSION_MAJOR 0)
set(Product_VERSION_MINOR 0)
set(Product_VERSION_PATCH 0)
set(BUILD_NUMBER 1)
set(SHORT_COMMIT_HASH "")
set(Product_VERSION ${Product_VERSION_MAJOR}.${Product_VERSION_MINOR}.${Product_VERSION_PATCH})
configure_file(${CMAKE_CURRENT_LIST_DIR}/product_version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/ProductVersion/product_version.cpp @ONLY)
configure_file(${CMAKE_CURRENT_LIST_DIR}/product_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/ProductVersion/product_version.h @ONLY)

# Any cpp file that wants to know the version number will need to link to this library
add_library(product_version STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/ProductVersion/product_version.cpp
)
target_include_directories(product_version
  PUBLIC
   ${EyeInHand_INCLUDE_DIR}
  INTERFACE
   ${CMAKE_CURRENT_BINARY_DIR}/ProductVersion # Make product_version.h available
)
