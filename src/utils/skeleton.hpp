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

// C
#include <signal.h>
#include <stdint.h>
#include <syslog.h>

// C++
#include <iostream>
#include <string>
using namespace std;

// SGCTP
#include "sgctp/sgctp.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------

/// Current file name, without path
#define __SGCTP_FILE__             \
  ( strrchr( __FILE__, '/' )       \
    ? strrchr( __FILE__ , '/') + 1 \
    : __FILE__ )

/// Logging macro
#ifdef __SGCTP_DEBUG__

#define SGCTP_LOG                                                       \
  std::clog << ( ( CSgctpUtilLog::SGCTP_LOG_LEVEL == LOG_DEBUG ) \
                 ? string("[")                                          \
                 + __SGCTP_FILE__                                       \
                 + ":"                                                  \
                 + to_string(__LINE__)                                  \
                 + "]"                                                 \
                 : "" )

#else // SGCTP_DEBUG

#define SGCTP_LOG std::clog

#endif // NOT SGCTP_DEBUG

#define SGCTP_PARSE_ARGS( __parseArgs__ ) \
  if( int __i__ = __parseArgs__ )         \
  {                                       \
   if( __i__ < 0 )                        \
     return __i__;                        \
   else                                   \
     continue;                            \
  }


//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------

// Classes pre-definitions; see further below for actual definitions
class CSgctpUtilLog;
class CSgctpUtilSyslog;

/// SGCTP utility skeleton
class CSgctpUtilSkeleton
{

  //----------------------------------------------------------------------
  // STATIC / CONSTANTS
  //----------------------------------------------------------------------

  //
  // Logging
  //

public:
  /// Log level values
  enum ELogLevel {
    SGCTP_DEBUG = LOG_DEBUG,
    SGCTP_INFO = LOG_INFO,
    SGCTP_NOTICE = LOG_NOTICE,
    SGCTP_WARNING = LOG_WARNING,
    SGCTP_ERROR = LOG_ERR,
    SGCTP_CRITICAL = LOG_CRIT,
    SGCTP_ALERT = LOG_ALERT,
    SGCTP_EMERGENCY = LOG_EMERG
  };

  //
  // Runtime
  //

protected:
  /// Interruption flag
  static volatile sig_atomic_t SGCTP_INTERRUPTED;
  /// Internal password salt size
  const static int PASSWORD_SALT_SIZE = 16; // Current max. password salt size: AES128 <-> 128 bits

  //
  // Geographical
  //

  // NOTE: Constants shamelessly copied from GPSD
protected:
  /// Mathematical PI constant
  const static double SGCTP_PI;
  /// Radian-to-Degree conversion factor
  const static double SGCTP_RAD2DEG;
  /// Degree-to-Radian conversion factor
  const static double SGCTP_DEG2RAD;
  /// WGS84 equatorial radius
  const static double SGCTP_WGS84A;
  /// WGS84 polar radius
  const static double SGCTP_WGS84B;

protected:
  /// Returns the loxodrome (rhumb-line) distance between the given coordinates
  static double distanceRL( double _fdLat1, double _fdLon1,
                            double _fdLat2, double _fdLon2 );
  /// Returns the loxodrome (rhumb-line) azimuth between the given coordinates
  static double azimuthRL( double _fdLat1, double _fdLon1,
                           double _fdLat2, double _fdLon2 );

  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

protected:
  // Application ID
  string sApplicationID;
  // Arguments count
  int iArgC;
  // Arguments values
  char **ppcArgV;

  /// Default logger object pointer
  streambuf *poStreambufStdClog;
  /// Standard logger object pointer
  CSgctpUtilLog *poSgctpUtilLog;
  /// Syslog logger object pointer
  CSgctpUtilSyslog *poSgctpUtilSyslog;

  /// Principals database path
  string sPrincipalsPath;
  /// Input payload type
  uint8_t ui8tPayloadType_in;
  /// Output payload type
  uint8_t ui8tPayloadType_out;
  /// Extended content usage
  bool bExtendedContent;
  /// Input transmission (generic) object pointer
  CTransmit *poTransmit_in;
  /// Output transmission (generic) object pointer
  CTransmit *poTransmit_out;
  /// Input password salt
  unsigned char pucPasswordSalt_in[PASSWORD_SALT_SIZE];
  /// Output password salt
  unsigned char pucPasswordSalt_out[PASSWORD_SALT_SIZE];

  /// Daemonization flag
  bool bDaemon;
  /// Daemon PID storage path
  string sDaemonPidPath;
  /// Daemon UID
  int iDaemonUid;
  /// Daemon GID
  int iDaemonGid;

  /// Signal set
  sigset_t tSigsetT;


  //----------------------------------------------------------------------
  // CONSTRUCTORS / DESTRUCTOR
  //----------------------------------------------------------------------

protected:
  CSgctpUtilSkeleton( const string &_rsApplicationID, int _iArgC, char *_ppcArgV[] );

public:
  virtual ~CSgctpUtilSkeleton();


