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
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

// SGCTP
#include "skeleton.hpp"
#include "sgctp/sgctp.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// STATIC / CONSTANTS
//----------------------------------------------------------------------

volatile sig_atomic_t CSgctpUtilSkeleton::SGCTP_INTERRUPTED = 0;

const double CSgctpUtilSkeleton::SGCTP_PI = 3.1415926535897932384626433832795029;
const double CSgctpUtilSkeleton::SGCTP_RAD2DEG = 57.2957795130823208767981548141051703;
const double CSgctpUtilSkeleton::SGCTP_DEG2RAD = 0.0174532925199432957692369076848861271;
const double CSgctpUtilSkeleton::SGCTP_WGS84A = 6378137.0;
const double CSgctpUtilSkeleton::SGCTP_WGS84B = 6356752.3142;

double CSgctpUtilSkeleton::distanceRL( double _fdLat1, double _fdLon1, double _fdLat2, double _fdLon2 )
{
  // Formula shamelessly copied from http://www.movable-type.co.uk/scripts/latlong.html
  _fdLat1 *= SGCTP_DEG2RAD; _fdLon1 *= SGCTP_DEG2RAD;
  _fdLat2 *= SGCTP_DEG2RAD; _fdLon2 *= SGCTP_DEG2RAD;
  double __fdLatD = _fdLat2 - _fdLat1;
  double __fdLonD = _fdLon2 - _fdLon1;
  double __fdPhiD = log( tan( _fdLat2/2.0 + SGCTP_PI/4.0 ) / tan( _fdLat1/2.0 + SGCTP_PI/4.0 ) );
  double __fdQ = __fdPhiD != 0 ? __fdLatD/__fdPhiD : cos( _fdLat1 );
  if( fabs( __fdLonD ) > SGCTP_PI ) __fdLonD = __fdLonD > 0 ? __fdLonD - SGCTP_PI*2.0 : SGCTP_PI*2.0 + __fdLonD;
  double __fdDistance = ( SGCTP_WGS84A + SGCTP_WGS84B ) / 2.0 * sqrt( __fdLatD*__fdLatD + __fdLonD*__fdLonD * __fdQ*__fdQ );
  return __fdDistance;
}

double CSgctpUtilSkeleton::azimuthRL( double _fdLat1, double _fdLon1, double _fdLat2, double _fdLon2 )
{
  // Formula shamelessly copied from http://www.movable-type.co.uk/scripts/latlong.html
  _fdLat1 *= SGCTP_DEG2RAD; _fdLon1 *= SGCTP_DEG2RAD;
  _fdLat2 *= SGCTP_DEG2RAD; _fdLon2 *= SGCTP_DEG2RAD;
  double __fdLonD = _fdLon2 - _fdLon1;
  double __fdPhiD = log( tan( _fdLat2/2.0 + SGCTP_PI/4.0 ) / tan( _fdLat1/2.0 + SGCTP_PI/4.0 ) );
  if( fabs( __fdLonD ) > SGCTP_PI ) __fdLonD = __fdLonD > 0 ? __fdLonD - SGCTP_PI*2.0 : SGCTP_PI*2.0 + __fdLonD;
  double __fdAzimuth = atan2( __fdLonD, __fdPhiD );
  __fdAzimuth *= SGCTP_RAD2DEG;
  if( __fdAzimuth < 0 ) __fdAzimuth += 360;
  return __fdAzimuth;
}

int CSgctpUtilLog::SGCTP_LOG_LEVEL = CSgctpUtilSkeleton::SGCTP_INFO;

