# Output
set( LIBNAME coreutils )

# Include files
include_directories(
  .
)

# Source files (*.cpp)
set( MY_CPPS
  base64.c
)

# Library
set( MY_DEPS ${MY_CPPS} )
add_library( ${LIBNAME} OBJECT ${MY_DEPS} )