  //----------------------------------------------------------------------
  // METHODS
  //----------------------------------------------------------------------

  //
  // Help
  //

protected:
  /// Displays the utilities help
  virtual void displayHelp() = 0;
  /// Displays the usage header
  void displayUsageHeader();
  /// Displays the sysnopsis header
  void displaySynopsisHeader();
  /// Displays the options header
  void displayOptionsHeader();
  /// Displays the principal option(s)
  void displayOptionPrincipal( bool _bInput, bool _bOutput );
  /// Displays the payload option(s)
  void displayOptionPayload( bool _bInput, bool _bOutput );
  /// Displays the password option(s)
  void displayOptionPassword( bool _bInput, bool _bOutput );
  /// Displays the password salt option(s)
  void displayOptionPasswordSalt( bool _bInput, bool _bOutput );
  /// Displays the extended content option
  void displayOptionExtendedContent();
  /// Displays the daemon options
  void displayOptionDaemon();

  //
  // Arguments
  //

protected:
  /// Parses the utilities arguments
  virtual int parseArgs() = 0;
  /// Parses the help option
  int parseArgsHelp( int *_piArgI );
  /// Parses the principal option(s)
  int parseArgsPrincipal( int *_piArgI, bool _bInput, bool _bOutput );
  /// Parses the payload option(s)
  int parseArgsPayload( int *_piArgI, bool _bInput, bool _bOutput );
  /// Parses the password option(s)
  int parseArgsPassword( int *_piArgI, bool _bInput, bool _bOutput );
  /// Parses the password salt option(s)
  int parseArgsPasswordSalt( int *_piArgI, bool _bInput, bool _bOutput );
  /// Parses the extended content option
  int parseArgsExtendedContent( int *_piArgI );
  /// Parses the daemon options
  int parseArgsDaemon( int *_piArgI );
  /// Displays extra argument(s) error
  void displayErrorExtraArgument( const string &_rsArg );
  /// Displays invalid option error
  void displayErrorInvalidOption( const string &_rsArg );
  /// Displays missing option argument error
  void displayErrorMissingArgument( const string &_rsArg );
  /// Displays invalid UID/GID error
  void displayErrorInvalidUidGid( const string &_rsArg );

  //
  // SGCTP
  //

protected:
  int principalLookup( bool _bInput, bool _bOutput );
  int transmitInit( bool _bInput, bool _bOutput );

  //
  // Runtime
  //

protected:
  /// Starts daemon (fork into background)
  int daemonStart();
  /// Ends daemon (clean-up)
  void daemonEnd();

  /// Enables signals catching
  void sigCatch( void (*_fHandler)(int) );
  /// Blocks signals
  void sigBlock();
  /// Unblocks signals
  void sigUnblock();

public:
  /// Executes the utility
  virtual int exec() = 0;

};


//----------------------------------------------------------------------
// CLASSES: Logging
//----------------------------------------------------------------------
// NOTE: Shamelessly inspired by http://stackoverflow.com/questions/2638654/redirect-c-stdclog-to-syslog-on-unix

/// Log-level-aware stream operator
ostream& operator<<( ostream &_rOStream,
                     CSgctpUtilSkeleton::ELogLevel _eLogLevel );

/// SGCTP generic logging stream
class CSgctpUtilLog: public basic_streambuf<char,char_traits<char>>
{
public:
  friend ostream& operator<<( ostream &_rOStream,
                              CSgctpUtilSkeleton::ELogLevel _eLogLevel );

  //----------------------------------------------------------------------
  // STATIC / CONSTANTS
  //----------------------------------------------------------------------

public:
  /// Log level
  static int SGCTP_LOG_LEVEL;

protected:
  /// Prefix string with log level
  static void prefixLogLevel( string *_psString,
                              int _iLogLevel );

  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

protected:
  string sApplicationID;
  int iLogLevel;
  string sLineBuffer;

  //----------------------------------------------------------------------
  // CONSTRUCTORS / DESTRUCTOR
  //----------------------------------------------------------------------

public:
  explicit CSgctpUtilLog( const string &_rsApplicationID );

  //----------------------------------------------------------------------
  // METHODS: basic_streambuf (implement/override)
  //----------------------------------------------------------------------

protected:
  virtual int overflow( int _iCharacter );
  virtual int sync();

};

/// SGCTP syslog logging stream
class CSgctpUtilSyslog: public CSgctpUtilLog
{

  //----------------------------------------------------------------------
  // CONSTRUCTORS / DESTRUCTOR
  //----------------------------------------------------------------------

public:
  explicit CSgctpUtilSyslog( const string &_rsApplicationID,
                             int _iFacility = LOG_DAEMON );

  //----------------------------------------------------------------------
  // METHODS: basic_streambuf (implement/override)
  //----------------------------------------------------------------------

protected:
  virtual int sync();

};
