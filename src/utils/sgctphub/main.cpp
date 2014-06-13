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
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

// C++
#include <vector>
using namespace std;

// SGCTP
#include "main.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// STATIC / CONSTANTS
//----------------------------------------------------------------------

pthread_t CSgctpHub::THREAD_DATA;
void* CSgctpHub::threadData( void* _poSgctpHub )
{
  return ((CSgctpHub*)_poSgctpHub)->dataThread();
}

pthread_t CSgctpHub::THREAD_AGENT_UDP;
void* CSgctpHub::threadAgentUDP( void* _poSgctpHub )
{
  return ((CSgctpHub*)_poSgctpHub)->agentUDPThread();
}

pthread_t CSgctpHub::THREAD_AGENT_TCP;
void* CSgctpHub::threadAgentTCP( void* _poSgctpHub )
{
  return ((CSgctpHub*)_poSgctpHub)->agentTCPThread();
}

pthread_t CSgctpHub::THREAD_CLIENT_RX;
void* CSgctpHub::threadClientRX( void* _poSgctpHub )
{
  return ((CSgctpHub*)_poSgctpHub)->clientRXThread();
}

pthread_t CSgctpHub::THREAD_CLIENT_TX;
void* CSgctpHub::threadClientTX( void* _poSgctpHub )
{
  return ((CSgctpHub*)_poSgctpHub)->clientTXThread();
}

void* CSgctpHub::getInAddr( struct sockaddr *_ptSockaddr )
{
  if( _ptSockaddr->sa_family == AF_INET )
    return &( ((struct sockaddr_in*)_ptSockaddr)->sin_addr );
  return &( ((struct sockaddr_in6*)_ptSockaddr)->sin6_addr );
}

void CSgctpHub::interrupt( int _iSignal )
{
  if( _iSignal == SIGPIPE )
    return;
  SGCTP_INTERRUPTED = 1;
  pthread_cancel( THREAD_DATA );
  pthread_cancel( THREAD_AGENT_UDP );
  pthread_cancel( THREAD_AGENT_TCP );
  pthread_cancel( THREAD_CLIENT_RX );
  pthread_cancel( THREAD_CLIENT_TX );
}


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpHubData::CSgctpHubData()
  : fdEpoch( CData::UNDEFINED_VALUE )
  , fdLatitude( CData::UNDEFINED_VALUE )
  , fdLongitude( CData::UNDEFINED_VALUE )
  , fdElevation( CData::UNDEFINED_VALUE )
{};

CSgctpHubAgentTCP::CSgctpHubAgentTCP()
  : sIP( "" )
  , ui64tPackets( 0 )
  , ui64tBytes( 0 )
  , iError( 0 )
{};

CSgctpHubClient::CSgctpHubClient()
  : bSync( false )
  , sIP( "" )
  , ui64tPackets( 0 )
  , ui64tBytes( 0 )
  , iError( 0 )
  , fdLatitude0( CData::UNDEFINED_VALUE )
  , fdLongitude0( CData::UNDEFINED_VALUE )
  , fdTime1( CData::UNDEFINED_VALUE )
  , fdLatitude1( CData::UNDEFINED_VALUE )
  , fdLongitude1( CData::UNDEFINED_VALUE )
  , fdElevation1( CData::UNDEFINED_VALUE )
  , fdTime2( CData::UNDEFINED_VALUE )
  , fdLatitude2( CData::UNDEFINED_VALUE )
  , fdLongitude2( CData::UNDEFINED_VALUE )
  , fdElevation2( CData::UNDEFINED_VALUE )
  , bTimeLimit( false )
  , fdTimeMin( CData::UNDEFINED_VALUE )
  , fdTimeMax( CData::UNDEFINED_VALUE )
  , bDistanceLimit( false )
  , fdDistanceMin( CData::UNDEFINED_VALUE )
  , fdDistanceMax( CData::UNDEFINED_VALUE )
  , bAzimuthLimit( false )
  , fdAzimuthMin( CData::UNDEFINED_VALUE )
  , fdAzimuthMax( CData::UNDEFINED_VALUE )
  , bElevationLimit( false )
  , fdElevationMin( CData::UNDEFINED_VALUE )
  , fdElevationMax( CData::UNDEFINED_VALUE )
{};

CSgctpHub::CSgctpHub( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "sgctphub", _iArgC, _ppcArgV )
  , bAgentUDPEnabled( true )
  , sdAgentUDP( -1 )
  , ptAddrinfo_AgentUDP( NULL )
  , sdAgentTCP( -1 )
  , ptAddrinfo_AgentTCP( NULL )
  , sdClient( -1 )
  , ptAddrinfo_Client( NULL )
  , sInputHost( "localhost" )
  , sInputPort_AgentUDP( "8947" )
  , sInputPort_AgentTCP( "8947" )
  , sInputPort_Client( "8948" )
  , fdTimeThrottle (CData::UNDEFINED_VALUE )
  , fdDistanceThreshold( CData::UNDEFINED_VALUE )
  , iDataTTL( 3600 )
{
  // Link actual transmission objects
  poTransmit_in = &oTransmit_AgentUDP;
};

CSgctpHub::~CSgctpHub()
{
  // De-allocate data resources
  for( unordered_map<string,CSgctpHubData*>::const_iterator __it =
         poSgctpHubData_umap.begin();
       __it != poSgctpHubData_umap.end();
       ++__it )
    delete __it->second;

  // De-allocate TCP agent resources
  for( unordered_map<int,CSgctpHubAgentTCP*>::const_iterator __it =
         poSgctpHubAgentTCP_umap.begin();
       __it != poSgctpHubAgentTCP_umap.end();
       ++__it )
  {
    delete __it->second;
    close( __it->first );
  }

  // De-allocate client resources
  for( unordered_map<int,CSgctpHubClient*>::const_iterator __it =
         poSgctpHubClient_umap.begin();
       __it != poSgctpHubClient_umap.end();
       ++__it )
  {
    delete __it->second;
    close( __it->first );
  }
};


//----------------------------------------------------------------------
// METHODS: CSgctpUtilSkeleton (implement/override)
//----------------------------------------------------------------------

void CSgctpHub::displayHelp()
{
  // Display full help
  displayUsageHeader();
  cout << "  sgctphub [options]" << endl;
  displaySynopsisHeader();
  cout << "  SGCTP data aggregation and redistribution hub." << endl;
  displayOptionsHeader();
  cout << "  -h, --host <host>" << endl;
  cout << "    Host name or IP address to bind to (defaut:localhost)" << endl;
  cout << "  -pu, --port-agent-udp <port>" << endl;
  cout << "    (Anonymous) UDP agents host port (defaut:8947); set to 0 to disable" << endl;
  cout << "  -pt, --port-agent-tcp <port>" << endl;
  cout << "    TCP agents host port (defaut:8947)" << endl;
  cout << "  -pc, --port-client-tcp <port>" << endl;
  cout << "    TCP clients host port (defaut:8948)" << endl;
  cout << "  -t, --time-throttle <delay>" << endl;
  cout << "    Throttle output (defaut:none, seconds)" << endl;
  cout << "  -d, --distance-threshold <distance>" << endl;
  cout << "    Distance variation threshold (defaut:none, meters)" << endl;
  displayOptionPrincipal( true, false );
  cout << "    NB: Applies to UDP agents only" << endl;
  displayOptionPayload( true, false );
  cout << "    NB: Applies to UDP agents only" << endl;
  displayOptionPassword( true, false );
  cout << "    NB: Applies to UDP agents only" << endl;
  displayOptionPasswordSalt( true, false );
  cout << "    NB: Applies to UDP agents only" << endl;
  displayOptionDaemon();
  cout << "  --ttl <seconds>" << endl;
  cout << "    Internal data Time-To-Live/TTL (default:3600)" << endl;
}

