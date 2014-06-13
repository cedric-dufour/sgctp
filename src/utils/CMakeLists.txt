# Output
set( LIBNAME skeleton )

# Include files
include_directories(
  ${CMAKE_BINARY_DIR}
  .
  ../lib
)

# Source files (*.cpp)
set( MY_CPPS
  skeleton.cpp
)

# Library
set( MY_DEPS ${MY_CPPS} )
add_library( ${LIBNAME} OBJECT ${MY_DEPS} )

# Sub-directories
add_subdirectory( sgctp2dsv )
add_subdirectory( sgctp2xml )
add_subdirectory( sgctp2gpx )
add_subdirectory( sgctp2udp )
add_subdirectory( sgctp2tcp )
add_subdirectory( gps2sgctp )
add_subdirectory( ais2sgctp )
add_subdirectory( sbs2sgctp )
add_subdirectory( flarm2sgctp )
add_subdirectory( udp2sgctp )
add_subdirectory( tcp2sgctp )
add_subdirectory( sgctpfilter )
add_subdirectory( sgctphub )
add_subdirectory( hub2sgctp )
add_subdirectory( sgctperror )
