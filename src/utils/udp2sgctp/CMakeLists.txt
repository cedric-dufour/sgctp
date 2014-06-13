# Output
set( EXECNAME udp2sgctp )

# Include files
include_directories(
  .
  ../../lib
)

# Source files (*.cpp)
set( MY_CPPS
  main.cpp
)

# Executable
set( MY_DEPS
  ${MY_CPPS}
)
set( EXECUTABLE_OUTPUT_PATH
  ${CMAKE_BINARY_DIR}/bin
)
add_executable( ${EXECNAME}
  $<TARGET_OBJECTS:skeleton>
  ${MY_DEPS}
)

# Libraries
target_link_libraries( ${EXECNAME}
  sgctp
)

# Install
install(
  TARGETS
    ${EXECNAME}
  DESTINATION
    ${BIN_INSTALL_DIR}
)