int CSgctpHub::parseArgs()
{
  // Parse all arguments
  int __iArgCount = 0;
  for( int __i=1; __i<iArgC; __i++ )
  {
    string __sArg = ppcArgV[__i];
    if( __sArg[0] == '-' )
    {
      SGCTP_PARSE_ARGS( parseArgsHelp( &__i ) );
      SGCTP_PARSE_ARGS( parseArgsPrincipal( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPayload( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPassword( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsDaemon( &__i ) );
      if( __sArg=="-h" || __sArg=="--host" )
      {
        if( ++__i<iArgC )
          sInputHost = ppcArgV[__i];
      }
      else if( __sArg=="-pu" || __sArg=="--port-agent-udp" )
      {
        if( ++__i<iArgC )
          sInputPort_AgentUDP = ppcArgV[__i];
        bAgentUDPEnabled = (bool)atoi( sInputPort_AgentUDP.c_str() );
      }
      else if( __sArg=="-pt" || __sArg=="--port-agent-tcp" )
      {
        if( ++__i<iArgC )
          sInputPort_AgentTCP = ppcArgV[__i];
      }
      else if( __sArg=="-pc" || __sArg=="--port-client-tcp" )
      {
        if( ++__i<iArgC )
          sInputPort_Client = ppcArgV[__i];
      }
      else if( __sArg=="-t" || __sArg=="--time-throttle" )
      {
        if( ++__i<iArgC )
          fdTimeThrottle = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-d" || __sArg=="--distance-threshold" )
      {
        if( ++__i<iArgC )
          fdDistanceThreshold = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="--ttl" )
      {
        if( ++__i<iArgC )
          iDataTTL = atoi( ppcArgV[__i] );
      }
      else if( __sArg[0] == '-' )
      {
        displayErrorInvalidOption( __sArg );
        return -EINVAL;
      }
      if( __i == iArgC )
      {
        displayErrorMissingArgument ( __sArg );
        return -EINVAL;
      }
    }
    else
    {
      switch( __iArgCount++ )
      {
      default:
        displayErrorExtraArgument( __sArg );
        return -EINVAL;
      }
    }
  }

  // Done
  return 0;
}

#define SGCTP_BREAK( __iExit__ ) { SGCTP_INTERRUPTED=1; __iExit = __iExit__; break; }
int CSgctpHub::exec()
{
  int __iExit = 0;
  int __iReturn;

  // Parse arguments
  __iReturn = parseArgs();
  if( __iReturn )
    return __iReturn;

  // Lookup principal(s)
  if( bAgentUDPEnabled )
  {
    __iReturn = principalLookup( true, false );
    if( __iReturn )
      return __iReturn;
  }

  // Initialize transmission object(s)
  if( bAgentUDPEnabled )
  {
    __iReturn = transmitInit( true, false );
    if( __iReturn )
      return __iReturn;
  }

  // Daemonize
  daemonStart();

  // Catch signals
  sigCatch( CSgctpHub::interrupt );

  // Error-catching block
  do
  {
    // Initialization
    // ... general
    __iReturn = init();
    if( __iReturn )
      SGCTP_BREAK( __iReturn );
    // ... data
    __iReturn = dataInit();
    if( __iReturn )
      SGCTP_BREAK( __iReturn );
    // ... UDP agent
    if( bAgentUDPEnabled )
    {
      __iReturn = agentUDPInit();
      if( __iReturn )
        SGCTP_BREAK( __iReturn );
    }
    // ... TCP agent
    __iReturn = agentTCPInit();
    if( __iReturn )
      SGCTP_BREAK( __iReturn );
    // ... client
    __iReturn = clientInit();
    if( __iReturn )
      SGCTP_BREAK( __iReturn );

    // Start threads
    // ... data
    __iReturn = pthread_create( &THREAD_DATA, NULL,
                                &CSgctpHub::threadData,
                                this );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to create data thread @ pthread_create=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }
    // ... UDP agent
    if( bAgentUDPEnabled )
    {
      __iReturn = pthread_create( &THREAD_AGENT_UDP, NULL,
                                  &CSgctpHub::threadAgentUDP,
                                  this );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to create UDP agent thread @ pthread_create=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }
    }
    // ... TCP agent
    __iReturn = pthread_create( &THREAD_AGENT_TCP, NULL,
                                &CSgctpHub::threadAgentTCP,
                                this );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to create TCP agent thread @ pthread_create=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }
    // ... client (RX/TX)
    __iReturn = pthread_create( &THREAD_CLIENT_RX, NULL,
                                &CSgctpHub::threadClientRX,
                                this );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to create client RX thread @ pthread_create=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }
    __iReturn = pthread_create( &THREAD_CLIENT_TX, NULL,
                                &CSgctpHub::threadClientTX,
                                this );
    if( __iReturn )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to create client TX thread @ pthread_create=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }

    // ... wait for threads to finish
    pthread_join( THREAD_DATA, NULL );
    if( bAgentUDPEnabled )
      pthread_join( THREAD_AGENT_UDP, NULL );
    pthread_join( THREAD_AGENT_TCP, NULL );
    pthread_join( THREAD_CLIENT_RX, NULL );
    pthread_join( THREAD_CLIENT_TX, NULL );

  }
  while( false ); // Error-catching block

  // Done
  pthread_mutex_destroy( &tLog_mutex );
  pthread_mutex_destroy( &tSgctpHubData_mutex );
  pthread_mutex_destroy( &tSyncID_mutex );
  if( sdAgentUDP >= 0 )
    close( sdAgentUDP );
  if( ptAddrinfo_AgentUDP )
    free( ptAddrinfo_AgentUDP );
  if( sdAgentTCP >= 0 )
    close( sdAgentTCP );
  if( ptAddrinfo_AgentTCP )
    free( ptAddrinfo_AgentTCP );
  if( sdClient >= 0 )
    close( sdClient );
  if( ptAddrinfo_Client )
    free( ptAddrinfo_Client );
  pthread_mutex_destroy( &tClientTX_mutex );
  pthread_cond_destroy( &tClientTX_cond );
  daemonEnd();
  return __iExit;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

//
// General
//

int CSgctpHub::init()
{
  int __iReturn;

  // Initialize mutex
  // ... logging
  __iReturn = pthread_mutex_init( &tLog_mutex, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize data mutex @ pthread_mutex_init=" << __iReturn << endl;
    return __iReturn;
  }

  // Initialize cryptographic engine
#ifndef __SGCTP_USE_OPENSSL__

  // ... GCrypt multi-threading
  __iReturn = gcry_control( GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize cryptopgraphic engine @ gcry_control=" << __iReturn << endl;
    return __iReturn;
  }

#endif // NOT __SGCTP_USE_OPENSSL__

  // Done
  return 0;
}

//
// Data
//

int CSgctpHub::dataInit()
{
  int __iReturn;

  // Initialize mutex
  // ... internal data map
  __iReturn = pthread_mutex_init( &tSgctpHubData_mutex, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize data mutex @ pthread_mutex_init=" << __iReturn << endl;
    return __iReturn;
  }
  // ... data (IDs) to syncronize
  __iReturn = pthread_mutex_init( &tSyncID_mutex, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize synchronization mutex @ pthread_mutex_init=" << __iReturn << endl;
    return __iReturn;
  }

  // Done
  return 0;
}

void* CSgctpHub::dataThread()
{
  for(;;)
  {
    if( SGCTP_INTERRUPTED ) break;

    // Sleep for 5 minutes
    sleep( 300 );

    // Clean-up internal data
    pthread_mutex_lock( &tSgctpHubData_mutex );
    dataCleanup();
    pthread_mutex_unlock( &tSgctpHubData_mutex );

  }
  pthread_exit( NULL );
}

