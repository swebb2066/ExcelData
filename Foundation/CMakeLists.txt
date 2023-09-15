project(Foundation CXX)

set(SRC_FILES
  Environment.cpp
  EventTime.cpp
  Exception.cpp
  FileIterator.cpp
  Logger.cpp
  LogMessages.cpp
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
if(TARGET log4cxx)
  get_target_property(LOG4CXX_COMPILE_DEFINITIONS log4cxx INTERFACE_COMPILE_DEFINITIONS)
  get_target_property(LOG4CXX_INCLUDE_DIR log4cxx INTERFACE_INCLUDE_DIRECTORIES)
else()
  message(FATAL "log4cxx target not defined by find_package")
endif()
message("-- Found log4cxx ${LOG4CXX_INCLUDE_DIR}")
list(APPEND PUBLIC_COMPILE_DEFINITIONS ${LOG4CXX_COMPILE_DEFINITIONS})
list(APPEND PUBLIC_INCLUDE_DIRS ${LOG4CXX_INCLUDE_DIR})
list(APPEND INTERFACE_LIBRARIES log4cxx )

# YAML
find_package(yaml-cpp REQUIRED)
if(TARGET yaml-cpp)
  get_target_property(YAML_INCLUDE_DIR yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(YAML_COMPILE_DEFINITIONS yaml-cpp INTERFACE_COMPILE_DEFINITIONS)
  set(YAML_LIBRARY yaml-cpp )
else()
  set(YAML_INCLUDE_DIR ${yaml-cpp_INCLUDE})
  if(NOT YAML_INCLUDE_DIR)
    message(FATAL_ERROR "yaml-cpp headers not found")
  endif()
  set(YAML_LIBRARY ${yaml-cpp_LIBRARY} )
  if(NOT YAML_LIBRARY)
    message(FATAL_ERROR "yaml-cpp library not found")
  endif()
endif()
message("-- Found yaml-cpp ${YAML_INCLUDE_DIR}")
list(APPEND PUBLIC_COMPILE_DEFINITIONS ${YAML_COMPILE_DEFINITIONS})
list(APPEND PUBLIC_INCLUDE_DIRS ${YAML_INCLUDE_DIR})
list(APPEND INTERFACE_LIBRARIES ${YAML_LIBRARY})

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

