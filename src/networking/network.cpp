#include "network.h"

#include <Arduino.h>
#include <WiFi.h>

#include "common/logging.h"

// Real credentials come from src/config/wifi_credentials.h (gitignored).
// Without that file the build still succeeds with empty credentials.
#if __has_include("config/wifi_credentials.h")
#include "config/wifi_credentials.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace networking
{
namespace
{
  const char* TAG = "NET";

  const unsigned long CONNECT_TIMEOUT_MS = 15000;
  const unsigned long BACKOFF_BASE_MS = 5000;
  const unsigned long BACKOFF_MAX_MS = 60000;
  const int BACKOFF_MAX_SHIFT = 6;

  const char* stateToString(State s)
  {
    switch (s)
    {
      case State::Disconnected: return "DISCONNECTED";
      case State::Connecting:   return "CONNECTING";
      case State::Connected:    return "CONNECTED";
      case State::Reconnecting: return "RECONNECTING";
    }
    return "UNKNOWN";
  }

  class NetworkImpl final : public INetwork
  {
  public:
    void begin() override
    {
      logging::info(TAG, "initialization");
      if (WIFI_SSID[0] == '\0')
      {
        logging::info(TAG, "no credentials configured; skipping connection");
        _state = State::Disconnected;
        return;
      }
      _retries = 0;
      startConnect(State::Connecting);
    }

    void update() override
    {
      switch (_state)
      {
        case State::Disconnected:
        case State::Reconnecting:
          if (WIFI_SSID[0] != '\0' && millis() >= _nextAttemptAt)
          {
            startConnect(_state == State::Reconnecting ? State::Reconnecting : State::Connecting);
          }
          break;

        case State::Connecting:
          handleAttempt();
          break;

        case State::Connected:
          if (WiFi.status() != WL_CONNECTED)
          {
            logging::info(TAG, "disconnected");
            _retries = 0;
            scheduleRetry(State::Reconnecting);
          }
          break;
      }
    }

    bool isConnected() const override { return _state == State::Connected; }

    State state() const override { return _state; }
    const char* stateName() const override { return stateToString(_state); }

    String ssid() const override { return WiFi.SSID(); }
    String localIp() const override { return WiFi.localIP().toString(); }
    int16_t rssi() const override { return WiFi.RSSI(); }

  private:
    State _state = State::Disconnected;
    unsigned long _attemptStart = 0;
    unsigned long _nextAttemptAt = 0;
    int _retries = 0;

    void startConnect(State next)
    {
      _state = State::Connecting;
      _attemptStart = millis();
      logging::info(TAG, "%s (retry=%d)", stateToString(next), _retries);
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }

    void handleAttempt()
    {
      wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED)
      {
        _state = State::Connected;
        _retries = 0;
        logging::info(TAG, "connected");
        logging::info(TAG, "ssid=%s ip=%s rssi=%d dBm",
                      ssid().c_str(), localIp().c_str(), (int)rssi());
        return;
      }

      if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL)
      {
        logging::info(TAG, "connect failed (status=%d)", (int)status);
        scheduleRetry(State::Disconnected);
        return;
      }

      if (millis() - _attemptStart >= CONNECT_TIMEOUT_MS)
      {
        logging::info(TAG, "connect timeout");
        scheduleRetry(State::Disconnected);
      }
    }

    void scheduleRetry(State next)
    {
      unsigned long backoff = BACKOFF_BASE_MS * (1UL << min(_retries, BACKOFF_MAX_SHIFT));
      if (backoff > BACKOFF_MAX_MS)
      {
        backoff = BACKOFF_MAX_MS;
      }
      _retries = min(_retries + 1, BACKOFF_MAX_SHIFT + 1);
      _nextAttemptAt = millis() + backoff;
      _state = next;
      logging::info(TAG, "%s in %lu s", stateToString(next), backoff / 1000);
    }
  };
}

INetwork& getNetwork()
{
  static NetworkImpl instance;
  return instance;
}
}