bool CSgctpHub::dataSync( const CData &_roData )
{
  string __sID = _roData.getID();
  CSgctpHubData* __poSgctpHubData;
  double __fdEpochNow = CData::epoch();
  unordered_map<string,CSgctpHubData*>::const_iterator __it =
    poSgctpHubData_umap.find( __sID );
  bool __bSync = false;
  if( __it == poSgctpHubData_umap.end() )
  {

    // Create new data container
    __poSgctpHubData = new CSgctpHubData();
    __poSgctpHubData->oData.copy( _roData );
    __poSgctpHubData->fdEpoch =
      CData::toEpoch( _roData.getTime(), __fdEpochNow );
    __poSgctpHubData->fdLatitude = _roData.getLatitude();
    __poSgctpHubData->fdLongitude = _roData.getLongitude();
    __poSgctpHubData->fdElevation = _roData.getElevation();
    poSgctpHubData_umap[__sID] = __poSgctpHubData;
    __bSync = true;

  }
  else
  {

    // Synchronize existing data container
    __poSgctpHubData = __it->second;
    do // Error-catching block
    {

      // ... check time throttling
      if( CData::isDefined( fdTimeThrottle ) )
      {
        double __fdEpoch = CData::toEpoch( _roData.getTime(), __fdEpochNow );
        if( __fdEpoch - __poSgctpHubData->fdEpoch < fdTimeThrottle )
          break;
      }

      // ... check distance threshold
      if( CData::isDefined( fdDistanceThreshold ) )
      {
        double __fdLatitude = _roData.getLatitude();
        double __fdLongitude = _roData.getLongitude();
        if( CData::isDefined( __fdLatitude )
            && CData::isDefined( __fdLongitude ) &&
            CData::isDefined( __poSgctpHubData->fdLatitude )
            && CData::isDefined( __poSgctpHubData->fdLongitude ) )
        {
          double __fdDistance = distanceRL( __poSgctpHubData->fdLatitude,
                                            __poSgctpHubData->fdLongitude,
                                            __fdLatitude,
                                            __fdLongitude );
          double __fdElevation = _roData.getElevation();
          if( CData::isDefined( __fdElevation )
              && CData::isDefined( __poSgctpHubData->fdElevation ) )
          {
            double __fdElevationDelta =
              __poSgctpHubData->fdLatitude - __fdElevation;
            __fdDistance = sqrt( __fdDistance*__fdDistance
                                 + __fdElevationDelta*__fdElevationDelta );
          }
          if( __fdDistance < fdDistanceThreshold )
            break;
        }
      }

      // ... sync
      __bSync = __poSgctpHubData->oData.sync( _roData );

    }
    while( false ); // Error-catching block
  }
  return __bSync;
}

void CSgctpHub::dataCleanup()
{
  // Get data keys (source IDs)
  vector<string> __vsKeys;
  __vsKeys.reserve( poSgctpHubData_umap.size() );
  for( unordered_map<string,CSgctpHubData*>::const_iterator __it =
         poSgctpHubData_umap.begin();
       __it != poSgctpHubData_umap.end();
       ++__it )
    __vsKeys.push_back( __it->first );

  // Loop through keys
  double __fdEpochNow = CData::epoch();
  for( vector<string>::const_iterator __it = __vsKeys.begin();
       __it != __vsKeys.end();
       ++__it )
  {
    // ... cleanup stale entries (older than data TTL)
    if( __fdEpochNow - poSgctpHubData_umap[ *__it ]->fdEpoch
        > (double)iDataTTL )
    {
      delete poSgctpHubData_umap[ *__it ];
      poSgctpHubData_umap.erase( *__it );
    }
  }
}


//
// UDP agents thread
//

int CSgctpHub::agentUDPInit()
{
  int __iReturn;

  // Open UDP agent socket

  // ... lookup socket address info
  struct addrinfo* __ptAddrinfoActual;
  struct addrinfo __tAddrinfoHints;
  memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
  __tAddrinfoHints.ai_family = AF_UNSPEC;
  __tAddrinfoHints.ai_socktype = SOCK_DGRAM;
  __tAddrinfoHints.ai_flags = AI_PASSIVE;
  __iReturn = getaddrinfo( sInputHost.c_str(),
                           sInputPort_AgentUDP.c_str(),
                           &__tAddrinfoHints,
                           &ptAddrinfo_AgentUDP );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve UDP agent socket address info (" << sInputHost << ":" << sInputPort_AgentUDP << ") @ getaddrinfo=" << __iReturn << endl;
    return __iReturn;
  }

  for( __ptAddrinfoActual = ptAddrinfo_AgentUDP;
       __ptAddrinfoActual != NULL;
       __ptAddrinfoActual = __ptAddrinfoActual->ai_next )
  {

    // ... create socket
    sdAgentUDP = socket( __ptAddrinfoActual->ai_family,
                         __ptAddrinfoActual->ai_socktype,
                         __ptAddrinfoActual->ai_protocol );
    if( sdAgentUDP < 0 )
    {
      SGCTP_LOG << SGCTP_WARNING << "Failed to create UDP agent socket (" << sInputHost << ":" << sInputPort_AgentUDP << ") @ socket=" << -errno << endl;
      continue;
    }

    // ... bind socket
    __iReturn = bind( sdAgentUDP,
                      __ptAddrinfoActual->ai_addr,
                      __ptAddrinfoActual->ai_addrlen );
    if( __iReturn )
    {
      close( sdAgentUDP );
      sdAgentUDP = -1;
      SGCTP_LOG << SGCTP_WARNING << "Failed to bind UDP agent socket (" << sInputHost << ":" << sInputPort_AgentUDP << ") @ bind=" << -errno << endl;
      continue;
    }
    break;

  }
  if( sdAgentUDP < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to create UDP agent socket (" << sInputHost << ":" << sInputPort_AgentUDP << ")" << endl;
    return( -errno );
  }

  // Done
  return 0;
}

void* CSgctpHub::agentUDPThread()
{
  int __iReturn;

  // Receive and dump data
  CData __oData;
  for(;;)
  {
    if( SGCTP_INTERRUPTED )
      break;

    // ... unserialize
    __iReturn = oTransmit_AgentUDP.unserialize( sdAgentUDP, &__oData );
    if( SGCTP_INTERRUPTED )
      break;
    if( __iReturn == 0 )
      break;
    if( __iReturn < 0 )
    {
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_WARNING << "Failed to unserialize UDP agent data @ unserialize=" << __iReturn << endl;
      pthread_mutex_unlock( &tLog_mutex );
      continue;
    }

    // ... synchronize data
    pthread_mutex_lock( &tSgctpHubData_mutex );
    bool __bSync = dataSync( __oData );
    pthread_mutex_unlock( &tSgctpHubData_mutex );
    if( __bSync )
    {
      pthread_mutex_lock( &tSyncID_mutex );
      sSyncID_queue.push( __oData.getID() );
      pthread_mutex_unlock( &tSyncID_mutex );
      pthread_cond_signal( &tClientTX_cond );
    }

  }
  pthread_exit( NULL );
}


//
// TCP agents thread
//

