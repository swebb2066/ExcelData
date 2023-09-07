project(Foundation CXX)

set(SRC_FILES
  Environment.cpp
  EventTime.cpp
  Exception.cpp
  FileIterator.cpp
  Logger.cpp
  LogMessages.cpp
  ReversePolishStack.cpp
  Yaml.cpp
  YamlDocument.cpp
)

set(PUBLIC_COMPILE_DEFINITIONS)
set(PUBLIC_INCLUDE_DIRS)
set(PRIVATE_INCLUDE_DIRS)
set(PRIVATE_COMPILE_DEFINITIONS)
set(PRIVATE_LIBRARIES )
set(SYSTEM_COMPILE_DEFINITIONS)
set(SYSTEM_LIBRARIES)

# log4cxx
if(TARGET product_version)
  get_target_property(PRODUCT_VERSION_INCLUDE_DIR product_version INTERFACE_INCLUDE_DIRECTORIES)
else()
  message(FATAL "product_version target not available")
endif()
list(APPEND PRIVATE_LIBRARIES product_version)
list(APPEND PUBLIC_INCLUDE_DIRS ${PRODUCT_VERSION_INCLUDE_DIR})

# Boost
set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_LIBS  ON)
set(Boost_USE_STATIC_RUNTIME OFF)
#set(Boost_DEBUG ON)
find_package(Boost COMPONENTS system filesystem unit_test_framework program_options REQUIRED )
if(WIN32)
  get_target_property(Boost_COMPILE_DEFINITIONS
    Boost::disable_autolinking INTERFACE_COMPILE_DEFINITIONS
  )
endif()
message("-- Found Boost ${Boost_INCLUDE_DIR}")

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
  get_target_property(LOG4CXX_DLL_PATH log4cxx "IMPORTED_LOCATION_${UPPER_BUILD_TYPE}")
  get_filename_component(LOG4CXX_FILE_DIR "${LOG4CXX_DLL_PATH}" PATH)
else()
  message(FATAL "log4cxx target not defined by find_package")
endif()
#message("LOG4CXX_INCLUDE_DIR=${LOG4CXX_INCLUDE_DIR}")
list(APPEND PUBLIC_COMPILE_DEFINITIONS ${LOG4CXX_COMPILE_DEFINITIONS})
list(APPEND PUBLIC_INCLUDE_DIRS ${LOG4CXX_INCLUDE_DIR})
set(LOG4CXX_LIBRARY log4cxx )
set(LOG4CXX_INCLUDE_LIST ${LOG4CXX_INCLUDE_DIR})
list(GET LOG4CXX_INCLUDE_LIST 0 LOG4CXX_DIR_0)
if(EXISTS ${LOG4CXX_DIR_0})
  get_filename_component(LOG4CXX_DIR ${LOG4CXX_DIR_0} PATH)
  set(LOG4CXX_FILE_DIR "${LOG4CXX_DIR}/bin")
  set(PATH_FOR_TESTS ${LOG4CXX_FILE_DIR})
else()
  message(WARNING "${LOG4CXX_DIR_0} does not exist")
endif()
message("-- Found log4cxx ${LOG4CXX_FILE_DIR}")

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
list(APPEND PRIVATE_COMPILE_DEFINITIONS ${YAML_COMPILE_DEFINITIONS})
list(APPEND PUBLIC_INCLUDE_DIRS ${YAML_INCLUDE_DIR})
set(YAML_INCLUDE_LIST ${YAML_INCLUDE_DIR})
list(GET YAML_INCLUDE_LIST 0 YAML_DIR_0)
if(EXISTS ${YAML_DIR_0})
  get_filename_component(YAML_DIR "${YAML_INCLUDE_DIR}" PATH)
else()
  message(WARNING "${YAML_INCLUDE_DIR} does not exist")
endif()
message("-- Found yaml-cpp ${YAML_DIR}")

if(WIN32)
  list(APPEND SRC_FILES
     WindowsEnvironment.cpp
  )
  list(APPEND TEST_FILES
    WindowsEnvironmentTests.cpp
  )
  set(SYSTEM_COMPILE_DEFINITIONS NOMINMAX)
  set(SYSTEM_LIBRARIES Dbghelp Winmm ) # For CallStack, ProcessControl
else()
  list(APPEND SRC_FILES
     GenericIniEnvironment.cpp
  )
  list(APPEND TEST_FILES
    GenericIniEnvironmentTests.cpp
  )
endif()

add_library(Foundation STATIC
  ${SRC_FILES}
)
target_compile_definitions(Foundation
  PUBLIC
    ${Boost_COMPILE_DEFINITIONS}
    ${LOG4CXX_COMPILE_DEFINITIONS}
    ${PUBLIC_COMPILE_DEFINITIONS}
    ${SYSTEM_COMPILE_DEFINITIONS}
  PRIVATE
    ${PRIVATE_COMPILE_DEFINITIONS}
)
target_include_directories(Foundation
  PUBLIC
    ${EyeInHand_INCLUDE_DIR}
    ${PUBLIC_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
  PRIVATE
   ${PRIVATE_INCLUDE_DIRS}
)
target_link_libraries(Foundation
  INTERFACE
    ${Boost_LIBRARIES}
    ${LOG4CXX_LIBRARY}
    ${YAML_LIBRARY}
    ${PRIVATE_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)

