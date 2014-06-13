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
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// C++
#include <queue>
#include <string>
#include <unordered_map>
using namespace std;

#ifndef __SGCTP_USE_OPENSSL__

// GCrypt
#include "gcrypt.h"

#endif // NOT __SGCTP_USE_OPENSSL__

// SGCTP
#include "../skeleton.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// EXTERNAL
//----------------------------------------------------------------------

#ifndef __SGCTP_USE_OPENSSL__

// GCrypt multi-threading
GCRY_THREAD_OPTION_PTHREAD_IMPL;

#endif // NOT __SGCTP_USE_OPENSSL__


//----------------------------------------------------------------------
// CLASSES: Helpers
//----------------------------------------------------------------------

// Class pre-definition; see further below for actual definition
class CSgctpHub;

/// Data container
class CSgctpHubData
{
  friend class CSgctpHub;

private:
  /// SGCTP data
  CData oData;
  /// Corresponding/converted (system-readable) UNIX epoch
  double fdEpoch;
  /// Corresponding/converted (system-readable) latitude, in degrees
  double fdLatitude;
  /// Corresponding/converted (system-readable) longitude, in degrees
  double fdLongitude;
  /// Corresponding/converted (system-readable) elevation, in meters
  double fdElevation;

private:
  CSgctpHubData();
};


/// TCP Agent container
class CSgctpHubAgentTCP
{
  friend class CSgctpHub;

private:
  /// SGCTP transmission object
  CTransmit_TCP oTransmit;
  /// Accounting data: IP address
  string sIP;
  /// Accounting data: (received) SGCTP packets
  uint64_t ui64tPackets;
  /// Accounting data: (received) SGCTP bytes
  uint64_t ui64tBytes;
  /// Accounting data: disconnection code
  int iError;

private:
  CSgctpHubAgentTCP();
};


/// Client container
class CSgctpHubClient
{
  friend class CSgctpHub;

  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

private:
  /// SGCTP transmission object
  CTransmit_TCP oTransmit;
  /// Data synchronization status
  bool bSync;
  /// Accounting data: IP address
  string sIP;
  /// Accounting data: (sent) SGCTP packets
  uint64_t ui64tPackets;
  /// Accounting data: (sent) SGCTP bytes
  uint64_t ui64tBytes;
  /// Accounting data: disconnection code
  int iError;

  /// Filter data: reference latitude, in degrees
  double fdLatitude0;
  /// Filter data: reference longitude, in degrees
  double fdLongitude0;
  /// Filter data: 1st time limit, in seconds
  double fdTime1;
  /// Filter data: 1st latitude limit, in degrees
  double fdLatitude1;
  /// Filter data: 1st longitude limit, in degrees
  double fdLongitude1;
  /// Filter data: 1st elevation limit, in meters
  double fdElevation1;
  /// Filter data: 2nd time limit, in seconds
  double fdTime2;
  /// Filter data: 2nd latitude limit, in degrees
  double fdLatitude2;
  /// Filter data: 2nd longitude limit, in degrees
  double fdLongitude2;
  /// Filter data: 2nd elevation limit, in meters
  double fdElevation2;

  /// Filter limits: time limits status
  bool bTimeLimit;
  /// Filter limits: minimum time limit
  double fdTimeMin;
  /// Filter limits: maximum time limit
  double fdTimeMax;
  /// Filter limits: distance limits status
  bool bDistanceLimit;
  /// Filter limits: minimum distance limit
  double fdDistanceMin;
  /// Filter limits: maximum distance limit
  double fdDistanceMax;
  /// Filter limits: azimuth limits status
  bool bAzimuthLimit;
  /// Filter limits: minimum azimuth limit
  double fdAzimuthMin;
  /// Filter limits: maximum azimuth limit
  double fdAzimuthMax;
  /// Filter limits: elevation limits status
  bool bElevationLimit;
  /// Filter limits: minimum elevation limit
  double fdElevationMin;
  /// Filter limits: maximum elevation limit
  double fdElevationMax;

private:
  CSgctpHubClient();
};


//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------


/// Application container
class CSgctpHub: protected CSgctpUtilSkeleton
{

  //----------------------------------------------------------------------
  // STATIC / CONSTANTS
  //----------------------------------------------------------------------

private:
  static pthread_t THREAD_DATA;
  static void* threadData( void* _poSgctpHub );
  static pthread_t THREAD_AGENT_UDP;
  static void* threadAgentUDP( void* _poSgctpHub );
  static pthread_t THREAD_AGENT_TCP;
  static void* threadAgentTCP( void* _poSgctpHub );
  static pthread_t THREAD_CLIENT_RX;
  static void* threadClientRX( void* _poSgctpHub );
  static pthread_t THREAD_CLIENT_TX;
  static void* threadClientTX( void* _poSgctpHub );

private:
  static void* getInAddr( struct sockaddr *_ptSockaddr );

public:
  static void interrupt( int _iSignal );

  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

  //
  // Resources: general
  //

private:
  /// Logging mutex
  pthread_mutex_t tLog_mutex;