int CSgctpHub::agentTCPInit()
{
  int __iReturn;

  // Open TCP agent socket
  FD_ZERO( &sdAgentTCP_fdset );

  // ... lookup socket address info
  struct addrinfo* __ptAddrinfoActual;
  struct addrinfo __tAddrinfoHints;
  memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
  __tAddrinfoHints.ai_family = AF_UNSPEC;
  __tAddrinfoHints.ai_socktype = SOCK_STREAM;
  __tAddrinfoHints.ai_flags = AI_PASSIVE;
  __iReturn = getaddrinfo( sInputHost.c_str(),
                           sInputPort_AgentTCP.c_str(),
                           &__tAddrinfoHints,
                           &ptAddrinfo_AgentTCP );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve TCP agent socket address info (" << sInputHost << ":" << sInputPort_AgentTCP << ") @ getaddrinfo=" << __iReturn << endl;
    return __iReturn;
  }

  for( __ptAddrinfoActual = ptAddrinfo_AgentTCP;
       __ptAddrinfoActual != NULL;
       __ptAddrinfoActual = __ptAddrinfoActual->ai_next )
  {

    // ... create socket
    sdAgentTCP = socket( __ptAddrinfoActual->ai_family,
                         __ptAddrinfoActual->ai_socktype,
                         __ptAddrinfoActual->ai_protocol );
    if( sdAgentTCP < 0 )
    {
      SGCTP_LOG << SGCTP_WARNING << "Failed to create TCP agent socket (" << sInputHost << ":" << sInputPort_AgentTCP << ") @ socket=" << -errno << endl;
      continue;
    }

    // ... configure socket
    int __iSO_REUSEADDR=1;
    __iReturn = setsockopt( sdAgentTCP,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            &__iSO_REUSEADDR,
                            sizeof( int ) );
    if( __iReturn )
    {
      close( sdAgentTCP );
      sdAgentTCP = -1;
      SGCTP_LOG << SGCTP_WARNING << "Failed to configure TCP agent socket (" << sInputHost << ":" << sInputPort_AgentTCP << ") @ setsockopt=" << -errno << endl;
      continue;
    }

    // ... bind socket
    __iReturn = bind( sdAgentTCP,
                      __ptAddrinfoActual->ai_addr,
                      __ptAddrinfoActual->ai_addrlen );
    if( __iReturn )
    {
      close( sdAgentTCP );
      sdAgentTCP = -1;
      SGCTP_LOG << SGCTP_WARNING << "Failed to bind TCP agent socket (" << sInputHost << ":" << sInputPort_AgentTCP << ") @ bind=" << -errno << endl;
      continue;
    }
    break;

  }
  if( sdAgentTCP < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to create TCP agent socket (" << sInputHost << ":" << sInputPort_AgentTCP << ")" << endl;
    return -errno;
  }

  // ... listen on socket
  __iReturn = listen( sdAgentTCP, 1 );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to listen on TCP agent socket (" << sInputHost << ":" << sInputPort_AgentTCP << ") @ listen=" << -errno << endl;
    return -errno;
  }

  // ... add listening socket to set
  FD_SET( sdAgentTCP, &sdAgentTCP_fdset );
  sdAgentTCP_max = sdAgentTCP;

  // Done
  return 0;
}

void* CSgctpHub::agentTCPThread()
{
  int __iReturn;

  // Loop through TCP connections
  for(;;)
  {
    if( SGCTP_INTERRUPTED )
      break;

    // Wait for connection
    fd_set __fdset = sdAgentTCP_fdset;
    __iReturn = select( sdAgentTCP_max+1, &__fdset, NULL, NULL, NULL );
    if( __iReturn < 0 )
    {
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_WARNING << "Failed to wait for TCP agent connection @ select=" << -errno << endl;
      pthread_mutex_unlock( &tLog_mutex );
      continue;
    }

    // Loop through selected connections
    for( int __i=0; __i<=sdAgentTCP_max; __i++ )
    {
      if( FD_ISSET( __i, &__fdset ) )
      {

        if( __i == sdAgentTCP )
        {

          // ... new connection(s)
          struct sockaddr_storage __tSockaddrRemote;
          socklen_t __tSocklenRemote = sizeof( __tSockaddrRemote );
          int __sdAgentTCP_new = accept( sdAgentTCP,
                                         (struct sockaddr*)&__tSockaddrRemote,
                                         &__tSocklenRemote );
          if( __sdAgentTCP_new < 0 )
          {
            pthread_mutex_lock( &tLog_mutex );
            SGCTP_LOG << SGCTP_WARNING << "Failed to accept TCP agent connection @ accept=" << -errno << endl;
            pthread_mutex_unlock( &tLog_mutex );
            continue;
          }

          // ... retrieve remote IP address
          char __pcIP[INET6_ADDRSTRLEN];
          inet_ntop( __tSockaddrRemote.ss_family,
                     getInAddr( (struct sockaddr*)&__tSockaddrRemote ),
                     __pcIP, INET6_ADDRSTRLEN );

          // ... error-catching block
          int __iError = 1;
          CSgctpHubAgentTCP *__poSgctpHubAgentTCP = NULL;
          do
          {

            // ... create transmission object
            __poSgctpHubAgentTCP = new CSgctpHubAgentTCP();
            if( !__poSgctpHubAgentTCP )
            {
              pthread_mutex_lock( &tLog_mutex );
              SGCTP_LOG << SGCTP_WARNING << "Failed to create (allocate) TCP agent transmission object" << endl;
              pthread_mutex_unlock( &tLog_mutex );
              __iError = -ENOMEM;
              break;
            }
            __poSgctpHubAgentTCP->sIP = __pcIP;
            if( !sPrincipalsPath.empty() )
              __poSgctpHubAgentTCP->oTransmit.setPrincipalsPath( sPrincipalsPath.c_str() );

            // ... receive handshake
            __iReturn =
              __poSgctpHubAgentTCP->oTransmit.recvHandshake( __sdAgentTCP_new );
            if( __iReturn < 0 )
            {
              pthread_mutex_lock( &tLog_mutex );
              SGCTP_LOG << SGCTP_ERROR << "TCP agent handshake failed"
                        << "; ip=" << __poSgctpHubAgentTCP->sIP
                        << "; err=" << __iReturn
                        << endl;
              pthread_mutex_unlock( &tLog_mutex );
              __iError = __iReturn;
              break;
            }

          }
          while( false ); // ... error-catching block
          if( __iError <= 0 )
          {
            if( __poSgctpHubAgentTCP )
              delete __poSgctpHubAgentTCP;
            if( __sdAgentTCP_new )
              close( __sdAgentTCP_new );
            continue;
          }
          poSgctpHubAgentTCP_umap[__sdAgentTCP_new] = __poSgctpHubAgentTCP;

          // ... add agent to (connected) agents set
          FD_SET( __sdAgentTCP_new, &sdAgentTCP_fdset );
          if( __sdAgentTCP_new > sdAgentTCP_max )
            sdAgentTCP_max = __sdAgentTCP_new;

          // ... log
          pthread_mutex_lock( &tLog_mutex );
          SGCTP_LOG << SGCTP_INFO << "TCP agent connected"
                    << "; ip=" << __poSgctpHubAgentTCP->sIP
                    << ", id=" << to_string( __poSgctpHubAgentTCP->oTransmit.usePrincipal()->getID() )
                    << endl;
          pthread_mutex_unlock( &tLog_mutex );

          // ... process TCP agents (RX) data
          if( __poSgctpHubAgentTCP->oTransmit.hasData() )
            agentTCPInput( __sdAgentTCP_new, __poSgctpHubAgentTCP );

        }
        else
        {

          // ... process TCP agents (RX) data
          agentTCPInput( __i, poSgctpHubAgentTCP_umap[__i] );

        }

      }

    } // Loop through selected connections

  } // Loop through TCP connections
  pthread_exit( NULL );
}

