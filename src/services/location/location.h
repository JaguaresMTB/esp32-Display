#pragma once

#include <Arduino.h>

namespace location
{
  // Provider-independent location. Resolved from the current Wi-Fi
  // connection (GeoIP via the router's public IP), cached per SSID.
  struct Location
  {
    String name;
    float latitude = 0.0f;
    float longitude = 0.0f;
    int32_t utcOffsetSeconds = 0; // local timezone offset for "last update"
  };

  class ILocationService
  {
  public:
    virtual ~ILocationService() = default;

    // Open the per-SSID cache (NVS).
    virtual bool begin() = 0;

    // Resolve the location for the currently connected network. Uses the
    // per-SSID cache first (instant); on a cache miss it performs a GeoIP
    // lookup and caches the result. Returns false if unavailable.
    virtual bool resolve(Location& out) = 0;
  };

  ILocationService& getLocationService();
}
