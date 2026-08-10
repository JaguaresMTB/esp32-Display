// OpenWeather credentials + location template.
//
// To provide real credentials:
//   1. Copy this file to  src/config/weather_credentials.h
//   2. Fill in your OpenWeather API key
//   3. Adjust the location (lat/lon/name) if needed
//   4. Build and upload
//
// src/config/weather_credentials.h is listed in .gitignore and must NEVER be
// committed. Only this .example.h file is tracked.
//
// The API key is never printed by the firmware.
#pragma once

#ifndef OPENWEATHER_API_KEY
#define OPENWEATHER_API_KEY "YOUR_OPENWEATHER_API_KEY"
#endif

#ifndef WEATHER_LOCATION_NAME
#define WEATHER_LOCATION_NAME "Mérida"
#endif

#ifndef WEATHER_LATITUDE
#define WEATHER_LATITUDE 20.9756f
#endif

#ifndef WEATHER_LONGITUDE
#define WEATHER_LONGITUDE -89.6170f
#endif