void CSgctpHub::agentTCPInput( int _iSocket,
                               CSgctpHubAgentTCP *_poSgctpHubAgentTCP )
{
  int __iReturn;

  // Set transmission timeout
  _poSgctpHubAgentTCP->oTransmit.setTimeout( _iSocket, 3.0 ); // prevent DoS

  // Loop through available TCP agents data
  for(;;)
  {

    // ... unserialize TCP agents data
    CData __oData;
    __iReturn =
      _poSgctpHubAgentTCP->oTransmit.unserialize( _iSocket, &__oData );
    if( __iReturn <= 0 )
    {
      _poSgctpHubAgentTCP->iError = __iReturn;

      // ... connection interrupted
      if( __iReturn < 0 )
      {
        pthread_mutex_lock( &tLog_mutex );
        SGCTP_LOG << SGCTP_WARNING << "Failed to unserialize TCP agent data @ unserialize=" << __iReturn << endl;
        pthread_mutex_unlock( &tLog_mutex );
      }

      // ... log
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_INFO << "TCP agent disconnected"
                << "; ip=" << _poSgctpHubAgentTCP->sIP
                << ", id=" << to_string( _poSgctpHubAgentTCP->oTransmit.usePrincipal()->getID() )
                << ", pkts=" << to_string( _poSgctpHubAgentTCP->ui64tPackets )
                << ", bytes=" << to_string( _poSgctpHubAgentTCP->ui64tBytes )
                << ", err=" << to_string( _poSgctpHubAgentTCP->iError )
                << endl;
      pthread_mutex_unlock( &tLog_mutex );

      // ... delete transmission object
      delete _poSgctpHubAgentTCP;
      poSgctpHubAgentTCP_umap.erase( _iSocket );
      close( _iSocket );
      FD_CLR( _iSocket, &sdAgentTCP_fdset );
      if( _iSocket == sdAgentTCP_max )
        for( int __j=sdAgentTCP_max-1; __j>=0; __j-- )
          if( FD_ISSET( __j, &sdAgentTCP_fdset ) )
          {
            sdAgentTCP_max = __j;
            break;
          }

    }
    else
    {
      int __iPayloadSize = __iReturn;

      // ... increase counters
      _poSgctpHubAgentTCP->ui64tPackets++;
      _poSgctpHubAgentTCP->ui64tBytes += __iPayloadSize;

      // ... synchronize data
      pthread_mutex_lock( &tSgctpHubData_mutex );
      bool __bSync = dataSync( __oData );
      pthread_mutex_unlock( &tSgctpHubData_mutex );
      if( __bSync )
      {
        pthread_mutex_lock( &tSyncID_mutex );
        sSyncID_queue.push( __oData.getID() );
        pthread_mutex_unlock( &tSyncID_mutex );
        pthread_cond_signal( &tClientTX_cond );
      }

    }

    if( !_poSgctpHubAgentTCP->oTransmit.hasData() )
      break;

  } // Loop through available TCP agents data

  // Unset transmission timeout
  _poSgctpHubAgentTCP->oTransmit.setTimeout( _iSocket, 0.0 );
}


//
// (TCP) clients threads
//

int CSgctpHub::clientInit()
{
  int __iReturn;

  // Initialize mutex
  // ... deletion
  __iReturn = pthread_mutex_init( &tClientDelete_mutex, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize client deletion mutex @ pthread_mutex_init=" << __iReturn << endl;
    return __iReturn;
  }
  // ... data transmission (TX)
  __iReturn = pthread_mutex_init( &tClientTX_mutex, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize client data transmission mutex @ pthread_mutex_init=" << __iReturn << endl;
    return __iReturn;
  }

  // Initialize condition
  // ... data transmission (TX)
  __iReturn = pthread_cond_init( &tClientTX_cond, NULL );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to initialize client data transmission condition @ pthread_cond_init=" << __iReturn << endl;
    return __iReturn;
  }

  // Open (TCP) client socket
  FD_ZERO( &sdClient_fdset );

  // ... lookup socket address info
  struct addrinfo* __ptAddrinfoActual;
  struct addrinfo __tAddrinfoHints;
  memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
  __tAddrinfoHints.ai_family = AF_UNSPEC;
  __tAddrinfoHints.ai_socktype = SOCK_STREAM;
  __tAddrinfoHints.ai_flags = AI_PASSIVE;
  __iReturn = getaddrinfo( sInputHost.c_str(),
                           sInputPort_Client.c_str(),
                           &__tAddrinfoHints,
                           &ptAddrinfo_Client );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve TCP client socket address info (" << sInputHost << ":" << sInputPort_Client << ") @ getaddrinfo=" << __iReturn << endl;
    return __iReturn;
  }

  for( __ptAddrinfoActual = ptAddrinfo_Client;
       __ptAddrinfoActual != NULL;
       __ptAddrinfoActual = __ptAddrinfoActual->ai_next )
  {

    // ... create socket
    sdClient = socket( __ptAddrinfoActual->ai_family,
                       __ptAddrinfoActual->ai_socktype,
                       __ptAddrinfoActual->ai_protocol );
    if( sdClient < 0 )
    {
      SGCTP_LOG << SGCTP_WARNING << "Failed to create TCP client socket (" << sInputHost << ":" << sInputPort_Client << ") @ socket=" << -errno << endl;
      continue;
    }

    // ... configure socket
    int __iSO_REUSEADDR=1;
    __iReturn = setsockopt( sdClient,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            &__iSO_REUSEADDR,
                            sizeof( int ) );
    if( __iReturn )
    {
      close( sdClient );
      sdClient = -1;
      SGCTP_LOG << SGCTP_WARNING << "Failed to configure TCP client socket (" << sInputHost << ":" << sInputPort_Client << ") @ setsockopt=" << -errno << endl;
      continue;
    }

    // ... bind socket
    __iReturn = bind( sdClient,
                      __ptAddrinfoActual->ai_addr,
                      __ptAddrinfoActual->ai_addrlen );
    if( __iReturn )
    {
      close( sdClient );
      sdClient = -1;
      SGCTP_LOG << SGCTP_WARNING << "Failed to bind TCP client socket (" << sInputHost << ":" << sInputPort_Client << ") @ bind=" << -errno << endl;
      continue;
    }
    break;
  }
  if( sdClient < 0 )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to create TCP client socket (" << sInputHost << ":" << sInputPort_Client << ")" << endl;
    return -errno;
  }

  // ... listen on socket
  __iReturn = listen( sdClient, 1 );
  if( __iReturn )
  {
    SGCTP_LOG << SGCTP_ERROR << "Failed to listen on TCP client socket (" << sInputHost << ":" << sInputPort_Client << ") @ listen=" << -errno << endl;
    return -errno;
  }

  // ... add listening socket to set
  FD_SET( sdClient, &sdClient_fdset );
  sdClient_max = sdClient;

  // Done
  return 0;
}

