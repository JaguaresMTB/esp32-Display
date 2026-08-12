// Weather configuration template.
//
// NOTE: the weather provider now uses Open-Meteo, which requires NO API key
// and NO response language parameter. OPENWEATHER_API_KEY and WEATHER_LANG
// below are LEGACY (unused by the firmware) and kept only for reference.
// Location values remain the fallback used when GeoIP location is unavailable.
//
// To configure:
//   1. Copy this file to  src/config/weather_credentials.h
//   2. Adjust the location (lat/lon/name) if needed
//   3. Build and upload
//
// src/config/weather_credentials.h is listed in .gitignore and must NEVER be
// committed. Only this .example.h file is tracked.
#pragma once

#ifndef OPENWEATHER_API_KEY // LEGACY (unused with Open-Meteo)
#define OPENWEATHER_API_KEY ""
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

// Local timezone offset for the configured location (UTC hours; Mérida,
// Mexico is UTC-6, no DST). Used as the initial/fallback "last update" offset.
#ifndef WEATHER_TIMEZONE_OFFSET_HOURS
#define WEATHER_TIMEZONE_OFFSET_HOURS -6
#endif

// LEGACY (unused with Open-Meteo; descriptions are derived locally from the
// WMO code and the UI language).
#ifndef WEATHER_LANG
#define WEATHER_LANG ""
#endif

// Weather screen UI language: 0 = English, 1 = Spanish.
#ifndef WEATHER_UI_LANG
#define WEATHER_UI_LANG 1
#endif
