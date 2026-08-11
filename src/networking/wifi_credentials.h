#pragma once

#include <Arduino.h>

namespace networking
{
  // Wi-Fi credentials model. Password is sensitive: never printed, never
  // written to the error log, never exposed over HTTP.
  struct WiFiCredentials
  {
    String ssid;
    String password;
  };

  // Persistent credential storage in NVS (dedicated "wifi" namespace, kept
  // separate from the boot/error log). Secrets are never logged.
  class WifiCredentialStore
  {
  public:
    void begin();

    bool hasCredentials() const;
    bool load(WiFiCredentials& out) const;
    void save(const WiFiCredentials& creds);
    void clear();
  };
}