void* CSgctpHub::clientRXThread()
{
  int __iReturn;

  // Loop through TCP connections
  for(;;)
  {
    if( SGCTP_INTERRUPTED )
      break;

    // Wait for connection
    fd_set __fdset = sdClient_fdset;
    __iReturn = select( sdClient_max+1, &__fdset, NULL, NULL, NULL );
    if( __iReturn < 0 )
    {
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_WARNING << "Failed to wait for client connection @ select=" << -errno << endl;
      pthread_mutex_unlock( &tLog_mutex );
      continue;
    }

    // Loop through selected connections
    for( int __i=0; __i<=sdClient_max; __i++ )
    {
      if( FD_ISSET( __i, &__fdset ) )
      {

        if( __i == sdClient )
        {

          // ... new connection(s)
          struct sockaddr_storage __tSockaddrRemote;
          socklen_t __tSocklenRemote = sizeof( __tSockaddrRemote );
          int __sdClient_new = accept( sdClient,
                                       (struct sockaddr*)&__tSockaddrRemote,
                                       &__tSocklenRemote );
          if( __sdClient_new < 0 )
          {
            pthread_mutex_lock( &tLog_mutex );
            SGCTP_LOG << SGCTP_WARNING << "Failed to accept client connection @ accept=" << -errno << endl;
            pthread_mutex_unlock( &tLog_mutex );
            continue;
          }

          // ... retrieve remote IP address
          char __pcIP[INET6_ADDRSTRLEN];
          inet_ntop( __tSockaddrRemote.ss_family,
                     getInAddr( (struct sockaddr*)&__tSockaddrRemote ),
                     __pcIP, INET6_ADDRSTRLEN );

          // ... error-catching block
          int __iError = 1;
          CSgctpHubClient *__poSgctpHubClient = NULL;
          do
          {

            // ... create client container
            __poSgctpHubClient = new CSgctpHubClient();
            if( !__poSgctpHubClient )
            {
              pthread_mutex_lock( &tLog_mutex );
              SGCTP_LOG << SGCTP_WARNING << "Failed to create (allocate) client container object" << endl;
              pthread_mutex_unlock( &tLog_mutex );
              __iError = -ENOMEM;
              break;
            }
            __poSgctpHubClient->sIP = __pcIP;
            if( !sPrincipalsPath.empty() )
              __poSgctpHubClient->oTransmit.setPrincipalsPath( sPrincipalsPath.c_str() );

            // ... receive handshake
            __iReturn =
              __poSgctpHubClient->oTransmit.recvHandshake( __sdClient_new );
            if( __iReturn < 0 )
            {
              pthread_mutex_lock( &tLog_mutex );
              SGCTP_LOG << SGCTP_ERROR << "Client handshake failed"
                        << "; ip=" << __poSgctpHubClient->sIP
                        << "; err=" << __iReturn
                        << endl;
              pthread_mutex_unlock( &tLog_mutex );
              __iError = __iReturn;
              break;
            }

          }
          while( false ); // ... error-catching block
          if( __iError <= 0 )
          {
            if( __poSgctpHubClient )
              delete __poSgctpHubClient;
            if( __sdClient_new )
              close( __sdClient_new );
            continue;
          }
          poSgctpHubClient_umap[__sdClient_new] = __poSgctpHubClient;

          // ... add client to (connected) clients set
          FD_SET( __sdClient_new, &sdClient_fdset );
          if( __sdClient_new > sdClient_max )
            sdClient_max = __sdClient_new;

          // ... log
          pthread_mutex_lock( &tLog_mutex );
          SGCTP_LOG << SGCTP_INFO << "Client connected"
                    << "; ip=" << __poSgctpHubClient->sIP
                    << ", id=" << to_string( __poSgctpHubClient->oTransmit.usePrincipal()->getID() )
                    << endl;
          pthread_mutex_unlock( &tLog_mutex );

          // ... process client (RX) data
          if( __poSgctpHubClient->oTransmit.hasData() )
            clientInput( __sdClient_new, __poSgctpHubClient );

        }
        else
        {

          // ... process client (RX) data
          clientInput( __i, poSgctpHubClient_umap[__i] );

        }

      }

    } // Loop through selected connections

  } // Loop through TCP connections
  pthread_exit( NULL );
}

void* CSgctpHub::clientTXThread()
{
  int __iReturn;

  pthread_mutex_lock( &tClientTX_mutex );
  for(;;)
  {
    if( SGCTP_INTERRUPTED )
      break;

    // Wait to be woken-up
    pthread_cond_wait( &tClientTX_cond, &tClientTX_mutex );

    // Send data to clients
    for(;;)
    {
      // ... retrieve ID to send
      string __sID;
      pthread_mutex_lock( &tSyncID_mutex );
      if( sSyncID_queue.empty() )
      {
        pthread_mutex_unlock( &tSyncID_mutex );
        break;
      }
      __sID = sSyncID_queue.front();
      sSyncID_queue.pop();
      pthread_mutex_unlock( &tSyncID_mutex );

      // ... retrieve data to send
      CData __oData;
      pthread_mutex_lock( &tSgctpHubData_mutex );
      if( poSgctpHubData_umap.find( __sID ) == poSgctpHubData_umap.end() )
      {
        pthread_mutex_unlock( &tSgctpHubData_mutex );
        continue;
      }
      __oData.copy( poSgctpHubData_umap[__sID]->oData );
      pthread_mutex_unlock( &tSgctpHubData_mutex );

      // ... lock client deletion
      pthread_mutex_lock( &tClientDelete_mutex );

      // ... serialize data clients
      fd_set __sdClientClose_fdset;
      FD_ZERO( &__sdClientClose_fdset );
      for( int __i=sdClient_max; __i>=0; __i-- )
      {
        if( __i == sdClient )
          continue;
        if( !FD_ISSET( __i, &sdClient_fdset ) )
          continue;

        // ... retrieve client container
        CSgctpHubClient *__poSgctpHubClient = poSgctpHubClient_umap[__i];

        // ... check synchronization status
        if( !__poSgctpHubClient->bSync )
          continue;

        // ... check limits (filter)
        if( !clientFilterCheck( __poSgctpHubClient, __oData ) )
          continue;

        // ... serialize data
        sigBlock();
        __iReturn = __poSgctpHubClient->oTransmit.serialize( __i, __oData );
        sigUnblock();
        if( __iReturn <= 0 )
        {
          __poSgctpHubClient->iError = __iReturn;
          FD_SET( __i, &__sdClientClose_fdset );
        }
        else
        {
          int __iPayloadSize = __iReturn;

          // ... increase counters
          __poSgctpHubClient->ui64tPackets++;
          __poSgctpHubClient->ui64tBytes += __iPayloadSize;
        }

      }

      // ... unlock client deletion
      pthread_mutex_unlock( &tClientDelete_mutex );

      // ... close trouble-makers
      for( int __i=sdClient_max; __i>=0; __i-- )
      {
        if( !FD_ISSET( __i, &__sdClientClose_fdset ) )
          continue;
        close( __i ); // TODO: check wheteher 'close' triggers ClientRXThread 'select' ?
      }

    }

  }
  pthread_mutex_unlock( &tClientTX_mutex );
  pthread_exit( NULL );
}