  //
  // Resources: data
  //

private:
  /// Internal data map
  unordered_map<string,CSgctpHubData*> poSgctpHubData_umap;
  /// Internal data modification mutex
  pthread_mutex_t tSgctpHubData_mutex;
  /// Pending data (IDs) to synchronize
  queue<string> sSyncID_queue;
  /// Pending data (IDs) modification mutex
  pthread_mutex_t tSyncID_mutex;

  //
  // Resources: UDP agents thread
  //

private:
  /// UDP agents activation status
  bool bAgentUDPEnabled;
  /// UDP agents (listening) socket
  int sdAgentUDP;
  /// UDP agents (listening) socket address info pointer
  struct addrinfo* ptAddrinfo_AgentUDP;
  /// UDP agents transmission object
  CTransmit_UDP oTransmit_AgentUDP;

  //
  // Resources: TCP agents thread
  //

private:
  /// TCP agents (listening) socket
  int sdAgentTCP;
  /// TCP agents (connected) sockets
  fd_set sdAgentTCP_fdset;
  /// TCP agents (connected) maximum socket ID
  int sdAgentTCP_max;
  /// TCP agents (listening) socket address info pointer
  struct addrinfo* ptAddrinfo_AgentTCP;
  /// TCP agents containers (for each connected agent)
  unordered_map<int,CSgctpHubAgentTCP*> poSgctpHubAgentTCP_umap;

  //
  // Resources: (TCP) clients threads
  //

private:
  /// (TCP) clients (listening) socket
  int sdClient;
  /// (TCP) clients (connected) sockets
  fd_set sdClient_fdset;
  /// (TCP) clients (connected) maximum socket ID
  int sdClient_max;
  /// (TCP) clients (listening) socket address info pointer
  struct addrinfo* ptAddrinfo_Client;
  /// (TCP) clients containers (for each connected client)
  unordered_map<int,CSgctpHubClient*> poSgctpHubClient_umap;
  /// (TCP) clients (connected) sockets to be deleted
  fd_set sdClientDelete_fdset;
  /// (TCP) client deletion mutex
  pthread_mutex_t tClientDelete_mutex;
  /// (TCP) client data transmission (TX) mutex
  pthread_mutex_t tClientTX_mutex;
  /// (TCP) client data transmission (TX) condition
  pthread_cond_t tClientTX_cond;

  //
  // Arguments
  //

private:
  /// Input host (IP/name)
  string sInputHost = "localhost";
  /// Input host port, for UDP agents
  string sInputPort_AgentUDP = "8947";
  /// Input host port, for TCP agents
  string sInputPort_AgentTCP = "8947";
  /// Input host port, for (TCP) clients
  string sInputPort_Client = "8948";
  /// Incoming data throttling delay, in seconds
  double fdTimeThrottle;
  /// Incoming data distance threshold, in meters
  double fdDistanceThreshold;
  /// Internal data Time-To-Live, in seconds
  int iDataTTL = 3600;


  //----------------------------------------------------------------------
  // CONSTRUCTORS / DESTRUCTOR
  //----------------------------------------------------------------------

public:
  CSgctpHub( int _iArgC, char *_ppcArgV[] );

public:
  virtual ~CSgctpHub();


  //----------------------------------------------------------------------
  // METHODS: CSgctpUtilSkeleton (implement/override)
  //----------------------------------------------------------------------

private:
  virtual void displayHelp();
  virtual int parseArgs();

public:
  virtual int exec();


  //----------------------------------------------------------------------
  // METHODS
  //----------------------------------------------------------------------

  //
  // General
  //

private:
  /// General initialization function
  int init();

  //
  // Data
  //

private:
  /// Data thread initialization function
  int dataInit();
  /// Data thread (execution) function
  void* dataThread();
  /// Synchronize internal data
  bool dataSync( const CData &_roData );
  /// Clean-up internal data
  void dataCleanup();

  //
  // UDP agents thread
  //

private:
  /// UDP agents thread initialization function
  int agentUDPInit();
  /// UDP agends thread (execution) function
  void* agentUDPThread();

  //
  // TCP agents thread
  //

private:
  /// TCP agents thread initialization function
  int agentTCPInit();
  /// TCP agents thread (execution) function
  void* agentTCPThread();
  /// TCP agent input processing
  void agentTCPInput( int _iSocket,
                      CSgctpHubAgentTCP *_poSgctpHubAgentTCP );

  //
  // (TCP) clients threads
  //

private:
  /// Clients thread initialization function
  int clientInit();
  /// Clients (RX) thread (execution) function
  void* clientRXThread();
  /// Clients (TX) thread (execution) function
  void* clientTXThread();
  /// Client input processing
  void clientInput( int _iSocket,
                    CSgctpHubClient *_poSgctpHubClient );
  /// Defines the client filter
  void clientFilterDefine( CSgctpHubClient *_poSgctpHubClient,
                           const CData &_roData );
  /// Starts the client synchronization
  int clientStart( CSgctpHubClient *_poSgctpHubClient );
  /// Returns whether the given data fits the given client filter
  bool clientFilterCheck( const CSgctpHubClient *_poSgctpHubClient,
                          const CData &_roData );

};
