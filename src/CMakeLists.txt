# Sub-directories
# ... library
add_subdirectory( lib )
IF(CMAKE_BUILD_TYPE MATCHES Debug)
add_subdirectory( debug )
ENDIF()
# ... utilities
add_subdirectory( 3rd-party )
add_subdirectory( utils )

# Documentation
find_package( Doxygen )
if( DOXYGEN_FOUND )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/doc/lib/doxygen.conf.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.lib.conf @ONLY)
  file( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/lib )
  add_custom_target( doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.lib.conf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc/lib
    COMMENT "Generating documentation with Doxygen" VERBATIM
    )
  install(
    DIRECTORY
      ${CMAKE_BINARY_DIR}/doc/lib
    DESTINATION
      ${DATA_INSTALL_PREFIX}/doc/sgctp
  )
endif( DOXYGEN_FOUND )
install(
  DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/doc/utils
  DESTINATION
    ${DATA_INSTALL_PREFIX}/doc/sgctp
)
