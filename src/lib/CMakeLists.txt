# Output
set( LIBNAME sgctp )

# External resources
IF(DEFINED ENV{__SGCTP_USE_OPENSSL__})
# ... OpenSSL
find_package( OpenSSL REQUIRED )
set( CRYPTO_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS} )
set( CRYPTO_LIBRARIES ${OPENSSL_LIBRARIES} )
ELSE()
# ... LibGcrypt
find_package( Libgcrypt REQUIRED )
set( CRYPTO_INCLUDE_DIRS ${LIBGCRYPT_INCLUDE_DIRS} )
set( CRYPTO_LIBRARIES ${LIBGCRYPT_LIBRARIES} )
ENDIF()

# Include files
include_directories(
  .
  ${CRYPTO_INCLUDE_DIRS}
)

# Source files (*.cpp)
set( MY_CPPS
  data.cpp
  payload.cpp
  payload_aes128.cpp
  principal.cpp
  transmit.cpp
  transmit_file.cpp
  transmit_udp.cpp
  transmit_tcp.cpp
)

# Version
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sgctp/version.hpp.in
  ${CMAKE_BINARY_DIR}/sgctp/version.hpp
  @ONLY
  )
include_directories( ${CMAKE_BINARY_DIR} )

# Library
set( MY_DEPS ${MY_CPPS} )
set( LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib )
add_library( ${LIBNAME}_object OBJECT ${MY_DEPS} )
add_library( ${LIBNAME} SHARED $<TARGET_OBJECTS:${LIBNAME}_object> )
add_library( ${LIBNAME}_static STATIC $<TARGET_OBJECTS:${LIBNAME}_object> )
set_target_properties( ${LIBNAME} PROPERTIES VERSION ${APPLICATION_VERSION_STRING} SOVERSION ${APPLICATION_VERSION_MAJOR} )

# Libraries
target_link_libraries( ${LIBNAME}
  rt
  ${CRYPTO_LIBRARIES}
)

# Install
install(
  TARGETS
    ${LIBNAME}
  DESTINATION
    ${LIB_INSTALL_DIR}
)
