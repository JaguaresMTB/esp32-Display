#pragma once

#include <Arduino.h>

namespace networking
{
  enum class State
  {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
  };

  // Sub-stage during a connection attempt, for the boot checklist display.
  enum class ConnectStage
  {
    None,
    Connecting,
    Authorizing,
    Connected,
  };

  // Wi-Fi abstraction. Application code depends only on this interface and
  // never on the Arduino WiFi API (WiFi.h). Implemented in network.cpp.
  class INetwork
  {
  public:
    virtual ~INetwork() = default;

    // Begin connecting to the configured Wi-Fi network (non-blocking).
    virtual void begin() = 0;

    // Advance the connection state machine. Call frequently from the loop.
    virtual void update() = 0;

    virtual bool isConnected() const = 0;
    virtual State state() const = 0;
    virtual const char* stateName() const = 0;

    virtual String ssid() const = 0;
    virtual String localIp() const = 0;
    virtual int16_t rssi() const = 0;

    // Consecutive failed connection attempts (0 on the first attempt).
    virtual int retryCount() const = 0;

    // The configured (target) SSID, even before a connection is established.
    virtual const char* configuredSsid() const = 0;

    // Progress sub-stage during attempts (for the boot checklist).
    virtual ConnectStage connectStage() const = 0;
  };

  // Factory: returns the singleton network instance.
  INetwork& getNetwork();
}
