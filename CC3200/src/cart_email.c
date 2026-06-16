//*****************************************************************************
// cart_email.c  -  Wi-Fi email alerts (see cart_email.h)
//
// Based on aws-rest-api-ssl-demo networking (network_utils.c). AWS IoT POST to
// a Thing Shadow triggers an IoT rule -> SNS -> email. Gated by
// ENABLE_WIFI_EMAIL; when 0 these are no-op stubs.
//*****************************************************************************
#include "hw_config.h"
#include "cart_email.h"

#if ENABLE_WIFI_EMAIL
#include <stdio.h>
#include <string.h>
#include "simplelink.h"
#include "uart_if.h"
#include "common.h"
#include "network_utils.h"      // connectToAccessPoint(), tls_connect(), g_* globals

// AWS IoT target (no https:// prefix)
#define AWS_IOT_ENDPOINT  "asohmy6nhzwqo-ats.iot.us-east-1.amazonaws.com"
#define THING_NAME        "CC3200_Thing"
#define SHADOW_PATH       "/things/" THING_NAME "/shadow"
#define AWS_TLS_PORT      8443

#define POSTHEADER "POST " SHADOW_PATH " HTTP/1.1\r\n"
#define HOSTHEADER "Host: " AWS_IOT_ENDPOINT "\r\n"
#define CHEADER    "Connection: close\r\n"
#define CTHEADER   "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1  "Content-Length: "
#define CLHEADER2  "\r\n\r\n"

// Board date for TLS certificate validation; must fall within cert validity window.
#define EMAIL_YEAR   2026
#define EMAIL_MONTH  6
#define EMAIL_DAY    4

static int s_iReady = 0;

static int set_time(void)
{
    long lRetVal;
    g_time.tm_day  = EMAIL_DAY;
    g_time.tm_mon  = EMAIL_MONTH;
    g_time.tm_year = EMAIL_YEAR;
    g_time.tm_hour = 12;
    g_time.tm_min  = 0;
    g_time.tm_sec  = 0;
    UART_PRINT("[DBG TIME] setting board date %d-%d-%d (for TLS cert check)\n\r",
               EMAIL_YEAR, EMAIL_MONTH, EMAIL_DAY);
    lRetVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                        SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                        sizeof(SlDateTime), (unsigned char *)(&g_time));
    UART_PRINT("[DBG TIME] sl_DevSet returned %ld\n\r", lRetVal);
    return (lRetVal < 0) ? -1 : 0;
}

// POST {"state":{"desired":{"var":"<msg>"}}} to the Thing Shadow.
static int http_post(int iSock, const char *pcMsg)
{
    char acSend[768], acRecv[600], acData[256], acLen[16];
    char *p = acSend;
    int  n;

    sprintf(acData, "{\"state\":{\"desired\":{\"var\":\"%s\"}}}\r\n\r\n", pcMsg);

    strcpy(p, POSTHEADER); p += strlen(POSTHEADER);
    strcpy(p, HOSTHEADER); p += strlen(HOSTHEADER);
    strcpy(p, CHEADER);    p += strlen(CHEADER);
    strcpy(p, CTHEADER);   p += strlen(CTHEADER);
    strcpy(p, CLHEADER1);  p += strlen(CLHEADER1);
    sprintf(acLen, "%d", (int)strlen(acData));
    strcpy(p, acLen);      p += strlen(acLen);
    strcpy(p, CLHEADER2);  p += strlen(CLHEADER2);
    strcpy(p, acData);     p += strlen(acData);

    UART_PRINT("[DBG POST 1] sock=%d, request (%d bytes):\n\r", iSock, (int)strlen(acSend));
    UART_PRINT("%s\n\r", acSend);

    UART_PRINT("[DBG POST 2] sl_Send...\n\r");
    n = sl_Send(iSock, acSend, strlen(acSend), 0);
    UART_PRINT("[DBG POST 3] sl_Send returned %d\n\r", n);
    if (n < 0) { UART_PRINT("[EMAIL] POST sl_Send FAILED %d\n\r", n); return -1; }

    UART_PRINT("[DBG POST 4] waiting for AWS response (sl_Recv)...\n\r");
    n = sl_Recv(iSock, acRecv, sizeof(acRecv) - 1, 0);
    UART_PRINT("[DBG POST 5] sl_Recv returned %d\n\r", n);
    if (n < 0) { UART_PRINT("[EMAIL] POST sl_Recv FAILED %d\n\r", n); return -1; }

    acRecv[n] = '\0';
    UART_PRINT("[DBG POST 6] AWS response (look for 'HTTP/1.1 200'):\n\r");
    UART_PRINT("%s\n\r", acRecv);
    UART_PRINT("[DBG POST 7] (200 = AWS accepted. If 200 but no mail -> the AWS "
               "IoT rule / SNS subscription is the issue, not the board.)\n\r");
    return 0;
}

int Email_Init(void)
{
    long lRetVal;

    g_app_config.host = (signed char *)AWS_IOT_ENDPOINT;
    g_app_config.port = AWS_TLS_PORT;

    UART_PRINT("\n\r========== EMAIL INIT ==========\n\r");
    UART_PRINT("[DBG EMAIL 1] endpoint=%s port=%d\n\r", AWS_IOT_ENDPOINT, AWS_TLS_PORT);
    UART_PRINT("[DBG EMAIL 2] connectToAccessPoint() - joining Wi-Fi (~10-20 s)...\n\r");
    lRetVal = connectToAccessPoint();
    UART_PRINT("[DBG EMAIL 3] connectToAccessPoint returned %ld\n\r", lRetVal);
    if (lRetVal < 0) {
        UART_PRINT("[EMAIL] Wi-Fi connect FAILED (%ld) - email disabled. Check SSID/"
                   "key in common.h, 2.4GHz, hotspot on.\n\r", lRetVal);
        return -1;
    }
    UART_PRINT("[DBG EMAIL 4] set_time()...\n\r");
    if (set_time() < 0) {
        UART_PRINT("[EMAIL] set_time FAILED - email disabled.\n\r");
        return -1;
    }
    s_iReady = 1;
    UART_PRINT("[DBG EMAIL 5] Wi-Fi + clock ready -> alerts ENABLED.\n\r");
    UART_PRINT("================================\n\r");
    return 0;
}

int Email_IsReady(void) { return s_iReady; }

int Email_Send(const char *pcMessage)
{
    int iSock, iRet;

    if (!s_iReady) { UART_PRINT("[EMAIL] not ready (Wi-Fi down) - skip send\n\r"); return -1; }

    UART_PRINT("\n\r[DBG SEND 1] message: \"%s\"\n\r", pcMessage);
    UART_PRINT("[DBG SEND 2] tls_connect() to AWS IoT...\n\r");
    iSock = tls_connect();
    UART_PRINT("[DBG SEND 3] tls_connect returned socket=%d\n\r", iSock);
    if (iSock < 0) {
        UART_PRINT("[EMAIL] tls_connect FAILED (%d) - common: -340 = wrong board "
                   "date, or certs not flashed to /cert/ as user files.\n\r", iSock);
        return -1;
    }
    iRet = http_post(iSock, pcMessage);
    sl_Close(iSock);
    UART_PRINT("[EMAIL] RESULT: '%s' -> %s\n\r", pcMessage, (iRet == 0) ? "SENT OK" : "FAILED");
    return iRet;
}

#else
// Feature disabled: no-op stubs.
int Email_Init(void)                  { return -1; }
int Email_IsReady(void)               { return 0;  }
int Email_Send(const char *pcMessage) { (void)pcMessage; return -1; }
#endif
