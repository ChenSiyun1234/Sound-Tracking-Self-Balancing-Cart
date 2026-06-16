/*
 * network_utils.c  (from aws-rest-api-ssl-demo)
 *
 * Wi-Fi connect + TLS + SimpleLink async event handlers.
 * Compiled only when ENABLE_WIFI_EMAIL == 1.
 */
#include "hw_config.h"
#if ENABLE_WIFI_EMAIL

#include "network_utils.h"

// stdlib includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "uart.h"

//Common interface includes
#include "gpio_if.h"
#include "uart_if.h"

// GPIO9 (PIN_64) is the motor PWM, so neutralize the LED helpers here.
#undef  GPIO_IF_LedOn
#undef  GPIO_IF_LedOff
#undef  GPIO_IF_LedConfigure
#define GPIO_IF_LedOn(x)         ((void)0)
#define GPIO_IF_LedOff(x)        ((void)0)
#define GPIO_IF_LedConfigure(x)  ((void)0)


// globals

volatile unsigned long  g_ulStatus = 0; //SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
unsigned long  g_ulGatewayIP = 0;       //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID

signed char * g_Host;
int g_port;

SlDateTime g_time;
SlAppConfig_t g_app_config;

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

// Provisioning event hook stubs required by simplelink.a; pass events through.
_SlEventPropogationStatus_e sl_Provisioning_HttpServerEventHdl(
                            SlHttpServerEvent_t    *apSlHttpServerEvent,
                            SlHttpServerResponse_t *apSlHttpServerResponse)
{
    return EVENT_PROPAGATION_CONTINUE;
}

_SlEventPropogationStatus_e sl_Provisioning_NetAppEventHdl(SlNetAppEvent_t *apNetAppEvent)
{
    return EVENT_PROPAGATION_CONTINUE;
}

_SlEventPropogationStatus_e sl_Provisioning_WlanEventHdl(SlWlanEvent_t *apEventInfo)
{
    return EVENT_PROPAGATION_CONTINUE;
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) {
    if(!pWlanEvent) {
        return;
    }

    switch(pWlanEvent->Event) {
        case SL_WLAN_CONNECT_EVENT: {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);
            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
                       "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                       g_ucConnectionSSID,g_ucConnectionBSSID[0],
                       g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                       g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                       g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT: {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);
            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code) {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                    "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s, "
                           "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        default: {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent) {
    if(!pNetAppEvent) {
        return;
    }

    switch(pNetAppEvent->Event) {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT: {
            SlIpV4AcquiredAsync_t *pEventData = NULL;
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_ulGatewayIP = pEventData->gateway;
            UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                       "Gateway=%d.%d.%d.%d\n\r",
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
        }
        break;

        default: {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse) {
    // Unused in this application
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
    if(!pDevEvent) {
        return;
    }
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->EventData.deviceEvent.status,
               pDevEvent->EventData.deviceEvent.sender);
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) {
    if(!pSock) {
        return;
    }
    switch( pSock->Event ) {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->socketAsyncEvent.SockTxFailData.status) {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                                "failed to transmit all queued packets\n\n",
                                    pSock->socketAsyncEvent.SockTxFailData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED  :  socket %d , reason "
                                "(%d) \n\n",
                                pSock->socketAsyncEvent.SockTxFailData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
                  break;
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
          break;
    }
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

static long InitializeAppVariables() {
    g_ulStatus = 0;
    g_ulGatewayIP = 0;
    g_Host = g_app_config.host;
    g_port = g_app_config.port;
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    return SUCCESS;
}

static long ConfigureSimpleLinkToDefaultState() {
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    if (ROLE_STA != lMode) {
        if (ROLE_AP == lMode) {
            while(!IS_IP_ACQUIRED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
            }
        }
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);
        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);
        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);
        if (ROLE_STA != lRetVal) {
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }

    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal) {
        while(IS_CONNECTED(g_ulStatus)) {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#endif
        }
    }

    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    ucPower = 0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (unsigned char *)&ucPower);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();
    return lRetVal; // Success
}

static long WlanConnect() {
    SlSecParams_t secParams = {0};
    long lRetVal = 0;
    unsigned long waitCount = 0;

    secParams.Key = SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;

    UART_PRINT("[EMAIL] Connecting to SSID: %s\n\r", SSID_NAME);
    lRetVal = sl_WlanConnect(SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(lRetVal);

    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus))) {
        _SlNonOsMainLoopTask();
        MAP_UtilsDelay(800000);
        _SlNonOsMainLoopTask();
        MAP_UtilsDelay(800000);

        waitCount++;
        if((waitCount % 10) == 0) {
            UART_PRINT("[EMAIL] waiting WiFi/IP count=%lu connected=%d ip=%d\n\r",
                       waitCount, IS_CONNECTED(g_ulStatus), IS_IP_ACQUIRED(g_ulStatus));
        }
        if(waitCount >= 300) {
            UART_PRINT("[EMAIL] Timed out waiting for WiFi/IP. Check SSID/password.\n\r");
            return LAN_CONNECTION_FAILED;
        }
    }
    UART_PRINT("[EMAIL] WiFi connected and IP acquired.\n\r");
    return SUCCESS;
}

