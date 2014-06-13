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

#ifndef SGCTP_CPRINCIPAL_HPP
#define SGCTP_CPRINCIPAL_HPP

// C
#include <stdint.h>

// C++
#include <set>
using namespace std;

// SGCTP
#define SGCTP_MAX_PASSWORD_SIZE 256
#define SGCTP_MAX_PASSWORD_LENGTH 255


// SGCTP namespace
namespace SGCTP
{

  /// SGCTP principal
  /**
   * This class gathers the parameters and credentials specific to
   * a SGCTP principal (client or agent).
   */
  class CPrincipal
  {

    //----------------------------------------------------------------------
    // CONSTANTS / STATIC
    //----------------------------------------------------------------------

  public:
    /// Maximum password length
    static const uint16_t MAX_PASSWORD_LENGTH = SGCTP_MAX_PASSWORD_LENGTH;


    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  private:
    /// ID
    uint64_t ui64tID;
    /// Password (max. 255 characters)
    char pcPassword[SGCTP_MAX_PASSWORD_SIZE];
    /// Allowed payload types
    set<uint8_t> ui8tPayloadType_set;

    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CPrincipal()
    {
      reset();
    };
    ~CPrincipal()
    {
      reset(); // clean sensitive data
    };


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

    //
    // SETTERS
    //

  public:
    /// Resets all parameters
    void reset();
    /// Erases the principal password (and nullifies memory)
    void erasePassword();

    /// Sets the principal ID
    void setID( uint64_t _ui64tID )
    {
      ui64tID = _ui64tID;
    };
    /// Sets the principal password
    void setPassword( const char *_pcPassword );
    /// Adds an allowed payload type
    void addPayloadType( uint8_t _ui8tPayloadType );

    //
    // GETTERS
    //

  public:
    /// Returns the principal ID
    uint64_t getID() const
    {
      return ui64tID;
    };
    /// Returns the principal password
    const char* getPassword() const
    {
      return pcPassword;
    };
    /// Returns whether payload types have been allowed
    bool hasPayloadTypes() const;
    /// Returns the allowed payload types
    bool hasPayloadType( uint8_t _ui8tPayloadType ) const;

    //
    // OTHER
    //

  public:
    /// Retrieves the principal paremeters from the given file
    int read( const char *_pcPrincipalsPath,
              uint64_t _ui64tID );

  };

}

#endif // SGCTP_CPRINCIPAL_HPP
