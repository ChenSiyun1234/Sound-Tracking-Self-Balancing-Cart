/*
 * network_utils.h
 *
 * Wi-Fi connect + TLS socket helpers and SimpleLink async event handlers.
 * Used when ENABLE_WIFI_EMAIL == 1.
 */

#ifndef UTILS_NETWORK_UTILS_H_
#define UTILS_NETWORK_UTILS_H_

// Simplelink includes
#include "simplelink.h"

#include "utils.h"
#include "common.h"

#define MAX_URI_SIZE 128
#define URI_SIZE MAX_URI_SIZE + 1

// Load as user files (not system files) when flashing.
#define SL_SSL_CA_CERT "/cert/rootCA.der" // Starfield Class2 root CA
#define SL_SSL_PRIVATE "/cert/private.der"
#define SL_SSL_CLIENT  "/cert/client.der"

// globals
extern volatile unsigned long  g_ulStatus;//SimpleLink Status
extern unsigned long  g_ulPingPacketsRecv; //Number of Ping Packets received
extern unsigned long  g_ulGatewayIP; //Network Gateway IP address
extern unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
extern unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

extern signed char * g_Host;
extern int g_port;

typedef struct SlDateTime {
   /* time */
   unsigned long tm_sec;
   unsigned long tm_min;
   unsigned long tm_hour;
   /* date */
   unsigned long tm_day;
   unsigned long tm_mon;
   unsigned long tm_year;
   unsigned long tm_week_day; //not required
   unsigned long tm_year_day; //not required
   unsigned long reserved[3];
} SlDateTime;

extern SlDateTime g_time;

typedef struct SlAppConfig {
    signed char * host;
    int port;
} SlAppConfig_t;

extern SlAppConfig_t g_app_config;

// Application specific status/error codes
typedef enum {
    // -0x7D0 base avoids overlap with host-driver error codes
    LAN_CONNECTION_FAILED = -0x7D0,
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;


void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent);

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse);

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);

int tls_connect();

int connectToAccessPoint();

#endif /* UTILS_NETWORK_UTILS_H_ */
