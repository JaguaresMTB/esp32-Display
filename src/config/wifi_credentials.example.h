// Wi-Fi credentials template.
//
// To provide real credentials:
//   1. Copy this file to  src/config/wifi_credentials.h
//   2. Fill in your SSID and password
//   3. Build and upload
//
// src/config/wifi_credentials.h is listed in .gitignore and must NEVER be
// committed. Only this .example.h file is tracked.
//
// The password is never printed by the firmware.
#pragma once

#ifndef WIFI_SSID
#define WIFI_SSID "your_wifi_ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your_wifi_password"
#endif
