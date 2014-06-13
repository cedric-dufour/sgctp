# REFERENCES:
# http://www.cmake.org/Wiki/CMake_Useful_Variables

# Project
project( SGCTP )
cmake_minimum_required( VERSION 2.8 )

set( CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/modules
)

# Version
set( APPLICATION_NAME ${PROJECT_NAME} )
set( APPLICATION_VERSION_MAJOR "1" )  # MUST be an integer
set( APPLICATION_VERSION_MINOR "0" )  # MUST be an integer
set( APPLICATION_VERSION_BUILD "20140613rc1" )  # MAY be a string
set( APPLICATION_VERSION_STRING "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_BUILD}" )
add_definitions(
  -DVER_MAJOR=${APPLICATION_VERSION_MAJOR}
  -DVER_MINOR=${APPLICATION_VERSION_MINOR}
  -DVER_BUILD=${APPLICATION_VERSION_BUILD}
  -DVER_STRING="${APPLICATION_VERSION_STRING}"
)
configure_file(
  ${CMAKE_SOURCE_DIR}/VERSION.in
  ${CMAKE_BINARY_DIR}/VERSION
  @ONLY
  )

# Add definitions
include( DefineCompilerFlags )
include( DefineInstallationPaths )
add_definitions(
  -DCONFDIR_STRING="/.config/sgctp" -Wall
)

# Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
IF(DEFINED ENV{SGCTP_USE_OPENSSL})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SGCTP_USE_OPENSSL__")
ENDIF()
# cmake -DCMAKE_BUILD_TYPE=Debug ...
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -D__SGCTP_DEBUG__")
# cmake -DCMAKE_BUILD_TYPE=Release ...
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O2")
#set(CMAKE_VERBOSE_MAKEFILE "ON")

# Sub-directories
add_subdirectory( src )

# Uninstall
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
)
add_custom_target(
  uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)
