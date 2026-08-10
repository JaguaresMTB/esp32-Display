#include "http.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace http
{
SecureClient::SecureClient(unsigned long timeoutMs) : _timeoutMs(timeoutMs) {}

bool SecureClient::get(const String& url, Response& response)
{
  WiFiClientSecure client;
  client.setInsecure(); // TLS encrypted, no certificate verification (validation build)

  HTTPClient http;
  if (!http.begin(client, url))
  {
    http.end();
    return false;
  }

  client.setTimeout(_timeoutMs);
  http.setTimeout(_timeoutMs);

  int status = http.GET();
  if (status > 0)
  {
    response.statusCode = status;
    response.body = http.getString();
    http.end();
    return true;
  }

  // status <= 0 => connection, TLS, or timeout failure.
  response.statusCode = 0;
  http.end();
  return false;
}
}
