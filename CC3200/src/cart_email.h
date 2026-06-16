//*****************************************************************************
// cart_email.h - Wi-Fi email alerts via AWS IoT shadow (TLS) -> SNS.
// Compiled out unless ENABLE_WIFI_EMAIL == 1 in hw_config.h. Calls are blocking.
//*****************************************************************************
#ifndef __CART_EMAIL_H__
#define __CART_EMAIL_H__

#ifdef __cplusplus
extern "C" {
#endif

//! Bring up Wi-Fi and set the clock (needed for TLS). \return 0 on success,
//! <0 on failure. No-op returning -1 when ENABLE_WIFI_EMAIL == 0.
extern int Email_Init(void);

//! \return 1 if email can be sent, else 0.
extern int Email_IsReady(void);

//! Send one alert. \return 0 on success, <0 on failure.
extern int Email_Send(const char *pcMessage);

#ifdef __cplusplus
}
#endif
#endif // __CART_EMAIL_H__
