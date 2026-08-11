#include "network.h"

#include <Arduino.h>
#include <WiFi.h>

#include "common/error_log.h"
#include "common/logging.h"
#include "networking/provisioning.h"
#include "networking/wifi_credentials.h"

namespace networking
{
namespace
{
  const char* TAG = "NET";

  const unsigned long CONNECT_TIMEOUT_MS = 15000;
  const unsigned long BACKOFF_BASE_MS = 5000;
  const unsigned long BACKOFF_MAX_MS = 60000;
  const int BACKOFF_MAX_SHIFT = 6;
  const unsigned long AUTHORIZE_DELAY_MS = 3000;

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

  String buildApSsid()
  {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[32];
    snprintf(buf, sizeof(buf), "WeatherDisplay-%02X%02X", mac[4], mac[5]);
    return String(buf);
  }

  class NetworkImpl final : public INetwork
  {
  public:
    NetworkImpl() : _provisioning(_store) {}

    void begin() override
    {
      logging::info(TAG, "initialization");
      _store.begin();

      if (!_store.load(_creds))
      {
        logging::info(TAG, "no stored credentials; entering provisioning mode");
        enterProvisioningMode();
        return;
      }

      _mode = Mode::Normal;
      _retries = 0;
      startConnect(State::Connecting);
    }

    void update() override
    {
      if (_mode == Mode::Provisioning)
      {
        _provisioning.update();

        if (_provisioning.takeCompletion())
        {
          _store.load(_creds);
          _mode = Mode::Normal;
          _retries = 0;
          startConnect(State::Connecting);
        }
        return;
      }

      switch (_state)
      {
        case State::Disconnected:
        case State::Reconnecting:
          if (!_creds.ssid.isEmpty() && millis() >= _nextAttemptAt)
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
            errorlog::record("wifi_disconnected");
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
    int retryCount() const override { return _retries; }
    const char* configuredSsid() const override { return _creds.ssid.c_str(); }

    ConnectStage connectStage() const override
    {
      switch (_state)
      {
        case State::Connected:
          return ConnectStage::Connected;
        case State::Connecting:
          return (millis() - _attemptStart) < AUTHORIZE_DELAY_MS ? ConnectStage::Connecting
                                                                 : ConnectStage::Authorizing;
        default:
          return ConnectStage::None;
      }
    }

    void enterProvisioningMode() override
    {
      if (_mode == Mode::Provisioning)
      {
        return;
      }
      logging::info(TAG, "entering provisioning mode");
      errorlog::record("provisioning_started");
      WiFi.disconnect();
      _mode = Mode::Provisioning;
      _state = State::Disconnected;
      String apSsid = buildApSsid();
      _provisioning.start(apSsid.c_str());
    }

    bool isProvisioning() const override { return _mode == Mode::Provisioning; }

    const char* provisioningStateName() const override { return _provisioning.stateName(); }

    String provisioningApSsid() const override { return _provisioning.apSsid(); }
    String provisioningApIp() const override { return _provisioning.apIp(); }

    bool hasStoredCredentials() const override { return _store.hasCredentials(); }

  private:
    enum class Mode
    {
      Normal,
      Provisioning,
    };

    Mode _mode = Mode::Normal;
    WifiCredentialStore _store;
    ProvisioningManager _provisioning;
    WiFiCredentials _creds;

    State _state = State::Disconnected;
    unsigned long _attemptStart = 0;
    unsigned long _nextAttemptAt = 0;
    int _retries = 0;

    void startConnect(State next)
    {
      _state = State::Connecting;
      _attemptStart = millis();
      logging::info(TAG, "%s (retry=%d)", stateToString(next), _retries);
      errorlog::record(("wifi_connecting " + String(_retries + 1)).c_str());
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(_creds.ssid.c_str(), _creds.password.c_str());
    }

    void handleAttempt()
    {
      wl_status_t status = WiFi.status();
      if (status == WL_CONNECTED)
      {
        _state = State::Connected;
        _retries = 0;
        logging::info(TAG, "connected");
        errorlog::record("wifi_connected");
        logging::info(TAG, "ssid=%s ip=%s rssi=%d dBm",
                      ssid().c_str(), localIp().c_str(), (int)rssi());
        return;
      }

      if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL)
      {
        logging::info(TAG, "connect failed (status=%d)", (int)status);
        errorlog::record("wifi_fail");
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
