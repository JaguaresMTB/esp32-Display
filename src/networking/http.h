#pragma once

#include <Arduino.h>

namespace http
{
  struct Response
  {
    int statusCode = 0; // 0 means no response received (connection/TLS failure)
    String body;
  };

  // Minimal HTTPS GET transport. Hides WiFiClientSecure/HTTPClient (and all
  // TLS details) from callers such as the weather provider.
  class SecureClient
  {
  public:
    explicit SecureClient(unsigned long timeoutMs = 10000);

    // Perform a GET request. Returns true when a response (any status) was
    // received; returns false on connection/TLS failure.
    bool get(const String& url, Response& response);

  private:
    unsigned long _timeoutMs;
  };
}
