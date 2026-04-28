project(Foundation CXX)

set(SRC_FILES
  Environment.cpp
  EventTime.cpp
  Exception.cpp
  FileIterator.cpp
  Logger.cpp
  LogMessages.cpp
  Memory.cpp
  Yaml.cpp
  YamlDocument.cpp
  SafeArrayWrapper.cpp
  WindowsEnvironment.cpp
)

set(PUBLIC_INCLUDE_DIRS)
set(PRIVATE_INCLUDE_DIRS)

set(PUBLIC_COMPILE_DEFINITIONS)
set(PRIVATE_COMPILE_DEFINITIONS)
set(SYSTEM_COMPILE_DEFINITIONS)

set(INTERFACE_LIBRARIES)
set(PRIVATE_LIBRARIES)
set(SYSTEM_LIBRARIES)

# product_version
if(TARGET product_version)
  get_target_property(PRODUCT_VERSION_INCLUDE_DIR product_version INTERFACE_INCLUDE_DIRECTORIES)
else()
  message(FATAL "product_version target not available")
endif()
list(APPEND INTERFACE_LIBRARIES product_version)
list(APPEND PUBLIC_INCLUDE_DIRS ${PRODUCT_VERSION_INCLUDE_DIR})

# Boost
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#set(Boost_DEBUG ON)
find_package(Boost COMPONENTS unit_test_framework program_options REQUIRED )
if(WIN32)
  get_target_property(Boost_COMPILE_DEFINITIONS
    Boost::disable_autolinking INTERFACE_COMPILE_DEFINITIONS
  )
  list(APPEND PUBLIC_COMPILE_DEFINITIONS ${Boost_COMPILE_DEFINITIONS})
endif()
message("-- Found Boost ${Boost_INCLUDE_DIR}")
list(APPEND PUBLIC_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
list(APPEND INTERFACE_LIBRARIES ${Boost_LIBRARIES} )

if(CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPER_BUILD_TYPE)
else()
  set(UPPER_BUILD_TYPE RELEASE)
endif(CMAKE_BUILD_TYPE)

# log4cxx
find_package(log4cxx REQUIRED)
message("-- Found log4cxx ${log4cxx_INCLUDE_DIR}")
list(APPEND PUBLIC_COMPILE_DEFINITIONS ${log4cxx_DEFINITIONS} )
list(APPEND PUBLIC_INCLUDE_DIRS ${log4cxx_INCLUDE_DIR} )
list(APPEND INTERFACE_LIBRARIES ${log4cxx_LIBRARIES} )

# YAML
find_package(yaml-cpp REQUIRED)
message("-- Found yaml-cpp ${yaml-cpp_INCLUDE_DIR}")
list(APPEND PUBLIC_COMPILE_DEFINITIONS ${yaml-cpp_DEFINITIONS})
list(APPEND PUBLIC_INCLUDE_DIRS ${yaml-cpp_INCLUDE_DIR})
list(APPEND INTERFACE_LIBRARIES ${yaml-cpp_LIBRARIES})

if(WIN32)
  list(APPEND SRC_FILES
     WindowsEnvironment.cpp
  )
  set(SYSTEM_COMPILE_DEFINITIONS NOMINMAX)
  set(SYSTEM_LIBRARIES Dbghelp Winmm ) # For CallStack, ProcessControl
else()
  list(APPEND SRC_FILES
     GenericIniEnvironment.cpp
  )
endif()

add_library(Foundation STATIC
  ${SRC_FILES}
)
target_compile_definitions(Foundation
  PUBLIC
    ${PUBLIC_COMPILE_DEFINITIONS}
    ${SYSTEM_COMPILE_DEFINITIONS}
  PRIVATE
    ${PRIVATE_COMPILE_DEFINITIONS}
)
target_include_directories(Foundation
  PUBLIC
    ${PUBLIC_INCLUDE_DIRS}
  PRIVATE
   ${PRIVATE_INCLUDE_DIRS}
)
target_link_libraries(Foundation
  INTERFACE
    ${INTERFACE_LIBRARIES}
  PRIVATE
    ${PRIVATE_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)