/// Client input processing
void CSgctpHub::clientInput( int _iSocket,
                             CSgctpHubClient *_poSgctpHubClient )
{
  int __iReturn;

  // Set transmission timeout
  _poSgctpHubClient->oTransmit.setTimeout( _iSocket, 3.0 ); // prevent DoS

  // Loop through available client data
  for(;;)
  {

    // ... unserialize clients data
    CData __oData;
    __iReturn =
      _poSgctpHubClient->oTransmit.unserialize( _iSocket, &__oData );
    if( __iReturn <= 0 )
    {
      _poSgctpHubClient->iError = __iReturn;

      // ... connection interrupted
      if( __iReturn < 0 )
      {
        pthread_mutex_lock( &tLog_mutex );
        SGCTP_LOG << SGCTP_WARNING << "Failed to unserialize client data @ unserialize=" << __iReturn << endl;
        pthread_mutex_unlock( &tLog_mutex );
      }

      // ... log
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_INFO << "Client disconnected"
                << "; ip=" << _poSgctpHubClient->sIP
                << ", id=" << to_string( _poSgctpHubClient->oTransmit.usePrincipal()->getID() )
                << ", pkts=" << to_string( _poSgctpHubClient->ui64tPackets )
                << ", bytes=" << to_string( _poSgctpHubClient->ui64tBytes )
                << ", err=" << to_string( _poSgctpHubClient->iError )
                << endl;
      pthread_mutex_unlock( &tLog_mutex );

      // ... lock client deletion
      pthread_mutex_lock( &tClientDelete_mutex );

      // ... delete client
      delete _poSgctpHubClient;
      poSgctpHubClient_umap.erase( _iSocket );
      close( _iSocket );
      FD_CLR( _iSocket, &sdClient_fdset );
      if( _iSocket == sdClient_max )
        for( int __j=sdClient_max-1; __j>=0; __j-- )
          if( FD_ISSET( __j, &sdClient_fdset ) )
          {
            sdClient_max = __j;
            break;
          }

      // ... unlock client deletion
      pthread_mutex_unlock( &tClientDelete_mutex );

    }
    else
    {

      // ... handle filter directives
      string __sID = __oData.getID();
      if( __sID == "###START" )
      {
        __iReturn = clientStart( _poSgctpHubClient );
        if( __iReturn )
        {

          // ... lock client deletion
          pthread_mutex_lock( &tClientDelete_mutex );

          // ... delete client
          delete _poSgctpHubClient;
          poSgctpHubClient_umap.erase( _iSocket );
          close( _iSocket );
          FD_CLR( _iSocket, &sdClient_fdset );
          if( _iSocket == sdClient_max )
            for( int __j=sdClient_max-1; __j>=0; __j-- )
              if( FD_ISSET( __j, &sdClient_fdset ) )
              {
                sdClient_max = __j;
                break;
              }

          // ... unlock client deletion
          pthread_mutex_unlock( &tClientDelete_mutex );

        }
      }
      else if( __sID.substr( 0, 4 ) == "#FLT" )
      {
        if( _poSgctpHubClient->bSync )
          break;
        clientFilterDefine( _poSgctpHubClient, __oData );
      }

    }

    if( !_poSgctpHubClient->oTransmit.hasData() )
      break;

  } // Loop through available client data

  // Unset transmission timeout
  _poSgctpHubClient->oTransmit.setTimeout( _iSocket, 0.0 );
}

void CSgctpHub::clientFilterDefine( CSgctpHubClient *_poSgctpHubClient,
                                    const CData &_roData )
{
  string __sID = _roData.getID();
  if( __sID == "#FLT0" )
  {

    // ... references
    _poSgctpHubClient->fdLatitude0 = _roData.getLatitude();
    _poSgctpHubClient->fdLongitude0 = _roData.getLongitude();

  }
  else if( __sID == "#FLT1" )
  {

    // ... 1st limits
    _poSgctpHubClient->fdTime1 = _roData.getTime();
    _poSgctpHubClient->fdLatitude1 = _roData.getLatitude();
    _poSgctpHubClient->fdLongitude1 = _roData.getLongitude();
    _poSgctpHubClient->fdElevation1 = _roData.getElevation();

  }
  else if( __sID == "#FLT2" )
  {

    // ... 2nd limits
    _poSgctpHubClient->fdTime2 = _roData.getTime();
    _poSgctpHubClient->fdLatitude2 = _roData.getLatitude();
    _poSgctpHubClient->fdLongitude2 = _roData.getLongitude();
    _poSgctpHubClient->fdElevation2 = _roData.getElevation();

  }

}

int CSgctpHub::clientStart( CSgctpHubClient *_poSgctpHubClient )
{
  // Compute limits

  // ... time
  _poSgctpHubClient->fdTimeMin = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->fdTimeMax = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->bTimeLimit = false;
  if( CData::isDefined( _poSgctpHubClient->fdTime1 ) )
    _poSgctpHubClient->fdTimeMin = _poSgctpHubClient->fdTime1;
  if( CData::isDefined( _poSgctpHubClient->fdTime2 ) )
    _poSgctpHubClient->fdTimeMax = _poSgctpHubClient->fdTime2;
  if( CData::isDefined( _poSgctpHubClient->fdTimeMin )
      && CData::isDefined( _poSgctpHubClient->fdTimeMax ) )
  {
    _poSgctpHubClient->bTimeLimit = true;
    if( _poSgctpHubClient->fdTimeMin
        > _poSgctpHubClient->fdTimeMax )
    {
      double __fdValue = _poSgctpHubClient->fdTimeMin;
      _poSgctpHubClient->fdTimeMin = _poSgctpHubClient->fdTimeMax;
      _poSgctpHubClient->fdTimeMax = __fdValue;
    }
    if( _poSgctpHubClient->fdTimeMax - _poSgctpHubClient->fdTimeMin < 1.0 )
    {
      pthread_mutex_lock( &tLog_mutex );
      SGCTP_LOG << SGCTP_NOTICE << "Invalid client filter (null time range)"
                << "; ip=" << _poSgctpHubClient->sIP
                << ", id=" << to_string( _poSgctpHubClient->oTransmit.usePrincipal()->getID() )
                << endl;
      pthread_mutex_unlock( &tLog_mutex );
      return -EINVAL;
    }
  }
  else
  {
    _poSgctpHubClient->fdTimeMin = CData::UNDEFINED_VALUE;
    _poSgctpHubClient->fdTimeMax = CData::UNDEFINED_VALUE;
  }

  // ... distance/azimuth
  _poSgctpHubClient->fdDistanceMin = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->fdDistanceMax = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->fdAzimuthMin = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->fdAzimuthMax = CData::UNDEFINED_VALUE;
  if( CData::isDefined( _poSgctpHubClient->fdLatitude0 )
      && CData::isDefined( _poSgctpHubClient->fdLongitude0 ) )
  {
    if( CData::isDefined( _poSgctpHubClient->fdLatitude1 )
        && CData::isDefined( _poSgctpHubClient->fdLongitude1 ) )
    {
      _poSgctpHubClient->fdDistanceMin =
        distanceRL( _poSgctpHubClient->fdLatitude0,
                    _poSgctpHubClient->fdLongitude0,
                    _poSgctpHubClient->fdLatitude1,
                    _poSgctpHubClient->fdLongitude1 );
      _poSgctpHubClient->fdAzimuthMin =
        azimuthRL( _poSgctpHubClient->fdLatitude0,
                   _poSgctpHubClient->fdLongitude0,
                   _poSgctpHubClient->fdLatitude1,
                   _poSgctpHubClient->fdLongitude1 );
    }
    if( CData::isDefined( _poSgctpHubClient->fdLatitude2 )
        && CData::isDefined( _poSgctpHubClient->fdLongitude2 ) )
    {
      _poSgctpHubClient->fdDistanceMax =
        distanceRL( _poSgctpHubClient->fdLatitude0,
                    _poSgctpHubClient->fdLongitude0,
                    _poSgctpHubClient->fdLatitude2,
                    _poSgctpHubClient->fdLongitude2 );
      _poSgctpHubClient->fdAzimuthMax =
        azimuthRL( _poSgctpHubClient->fdLatitude0,
                   _poSgctpHubClient->fdLongitude0,
                   _poSgctpHubClient->fdLatitude2,
                   _poSgctpHubClient->fdLongitude2 );
    }
  }

  // ... azimuth
  _poSgctpHubClient->bAzimuthLimit = false;
  if( CData::isDefined( _poSgctpHubClient->fdAzimuthMin )
      && CData::isDefined( _poSgctpHubClient->fdAzimuthMax ) )
  {
    _poSgctpHubClient->bAzimuthLimit = true;
    if( _poSgctpHubClient->fdAzimuthMin
        > _poSgctpHubClient->fdAzimuthMax )
    {
      double __fdValue = _poSgctpHubClient->fdAzimuthMin;
      _poSgctpHubClient->fdAzimuthMin = _poSgctpHubClient->fdAzimuthMax;
      _poSgctpHubClient->fdAzimuthMax = __fdValue;
    }
    if( ( _poSgctpHubClient->fdAzimuthMax
          - _poSgctpHubClient->fdAzimuthMin ) < 1.0 ) // too-small a sector = 360-degree sector
    {
      _poSgctpHubClient->fdAzimuthMin = CData::UNDEFINED_VALUE;
      _poSgctpHubClient->fdAzimuthMax = CData::UNDEFINED_VALUE;
      _poSgctpHubClient->bAzimuthLimit = false;
    }
  }
  else
  {
    _poSgctpHubClient->fdAzimuthMin = CData::UNDEFINED_VALUE;
    _poSgctpHubClient->fdAzimuthMax = CData::UNDEFINED_VALUE;
  }

  // ... distance
  _poSgctpHubClient->bDistanceLimit = false;
  if( CData::isDefined( _poSgctpHubClient->fdDistanceMin )
      || CData::isDefined( _poSgctpHubClient->fdDistanceMax ) )
  {
    _poSgctpHubClient->bDistanceLimit = true;
    if( !CData::isDefined( _poSgctpHubClient->fdDistanceMax ) )
    {
      _poSgctpHubClient->fdDistanceMax = _poSgctpHubClient->fdDistanceMin;
      _poSgctpHubClient->fdDistanceMin = CData::UNDEFINED_VALUE;
    }
    if( CData::isDefined( _poSgctpHubClient->fdDistanceMin ) )
    {
      if( _poSgctpHubClient->fdDistanceMin
          > _poSgctpHubClient->fdDistanceMax )
      {
        double __fdValue = _poSgctpHubClient->fdDistanceMin;
        _poSgctpHubClient->fdDistanceMin = _poSgctpHubClient->fdDistanceMax;
        _poSgctpHubClient->fdDistanceMax = __fdValue;
      }
      if( ( _poSgctpHubClient->fdDistanceMax
            - _poSgctpHubClient->fdDistanceMin ) < 1.0 )
      {
        if( _poSgctpHubClient->bAzimuthLimit )
        {
          _poSgctpHubClient->fdDistanceMin = CData::UNDEFINED_VALUE;
          _poSgctpHubClient->fdDistanceMax = CData::UNDEFINED_VALUE;
          _poSgctpHubClient->bDistanceLimit = false;
        }
        else
        {
          pthread_mutex_lock( &tLog_mutex );
          SGCTP_LOG << SGCTP_NOTICE << "Invalid client filter (null distance range)"
                    << "; ip=" << _poSgctpHubClient->sIP
                    << ", id=" << to_string( _poSgctpHubClient->oTransmit.usePrincipal()->getID() )
                    << endl;
          pthread_mutex_unlock( &tLog_mutex );
          return -EINVAL;
        }
      }
    }
  }

  // ... elevation
  _poSgctpHubClient->fdElevationMin = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->fdElevationMax = CData::UNDEFINED_VALUE;
  _poSgctpHubClient->bElevationLimit = false;
  if( CData::isDefined( _poSgctpHubClient->fdElevation1 ) )
    _poSgctpHubClient->fdElevationMin = _poSgctpHubClient->fdElevation1;
  if( CData::isDefined( _poSgctpHubClient->fdElevation2 ) )
    _poSgctpHubClient->fdElevationMax = _poSgctpHubClient->fdElevation2;
  if( CData::isDefined( _poSgctpHubClient->fdElevationMin )
      || CData::isDefined( _poSgctpHubClient->fdElevationMax ) )
  {
    _poSgctpHubClient->bElevationLimit = true;
    if( !CData::isDefined( _poSgctpHubClient->fdElevationMax ) )
    {
      _poSgctpHubClient->fdElevationMax = _poSgctpHubClient->fdElevationMin;
      _poSgctpHubClient->fdElevationMin = CData::UNDEFINED_VALUE;
    }
    if( CData::isDefined( _poSgctpHubClient->fdElevationMin ) )
    {
      if( _poSgctpHubClient->fdElevationMin
          > _poSgctpHubClient->fdElevationMax )
      {
        double __fdValue = _poSgctpHubClient->fdElevationMin;
        _poSgctpHubClient->fdElevationMin = _poSgctpHubClient->fdElevationMax;
        _poSgctpHubClient->fdElevationMax = __fdValue;
      }
      if( ( _poSgctpHubClient->fdElevationMax
            - _poSgctpHubClient->fdElevationMin ) < 1.0 )
      {
        pthread_mutex_lock( &tLog_mutex );
        SGCTP_LOG << SGCTP_NOTICE << "Invalid client filter (null elevation range)"
                  << "; ip=" << _poSgctpHubClient->sIP
                  << ", id=" << to_string( _poSgctpHubClient->oTransmit.usePrincipal()->getID() )
                  << endl;
        pthread_mutex_unlock( &tLog_mutex );
        return -EINVAL;
      }
    }
  }

  // Start synchronization
  _poSgctpHubClient->bSync = true;
  return 0;
}

