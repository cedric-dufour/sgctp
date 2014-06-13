// INDENTING (emacs/vi): -*- mode:c++; tab-width:2; c-basic-offset:2; intent-tabs-mode:nil; -*- ex: set tabstop=2 expandtab:

/*
 * Simple Geolocalization and Course Transmission Protocol (SGCTP)
 * Copyright (C) 2014 Cedric Dufour <http://cedric.dufour.name>
 *
 * The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 * free software:
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, Version 3.
 *
 * The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 */

#ifndef SGCTP_CPARAMETERS_HPP
#define SGCTP_CPARAMETERS_HPP

// C
#include <stdint.h>
#include <sys/socket.h>

// SGCTP
#include "sgctp/principal.hpp"


// SGCTP namespace
namespace SGCTP
{

  /// SGCTP transmission/payload parameters
  /**
   * This class encapsulates the parameters required by SGCTP transmission
   * and payload objects.
   */
  class CParameters
  {

    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  protected:
    /// Network socket address (pointer to existing variable)
    struct sockaddr *ptSockaddr;
    /// Network socket address length (pointer to existing variable)
    socklen_t *ptSocklenT;

    /// Principals (database) path (pointer to existing variable)
    const char *pcPrincipalsPath;
    /// Principal
    CPrincipal oPrincipal;

    /// Cryptographic password salt/nonce (pointer to existing variable)
    const unsigned char *pucPasswordSalt;

    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CParameters()
    {
      reset();
    };
    ~CParameters()
    {};


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

    //
    // GETTERS
    //

  public:
    /// Return the principal (pointer)
    CPrincipal* usePrincipal() {
      return &oPrincipal;
    };

    //
    // SETTERS
    //

  public:
    /// Reset (undefines) all data
    void reset()
    {
      ptSockaddr = NULL;
      ptSocklenT = NULL;
      pcPrincipalsPath = NULL;
      oPrincipal.reset();
      pucPasswordSalt = NULL;
    };

    /// Set the network socket address (pointers to existing variables)
    /**
     *  @param[in] _ptSockaddr Socket address object (pointer to existing variable)
     *  @param[in] _ptSocklenT Socket address object length (pointer to existing variable)
     */
    void setSocketAddress( struct sockaddr *_ptSockaddr,
                           socklen_t *_ptSocklenT )
    {
      ptSockaddr = _ptSockaddr;
      ptSocklenT = _ptSocklenT;
    };


    /// Set the principals (database) path (pointer to existing variable)
    /**
     *  @param[in] _pcPrincipalsPath Principals (database) path (pointer to existing variable)
     */
    void setPrincipalsPath( const char *_pcPrincipalsPath )
    {
      pcPrincipalsPath = _pcPrincipalsPath;
    };

    /// Set the cryptographic password salt/nonce (pointer to existing variable)
    /**
     *  @param[in] _pucPasswordSalt Cryptographic password salt/nonce (pointer to existing variable)
     */
    void setPasswordSalt( const unsigned char *_pucPasswordSalt )
    {
      pucPasswordSalt = _pucPasswordSalt;
    };

  };

}

#endif // SGCTP_CPARAMETERS_HPP