static long printErrConvenience(char * msg, long retVal) {
    UART_PRINT(msg);
    return retVal;
}

int tls_connect() {
    SlSockAddrIn_t    Addr;
    int    iAddrSize;
    unsigned char    ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiIP;
    unsigned int uiCipher = SL_SEC_MASK_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;
    long lRetVal = -1;
    int iSockID;

    UART_PRINT("[DBG TLS 1] DNS lookup host=%s\n\r", g_Host);
    lRetVal = sl_NetAppDnsGetHostByName(g_Host, strlen((const char *)g_Host),
                                    (unsigned long*)&uiIP, SL_AF_INET);
    UART_PRINT("[DBG TLS 2] DNS returned %ld\n\r", lRetVal);
    if(lRetVal < 0) {
        return printErrConvenience("[EMAIL] DNS FAILED (no internet?)\n\r", lRetVal);
    }
    UART_PRINT("[DBG TLS 3] server IP = %d.%d.%d.%d\n\r",
               SL_IPV4_BYTE(uiIP,3), SL_IPV4_BYTE(uiIP,2),
               SL_IPV4_BYTE(uiIP,1), SL_IPV4_BYTE(uiIP,0));

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(g_port);
    Addr.sin_addr.s_addr = sl_Htonl(uiIP);
    iAddrSize = sizeof(SlSockAddrIn_t);

    UART_PRINT("[DBG TLS 4] creating secure socket...\n\r");
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    UART_PRINT("[DBG TLS 5] sl_Socket returned %d\n\r", iSockID);
    if( iSockID < 0 ) {
        return printErrConvenience("[EMAIL] secure socket FAILED\n\r", iSockID);
    }

    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,
                               sizeof(ucMethod));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }
    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,
                           sizeof(uiCipher));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET,
                           SL_SO_SECURE_FILES_CA_FILE_NAME,
                           SL_SSL_CA_CERT,
                           strlen(SL_SSL_CA_CERT));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET,
                SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                                    SL_SSL_CLIENT,
                           strlen(SL_SSL_CLIENT));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET,
            SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
            SL_SSL_PRIVATE,
                           strlen(SL_SSL_PRIVATE));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set socket options \n\r", lRetVal);
    }

    lRetVal = sl_SetSockOpt(iSockID, SL_SOL_SOCKET,
                            SL_SO_SECURE_DOMAIN_NAME_VERIFICATION,
                            g_Host,
                            strlen((const char *)g_Host));
    if(lRetVal < 0) {
        return printErrConvenience("Device couldn't set domain name verification / SNI option \n\r", lRetVal);
    }

    UART_PRINT("[DBG TLS 6] certs set, starting TLS handshake (sl_Connect)...\n\r");
    lRetVal = sl_Connect(iSockID, ( SlSockAddr_t *)&Addr, iAddrSize);
    UART_PRINT("[DBG TLS 7] sl_Connect returned %ld\n\r", lRetVal);
    if(lRetVal >= 0) {
        UART_PRINT("[DBG TLS 8] TLS connected to %s\n\r", g_Host);
    }
    else if(lRetVal == SL_ESECSNOVERIFY) {
        UART_PRINT("[DBG TLS 8] TLS connected (server cert UNVERIFIED) to %s\n\r", g_Host);
    }
    else if(lRetVal < 0) {
        // -340 = cert/date mismatch; -461/-462 = file not found (certs not flashed)
        return printErrConvenience("[EMAIL] TLS handshake FAILED (check date / certs)\n\r", lRetVal);
    }

    return iSockID;
}

int connectToAccessPoint() {
    long lRetVal = -1;

    lRetVal = InitializeAppVariables();

    UART_PRINT("[DBG AP 1] ConfigureSimpleLinkToDefaultState()...\n\r");
    lRetVal = ConfigureSimpleLinkToDefaultState();
    UART_PRINT("[DBG AP 2] ConfigureSimpleLinkToDefaultState returned %ld\n\r", lRetVal);
    if(lRetVal < 0) {
      if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
          UART_PRINT("[EMAIL] Failed to configure device in default state\n\r");
      return lRetVal;
    }

    CLR_STATUS_BIT_ALL(g_ulStatus);

    UART_PRINT("[DBG AP 3] sl_Start (station mode)...\n\r");
    lRetVal = sl_Start(0, 0, 0);
    UART_PRINT("[DBG AP 4] sl_Start returned role=%ld (expect 0=STA)\n\r", lRetVal);
    if (lRetVal < 0 || ROLE_STA != lRetVal) {
        UART_PRINT("[EMAIL] Failed to start the device\n\r");
        return lRetVal;
    }

    UART_PRINT("[DBG AP 5] WlanConnect() - joining %s ...\n\r", SSID_NAME);
    lRetVal = WlanConnect();
    UART_PRINT("[DBG AP 6] WlanConnect returned %ld\n\r", lRetVal);
    if(lRetVal < 0) {
        UART_PRINT("[EMAIL] Failed to connect to AP (SSID/key/2.4GHz?)\n\r");
        return lRetVal;
    }

    UART_PRINT("[DBG AP 7] Wi-Fi up, IP acquired.\n\r");
    return 0;
}

#endif /* ENABLE_WIFI_EMAIL */