void CSgctpUtilLog::prefixLogLevel( string *_psString,
                                    int _iLogLevel )
{
  switch( _iLogLevel )
  {
  case LOG_DEBUG:
    _psString->insert( 0, "[DEBUG] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_INFO:
    _psString->insert( 0, "[INFO] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_NOTICE:
    _psString->insert( 0, "[NOTICE] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_WARNING:
    _psString->insert( 0, "[WARNING] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_ERROR:
    _psString->insert( 0, "[ERROR] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_CRITICAL:
    _psString->insert( 0, "[CRITICAL] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_ALERT:
    _psString->insert( 0, "[ALERT] " );
    break;
  case CSgctpUtilSkeleton::SGCTP_EMERGENCY:
    _psString->insert( 0, "[EMERGENCY] " );
    break;
  }
}

ostream& operator<<( ostream &_rOStream,
                              CSgctpUtilSkeleton::ELogLevel _eLogLevel )
{
  static_cast<CSgctpUtilLog*>(_rOStream.rdbuf())->iLogLevel = _eLogLevel;
  return _rOStream;
}


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtilSkeleton::CSgctpUtilSkeleton( const string &_rsApplicationID, int _iArgC, char *_ppcArgV[] )
  : sApplicationID( _rsApplicationID )
  , iArgC( _iArgC )
  , ppcArgV( _ppcArgV )
  , poStreambufStdClog( NULL )
  , poSgctpUtilLog( NULL )
  , poSgctpUtilSyslog( NULL )
  , sPrincipalsPath( "" )
  , ui8tPayloadType_in( 0 )
  , ui8tPayloadType_out( 0 )
  , bExtendedContent( false )
  , poTransmit_in( NULL )
  , poTransmit_out( NULL )
  , bDaemon( false )
  , sDaemonPidPath( "" )
  , iDaemonUid( 0 )
  , iDaemonGid( 0 )
{
  // Redirect standard log
  poStreambufStdClog = clog.rdbuf();
  poSgctpUtilLog = new CSgctpUtilLog( sApplicationID );
  clog.rdbuf( poSgctpUtilLog );

  // Zero password salt
  memset( pucPasswordSalt_in, 0, PASSWORD_SALT_SIZE );
  memset( pucPasswordSalt_out, 0, PASSWORD_SALT_SIZE );
}

CSgctpUtilSkeleton::~CSgctpUtilSkeleton()
{
  if( poStreambufStdClog )
    clog.rdbuf( poStreambufStdClog );
  if( poSgctpUtilLog )
    delete poSgctpUtilLog;
  if( poSgctpUtilSyslog )
    delete poSgctpUtilSyslog;
}

CSgctpUtilLog::CSgctpUtilLog( const string &_rsApplicationID )
  : sApplicationID( _rsApplicationID )
  , iLogLevel( LOG_DEBUG )
{
  sLineBuffer.erase();
}

CSgctpUtilSyslog::CSgctpUtilSyslog( const string &_rsApplicationID,
                                    int _iFacility )
  : CSgctpUtilLog( _rsApplicationID )
{
  openlog( sApplicationID.c_str(), LOG_PID, _iFacility );
}


//----------------------------------------------------------------------
// METHODS: basic_streambuf (implement/override)
//----------------------------------------------------------------------

int CSgctpUtilLog::overflow( int _iCharacter )
{
  if( _iCharacter != EOF )
    sLineBuffer += static_cast<char>( _iCharacter );
  else
    sync();
  return _iCharacter;
}

int CSgctpUtilLog::sync()
{
  if( !sLineBuffer.empty() )
  {
    if( iLogLevel <= SGCTP_LOG_LEVEL )
    {
      prefixLogLevel( &sLineBuffer, iLogLevel );
      sLineBuffer.insert( 0, "["+sApplicationID+"]" );
      cerr << sLineBuffer;
    }
    sLineBuffer.erase();
  }
  iLogLevel = LOG_DEBUG;
  return 0;
}

int CSgctpUtilSyslog::sync()
{
  if( !sLineBuffer.empty() )
  {
    if( iLogLevel <= SGCTP_LOG_LEVEL )
    {
      prefixLogLevel( &sLineBuffer, iLogLevel );
      syslog( iLogLevel, "%s", sLineBuffer.c_str() );
    }
    sLineBuffer.erase();
  }
  iLogLevel = LOG_DEBUG;
  return 0;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

//
// Help
//

void CSgctpUtilSkeleton::displayUsageHeader()
{
  cout << "USAGE:" << endl;
}

void CSgctpUtilSkeleton::displaySynopsisHeader()
{
  cout << endl << "SYNOPSIS:" << endl;
}

void CSgctpUtilSkeleton::displayOptionsHeader()
{
  cout << endl << "OPTIONS:" << endl;
  cout << "  --help" << endl;
  cout << "    Display help message" << endl;
  cout << "  --version" << endl;
  cout << "    Display version" << endl;
  cout << "  --verbose <level>" << endl;
  cout << "    Log level (default:6, LOG_INFO)" << endl;
}

void CSgctpUtilSkeleton::displayOptionPrincipal( bool _bInput, bool _bOutput )
{
  cout << "  -P, --principals <path>" << endl;
  cout << "    Principals (database) file path" << endl;
  if( _bInput )
    cout << "  -Ii, --principal-in <ID>" << endl;
  if( _bOutput )
    cout << "  -Io, --principal-out <ID>" << endl;
  cout << "    Payload principal ID" << endl;
}

void CSgctpUtilSkeleton::displayOptionPayload( bool _bInput, bool _bOutput )
{
  if( _bInput )
    cout << "  -Ti, --payload-in <type>" << endl;
  if( _bOutput )
    cout << "  -To, --payload-out <type>" << endl;
  cout << "    Payload type (default:0; 0=RAW, 1=AES128)" << endl;
}

void CSgctpUtilSkeleton::displayOptionPassword( bool _bInput, bool _bOutput )
{
  if( _bInput )
    cout << "  -Wi, --password-in <password>" << endl;
  if( _bOutput )
    cout << "  -Wo, --password-out <password>" << endl;
  cout << "    Payload password (ignored if a principals file is used)" << endl;
}

void CSgctpUtilSkeleton::displayOptionPasswordSalt( bool _bInput, bool _bOutput )
{
  if( _bInput )
    cout << "  -Si, --password-salt-in <password-salt>" << endl;
  if( _bOutput )
    cout << "  -So, --password-salt-out <password-salt>" << endl;
  cout << "    Payload password salt" << endl;
}

void CSgctpUtilSkeleton::displayOptionExtendedContent()
{
  cout << "  -X, --extended-content" << endl;
  cout << "    Add/include extended content in payload" << endl;
}

void CSgctpUtilSkeleton::displayOptionDaemon()
{
  cout << "  -D, --daemon" << endl;
  cout << "    Daemonize (fork into background)" << endl;
  cout << "  -Dp, --daemon-pid <path>" << endl;
  cout << "    Daemon PID file path" << endl;
  cout << "  -Du, --daemon-uid <uid>" << endl;
  cout << "    Daemon user ID (UID)" << endl;
  cout << "  -Dg, --daemon-gid <gid>" << endl;
  cout << "    Daemon group ID (GID)" << endl;
  cout << "  --syslog" << endl;
  cout << "    Log to syslog (instead of standard error)" << endl;
}

//
// Arguments
//

int CSgctpUtilSkeleton::parseArgsHelp( int *_piArgI )
{
  string __sArg = ppcArgV[*_piArgI];
  if( __sArg=="--help" )
  {
    displayHelp();
    return -EAGAIN;
  }
  else if( __sArg=="--version" )
  {
    cout << SGCTP_VERSION_STRING << endl;
    return -EAGAIN;
  }
  else if( __sArg=="--verbose" )
  {
    if( (*_piArgI)++ < iArgC )
      CSgctpUtilLog::SGCTP_LOG_LEVEL = atoi(ppcArgV[*_piArgI]);
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsPrincipal( int *_piArgI, bool _bInput, bool _bOutput )
{
  string __sArg = ppcArgV[*_piArgI];
  if( __sArg=="-P" || __sArg=="--principals" )
  {
    if( (*_piArgI)++ < iArgC )
      sPrincipalsPath = ppcArgV[*_piArgI];
    if( poTransmit_in )
      poTransmit_in->setPrincipalsPath( sPrincipalsPath.c_str() );
    if( poTransmit_out )
      poTransmit_out->setPrincipalsPath( sPrincipalsPath.c_str() );
    return 1;
  }
  else if( _bInput && ( __sArg=="-Ii" || __sArg=="--principal-in" ) )
  {
    if( !poTransmit_in )
      return -ENODATA;
    if( (*_piArgI)++ < iArgC )
      poTransmit_in->usePrincipal()->setID( (uint64_t)strtoull( ppcArgV[*_piArgI], NULL, 10 ) );
    return 1;
  }
  else if( _bOutput && ( __sArg=="-Io" || __sArg=="--principal-out" ) )
  {
    if( !poTransmit_out )
      return -ENODATA;
    if( (*_piArgI)++ < iArgC )
      poTransmit_out->usePrincipal()->setID( (uint64_t)strtoull( ppcArgV[*_piArgI], NULL, 10 ) );
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsPayload( int *_piArgI, bool _bInput, bool _bOutput )
{
  string __sArg = ppcArgV[*_piArgI];
  if( _bInput && ( __sArg=="-Ti" || __sArg=="--payload-in" ) )
  {
    if( (*_piArgI)++ < iArgC )
      ui8tPayloadType_in = (uint8_t)atoi( ppcArgV[*_piArgI] );
    return 1;
  }
  else if( _bOutput && ( __sArg=="-To" || __sArg=="--payload-out" ) )
  {
    if( (*_piArgI)++ < iArgC )
      ui8tPayloadType_out = (uint8_t)atoi( ppcArgV[*_piArgI] );
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsPassword( int *_piArgI, bool _bInput, bool _bOutput )
{
  string __sArg = ppcArgV[*_piArgI];
  if( _bInput && ( __sArg=="-Wi" || __sArg=="--password-in" ) )
  {
    if( !poTransmit_in )
      return -ENODATA;
    if( (*_piArgI)++ < iArgC )
      poTransmit_in->usePrincipal()->setPassword( ppcArgV[*_piArgI] );
    return 1;
  }
  else if( _bOutput && ( __sArg=="-Wo" || __sArg=="--password-out" ) )
  {
    if( !poTransmit_out )
      return -ENODATA;
    if( (*_piArgI)++ < iArgC )
      poTransmit_out->usePrincipal()->setPassword( ppcArgV[*_piArgI] );
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsPasswordSalt( int *_piArgI, bool _bInput, bool _bOutput )
{
  string __sArg = ppcArgV[*_piArgI];
  if( _bInput && ( __sArg=="-Si" || __sArg=="--password-salt-in" ) )
  {
    if( (*_piArgI)++ < iArgC )
    {
      memset( pucPasswordSalt_in, 0, PASSWORD_SALT_SIZE );
      int __iSize = strlen( ppcArgV[*_piArgI] );
      if( __iSize > PASSWORD_SALT_SIZE )
        __iSize = PASSWORD_SALT_SIZE;
      memcpy( pucPasswordSalt_in, ppcArgV[*_piArgI], __iSize );
    }
    return 1;
  }
  else if( _bOutput && ( __sArg=="-So" || __sArg=="--password-salt-out" ) )
  {
    if( (*_piArgI)++ < iArgC )
    {
      memset( pucPasswordSalt_out, 0, PASSWORD_SALT_SIZE );
      int __iSize = strlen( ppcArgV[*_piArgI] );
      if( __iSize > PASSWORD_SALT_SIZE )
        __iSize = PASSWORD_SALT_SIZE;
      memcpy( pucPasswordSalt_out, ppcArgV[*_piArgI], __iSize );
    }
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsExtendedContent( int *_piArgI )
{
  string __sArg = ppcArgV[*_piArgI];
  if( __sArg=="-X" || __sArg=="--extended-content" )
  {
    bExtendedContent = true;
    return 1;
  }
  return 0;
}

int CSgctpUtilSkeleton::parseArgsDaemon( int *_piArgI )
{
  string __sArg = ppcArgV[*_piArgI];
  if( __sArg=="-D" || __sArg=="--daemon" )
  {
    bDaemon = true;
    return 1;
  }
  else if( __sArg=="-Dp" || __sArg=="--daemon-pid" )
  {
    if( (*_piArgI)++ < iArgC )
      sDaemonPidPath = ppcArgV[*_piArgI];
    return 1;
  }
  else if( __sArg=="-Du" || __sArg=="--daemon-uid" )
  {
    if( (*_piArgI)++ < iArgC )
    {
      char *__pcEnd;
      int __iUid = (int)strtol( ppcArgV[*_piArgI], &__pcEnd, 10 );
      if( ppcArgV[*_piArgI][0] == '\0' || __pcEnd[0] == '\0' )
      {
        struct passwd *__pPasswd = getpwnam( ppcArgV[*_piArgI] );
        if( __pPasswd ) iDaemonUid = __pPasswd->pw_uid;
        else displayErrorInvalidUidGid( ppcArgV[*_piArgI] );
      }
      else
        iDaemonUid = __iUid;
    }
    return 1;
  }
  else if( __sArg=="-Dg" || __sArg=="--daemon-gid" )
  {
    if( (*_piArgI)++ < iArgC )
    {
      char *__pcEnd;
      int __iGid = (int)strtol( ppcArgV[*_piArgI], &__pcEnd, 10 );
      if( ppcArgV[*_piArgI][0] == '\0' || __pcEnd[0] == '\0' )
      {
        struct group *__pGroup = getgrnam( ppcArgV[*_piArgI] );
        if( __pGroup ) iDaemonGid = __pGroup->gr_gid;
        else displayErrorInvalidUidGid( ppcArgV[*_piArgI] );
      }
      else
        iDaemonGid = __iGid;
    }
    return 1;
  }
  else if( __sArg=="--syslog" )
  {
    if( !poSgctpUtilSyslog )
    {
      poSgctpUtilSyslog = new CSgctpUtilSyslog( sApplicationID );
      if( !poSgctpUtilSyslog )
        return -ENOMEM;
    }
    clog.rdbuf( poSgctpUtilSyslog );
    if( poSgctpUtilLog )
    {
      delete poSgctpUtilLog;
      poSgctpUtilLog = NULL;
    }
    return 1;
  }
  return 0;
}

void CSgctpUtilSkeleton::displayErrorExtraArgument( const string &_rsArg )
{
  SGCTP_LOG << SGCTP_ERROR << "Too many arguments (" << _rsArg << " ... )" << endl;
}

void CSgctpUtilSkeleton::displayErrorInvalidOption( const string &_rsArg )
{
  SGCTP_LOG << SGCTP_ERROR << "Invalid option (" << _rsArg << ")" << endl;
}

void CSgctpUtilSkeleton::displayErrorMissingArgument( const string &_rsArg )
{
  SGCTP_LOG << SGCTP_ERROR << "Missing argument for option (" << _rsArg << ")" << endl;
}

void CSgctpUtilSkeleton::displayErrorInvalidUidGid( const string &_rsArg )
{
  SGCTP_LOG << SGCTP_ERROR << "Invalid UID/GID (" << _rsArg << ")" << endl;
}

//
// SGCTP
//

int CSgctpUtilSkeleton::principalLookup( bool _bInput, bool _bOutput )
{
  if( sPrincipalsPath.empty() ) return 0;
  int __iReturn;

  // Lookup principal(s)

  // ... input
  if( _bInput )
  {
    if( !poTransmit_in ) return -ENODATA;
    uint64_t __ui64tPrincipalID = poTransmit_in->usePrincipal()->getID();
    __iReturn = poTransmit_in->usePrincipal()->read( sPrincipalsPath.c_str(), __ui64tPrincipalID );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to lookup principal (" << sPrincipalsPath << ":" << __ui64tPrincipalID << ")" << endl;
      return -EACCES;
    }
  }
  // ... output
  if( _bOutput )
  {
    if( !poTransmit_out ) return -ENODATA;
    uint64_t __ui64tPrincipalID = poTransmit_out->usePrincipal()->getID();
    __iReturn = poTransmit_out->usePrincipal()->read( sPrincipalsPath.c_str(), __ui64tPrincipalID );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to lookup principal (" << sPrincipalsPath << ":" << __ui64tPrincipalID << ")" << endl;
      return -EACCES;
    }
  }

  // Done
  return 0;
}

int CSgctpUtilSkeleton::transmitInit( bool _bInput, bool _bOutput )
{
  int __iReturn;

  // Initialize transmission object(s)

  // ... input
  if( _bInput )
  {
    if( !poTransmit_in ) return -ENODATA;
    poTransmit_in->setPasswordSalt( pucPasswordSalt_in );
    __iReturn = poTransmit_in->initPayload( ui8tPayloadType_in );
    poTransmit_in->usePrincipal()->erasePassword(); // clear password from memory
    if( __iReturn < 0 )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to initialize payload (" << to_string( ui8tPayloadType_in ) << ") @ initPayload=" << __iReturn << endl;
      return __iReturn;
    }
  }

  // ... output
  if( _bOutput )
  {
    if( !poTransmit_out ) return -ENODATA;
    poTransmit_out->setPasswordSalt( pucPasswordSalt_out );
    __iReturn = poTransmit_out->initPayload( ui8tPayloadType_out );
    poTransmit_out->usePrincipal()->erasePassword(); // clear password from memory
    if( __iReturn < 0 )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to initialize payload (" << to_string( ui8tPayloadType_out ) << ") @ initPayload=" << __iReturn << endl;
      return __iReturn;
    }
  }

  // Done
  return 0;
}

//
// Runtime
//

int CSgctpUtilSkeleton::daemonStart()
{
  // Check whether we should daemonize
  if( !bDaemon )
    return 0;

  // Check UID/GID
  if( iDaemonUid < 0 || iDaemonGid < 0 )
    return -EINVAL;
  if( !iDaemonUid )
    iDaemonUid = getuid();
  if( !iDaemonGid )
    iDaemonGid = getgid();

  // Daemonize
  pid_t __pidProcess = 0;
  pid_t __pidSession = 0;
  // ... fork
  if( ( __pidProcess = fork() ) < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to fork @ fork=" << -errno << endl;
    return -errno;
  }
  // ... kill parent
  if( __pidProcess > 0 )
  {
    // ... save PID before we do so
    if( !sDaemonPidPath.empty() )
    {
      if( sDaemonPidPath[0] != '/' )
      {
        SGCTP_LOG << SGCTP_WARNING << "PID file path must be absolute (" << sDaemonPidPath << ")" << endl;
        exit( 0 );
      }
      int __fd = open( sDaemonPidPath.c_str(),
                       O_CREAT|O_TRUNC|O_WRONLY,
                       S_IWUSR|S_IRUSR|S_IRGRP );
      if( __fd < 0 )
      {
        SGCTP_LOG << SGCTP_WARNING << "Failed to save PID file (" << sDaemonPidPath << ") @ open=" << -errno << endl;
        exit( 0 );
      }
      char __pcPID[10];
      snprintf( __pcPID, 10, "%d\n", __pidProcess );
      if( write( __fd, __pcPID, strlen( __pcPID ) ) < 0 )
      {
        SGCTP_LOG << SGCTP_WARNING << "Failed to write PID to file (" << sDaemonPidPath << ") @ write=" << -errno << endl;
      }
      if( fchown( __fd, iDaemonUid, iDaemonGid ) )
      {
        SGCTP_LOG << SGCTP_WARNING << "Failed to change PID file ownership (" << sDaemonPidPath << ") @ fchown=" << -errno << endl;
      }
      close( __fd );
    }
    exit( 0 );
  }
  // ... safe umask
  umask( 0 );
  // ... create new session
  if( ( __pidSession = setsid() ) < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to create new session @ setsid=" << -errno << endl;
    return -errno;
  }
  // ... change UID/GID
  if( iDaemonUid && setuid( iDaemonUid ) < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to change UID (" << to_string( iDaemonUid ) << ") @ setuid=" << -errno << endl;
    return -errno;
  }
  if( iDaemonGid && setgid( iDaemonGid ) < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to change GID (" << to_string( iDaemonGid ) << ") @ setgid=" << -errno << endl;
    return -errno;
  }
  // ... safe working directory
  if( chdir( "/" ) )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to change working directory @ chdir=" << -errno << endl;
    return -errno;
  }
  // ... logging
  if( poSgctpUtilLog )
  {
    SGCTP_LOG << SGCTP_WARNING << "Standard output logging will be now be disabled" << endl;
    SGCTP_LOG << SGCTP_WARNING << "Use syslog to maintain full logging capabilities" << endl;
    CSgctpUtilLog::SGCTP_LOG_LEVEL = LOG_EMERG;
  }
  // ... close standard descriptors
  close( STDIN_FILENO );
  close( STDOUT_FILENO );
  close( STDERR_FILENO );

  // Done
  SGCTP_LOG << SGCTP_INFO << "Daemon started" << endl;
  return 0;
}

void CSgctpUtilSkeleton::daemonEnd()
{
  // Check whether we have daemonized
  if( !bDaemon ) return;

  // Clean-up
  // ... delete PID file
  if( !sDaemonPidPath.empty() )
  {
    unlink( sDaemonPidPath.c_str() );
  }

  // Done
  SGCTP_LOG << SGCTP_INFO << "Daemon ended" << endl;
}

void CSgctpUtilSkeleton::sigCatch( void (*_pfHandler)(int) )
{
  // Catch signals (and set handler)
  struct sigaction __tSigaction_new, __tSigaction_old;
  sigemptyset( &tSigsetT );
  sigaddset( &tSigsetT, SIGINT );
  sigaddset( &tSigsetT, SIGTERM );
  sigaddset( &tSigsetT, SIGPIPE );
  __tSigaction_new.sa_mask = tSigsetT;
  __tSigaction_new.sa_handler = _pfHandler;
  __tSigaction_new.sa_flags = 0;
  sigaction( SIGINT, NULL, &__tSigaction_old );
  if( __tSigaction_old.sa_handler != SIG_IGN )
    sigaction( SIGINT, &__tSigaction_new, NULL );
  sigaction( SIGTERM, NULL, &__tSigaction_old );
  if( __tSigaction_old.sa_handler != SIG_IGN )
    sigaction( SIGTERM, &__tSigaction_new, NULL );
  sigaction( SIGPIPE, NULL, &__tSigaction_old );
  if( __tSigaction_old.sa_handler != SIG_IGN )
    sigaction( SIGPIPE, &__tSigaction_new, NULL );
}

void CSgctpUtilSkeleton::sigBlock()
{
  sigprocmask( SIG_BLOCK, &tSigsetT, NULL );
}

void CSgctpUtilSkeleton::sigUnblock()
{
  sigprocmask( SIG_UNBLOCK, &tSigsetT, NULL );
}
