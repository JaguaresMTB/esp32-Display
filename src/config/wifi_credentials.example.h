// LEGACY — superseded by on-device provisioning (Sprint 8).
//
// Wi-Fi credentials are now configured through the provisioning portal
// (SoftAP "WeatherDisplay-XXXX" + http://192.168.4.1) and stored in NVS.
// The firmware no longer reads this file at runtime.
//
// This file is kept only as a historical record of the old development
// mechanism. src/config/wifi_credentials.h (gitignored) is unused.
//
// The password is never printed by the firmware.
#pragma once

#ifndef WIFI_SSID
#define WIFI_SSID "your_wifi_ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your_wifi_password"
#endif
