#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include "networking/wifi_credentials.h"

namespace networking
{
  enum class ProvisioningState
  {
    Idle,
    Provisioning,
    TestingConnection,
  };

  const char* provisioningStateName(ProvisioningState state);

  // SoftAP + DNS captive portal + HTTP configuration portal + cached Wi-Fi
  // scan + candidate/active credential handling. Active only during
  // provisioning; fully stopped for normal operation.
  class ProvisioningManager
  {
  public:
    explicit ProvisioningManager(WifiCredentialStore& store);

    void start(const char* apSsid);
    void stop();
    void update();

    bool active() const;
    ProvisioningState state() const;
    const char* stateName() const;
    String apSsid() const;
    String apIp() const;

    // Called by the HTTP portal with the user-submitted credentials.
    void submitCandidate(const String& ssid, const String& password);

    // True once after a successful provisioning (candidate committed). The
    // caller (network manager) uses this to transition to normal mode.
    bool takeCompletion();

  private:
    static constexpr unsigned long kTestTimeoutMs = 15000;
    static constexpr unsigned long kScanCacheMs = 30000;
    static constexpr int kMaxScan = 12;

    WifiCredentialStore& _store;

    WebServer _server;
    DNSServer _dns;

    ProvisioningState _state = ProvisioningState::Idle;
    bool _completed = false;
    String _apSsid;
    String _lastResult;
    String _candidateSsid;
    String _candidatePassword;
    unsigned long _testStart = 0;

    unsigned long _lastScan = 0;
    int _scanCount = 0;
    String _scanSsid[kMaxScan];
    int32_t _scanRssi[kMaxScan];
    uint8_t _scanEnc[kMaxScan];

    void handleRoot();
    void handleConfigure();
    void handleStatus();
    void handleNotFound();

    void scanNetworks();
    String buildPage() const;
    String buildStatusJson() const;
    String htmlEscape(const String& text) const;
  };
}