bool CSgctpHub::clientFilterCheck( const CSgctpHubClient *_poSgctpHubClient,
                                   const CData &_roData )
{
  // Check limits

  // ... time
  if( _poSgctpHubClient->bTimeLimit )
  {
    double __fdTime = _roData.getTime();
    if( CData::isDefined( _poSgctpHubClient->fdTimeMin )
        && __fdTime < _poSgctpHubClient->fdTimeMin )
      return false;
    if( CData::isDefined( _poSgctpHubClient->fdTimeMax )
        && __fdTime > _poSgctpHubClient->fdTimeMax )
      return false;
  }

  // ... fix
  if( !_poSgctpHubClient->bDistanceLimit
      && !_poSgctpHubClient->bAzimuthLimit
      && !_poSgctpHubClient->bElevationLimit )
    return true;

  double __fdLatitude = _roData.getLatitude();
  double __fdLongitude = _roData.getLongitude();
  double __fdElevation = _roData.getElevation();
  bool __b2D =
    CData::isDefined( __fdLatitude )
    && CData::isDefined( __fdLongitude );
  bool __b3D =
    __b2D
    && CData::isDefined( __fdElevation );

  // ... distance
  if( _poSgctpHubClient->bDistanceLimit && __b2D )
  {
    double __fdDistance = distanceRL( _poSgctpHubClient->fdLatitude0,
                                      _poSgctpHubClient->fdLongitude0,
                                      __fdLatitude,
                                      __fdLongitude );
    if( CData::isDefined( _poSgctpHubClient->fdDistanceMin )
        && __fdDistance < _poSgctpHubClient->fdDistanceMin )
      return false;
    if( CData::isDefined( _poSgctpHubClient->fdDistanceMax )
        && __fdDistance > _poSgctpHubClient->fdDistanceMax )
      return false;
  }

  // ... azimuth
  if( _poSgctpHubClient->bAzimuthLimit && __b2D )
  {
    double __fdAzimuth = azimuthRL( _poSgctpHubClient->fdLatitude0,
                                    _poSgctpHubClient->fdLongitude0,
                                    __fdLatitude,
                                    __fdLongitude );
    if( CData::isDefined( _poSgctpHubClient->fdAzimuthMin )
        && __fdAzimuth < _poSgctpHubClient->fdAzimuthMin )
      return false;
    if( CData::isDefined( _poSgctpHubClient->fdAzimuthMax )
        && __fdAzimuth > _poSgctpHubClient->fdAzimuthMax )
      return false;
  }

  // ... elevation
  if( _poSgctpHubClient->bElevationLimit && __b3D )
  {
    if( CData::isDefined( _poSgctpHubClient->fdElevationMin )
        && __fdElevation < _poSgctpHubClient->fdElevationMin )
      return false;
    if( CData::isDefined( _poSgctpHubClient->fdElevationMax )
        && __fdElevation > _poSgctpHubClient->fdElevationMax )
      return false;
  }

  // Done
  return true;
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpHub __oSgctpHub( argc, argv );
  return __oSgctpHub.exec();
}
