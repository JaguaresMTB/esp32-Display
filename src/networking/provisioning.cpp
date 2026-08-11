#include "provisioning.h"

#include <WiFi.h>

#include "common/error_log.h"
#include "common/logging.h"

namespace networking
{
namespace
{
  const char* TAG = "PROV";
  const IPAddress kApIp(192, 168, 4, 1);
  const IPAddress kApGateway(192, 168, 4, 1);
  const IPAddress kApSubnet(255, 255, 255, 0);
}

const char* provisioningStateName(ProvisioningState state)
{
  switch (state)
  {
    case ProvisioningState::Idle:              return "IDLE";
    case ProvisioningState::Provisioning:      return "PROVISIONING";
    case ProvisioningState::TestingConnection: return "TESTING_CONNECTION";
  }
  return "UNKNOWN";
}

ProvisioningManager::ProvisioningManager(WifiCredentialStore& store) : _store(store) {}

void ProvisioningManager::start(const char* apSsid)
{
  _apSsid = apSsid;
  _state = ProvisioningState::Provisioning;
  _completed = false;
  _lastResult = "waiting";

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(kApIp, kApGateway, kApSubnet);
  WiFi.softAP(_apSsid.c_str()); // open AP (documented trade-off; physical trigger)
  delay(300);

  _dns.start(53, "*", kApIp);

  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/configure", HTTP_POST, [this]() { handleConfigure(); });
  _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();

  logging::info(TAG, "softap=%s ip=%s", _apSsid.c_str(), kApIp.toString().c_str());
}

void ProvisioningManager::stop()
{
  _server.stop();
  _dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  _state = ProvisioningState::Idle;
}

void ProvisioningManager::update()
{
  if (_state == ProvisioningState::Idle)
  {
    return;
  }

  _dns.processNextRequest();
  _server.handleClient();

  if (_state == ProvisioningState::TestingConnection)
  {
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
      _lastResult = "connected";
      errorlog::record("provisioning_connection_success");

      WiFiCredentials creds;
      creds.ssid = _candidateSsid;
      creds.password = _candidatePassword;
      _store.save(creds);

      stop();
      errorlog::record("provisioning_completed");
      _completed = true;
    }
    else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL ||
             millis() - _testStart >= kTestTimeoutMs)
    {
      _lastResult = "failed";
      errorlog::record("provisioning_connection_failed");
      WiFi.disconnect();
      WiFi.mode(WIFI_AP_STA);
      _state = ProvisioningState::Provisioning; // AP stays up; user can retry
    }
  }
}

bool ProvisioningManager::active() const
{
  return _state != ProvisioningState::Idle;
}

ProvisioningState ProvisioningManager::state() const
{
  return _state;
}

const char* ProvisioningManager::stateName() const
{
  return provisioningStateName(_state);
}

String ProvisioningManager::apSsid() const
{
  return _apSsid;
}

String ProvisioningManager::apIp() const
{
  return kApIp.toString();
}

bool ProvisioningManager::takeCompletion()
{
  bool completed = _completed;
  _completed = false;
  return completed;
}

void ProvisioningManager::submitCandidate(const String& ssid, const String& password)
{
  _candidateSsid = ssid;
  _candidatePassword = password;
  _lastResult = "testing";
  _state = ProvisioningState::TestingConnection;
  _testStart = millis();

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA); // keep the AP up while testing the candidate
  WiFi.begin(_candidateSsid.c_str(), _candidatePassword.c_str());
}

void ProvisioningManager::handleRoot()
{
  scanNetworks();
  _server.send(200, "text/html", buildPage());
}

void ProvisioningManager::handleConfigure()
{
  String ssid = _server.arg("ssid");
  String password = _server.arg("password");
  ssid.trim();

  if (ssid.length() == 0)
  {
    _server.send(400, "text/plain", "SSID must not be empty");
    return;
  }

  submitCandidate(ssid, password);

  const char* page =
      "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:60px'>"
      "<h2>Testing connection...</h2>"
      "<p>The device is trying to connect. The page will refresh in a few seconds.</p>"
      "<script>setTimeout(function(){location.href='/';},3000);</script>"
      "</body></html>";
  _server.send(200, "text/html", page);
}

void ProvisioningManager::handleStatus()
{
  _server.send(200, "application/json", buildStatusJson());
}

void ProvisioningManager::handleNotFound()
{
  _server.sendHeader("Location", "http://192.168.4.1/", true);
  _server.send(302, "text/plain", "");
}

void ProvisioningManager::scanNetworks()
{
  if (millis() - _lastScan < kScanCacheMs)
  {
    return;
  }
  _lastScan = millis();

  int n = WiFi.scanNetworks();
  if (n < 0)
  {
    n = 0;
  }
  if (n > kMaxScan)
  {
    n = kMaxScan;
  }
  _scanCount = n;
  for (int i = 0; i < n; i++)
  {
    _scanSsid[i] = WiFi.SSID(i);
    _scanRssi[i] = WiFi.RSSI(i);
    _scanEnc[i] = WiFi.encryptionType(i);
  }
}

String ProvisioningManager::buildPage() const
{
  String html;
  html.reserve(1600);

  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Weather Display Setup</title>";
  html += "<style>body{font-family:Helvetica,Arial,sans-serif;margin:16px;max-width:480px}";
  html += "h2{color:#0a3d91}ul{list-style:none;padding:0}li{margin:4px 0}";
  html += "input{width:100%;padding:8px;margin:4px 0;box-sizing:border-box;font-size:16px}";
  html += "button{width:100%;padding:10px;font-size:16px;background:#0a3d91;color:#fff;border:none;margin-top:8px}";
  html += "#status{margin-top:12px;font-weight:bold}</style></head><body>";
  html += "<h2>Weather Display Setup</h2>";
  html += "<p>Device: <b>" + htmlEscape(_apSsid) + "</b><br>IP: 192.168.4.1</p>";

  html += "<p><b>Available networks:</b></p><ul>";
  if (_scanCount == 0)
  {
    html += "<li><i>No networks found. Type the SSID manually below.</i></li>";
  }
  for (int i = 0; i < _scanCount; i++)
  {
    html += "<li>";
    html += (_scanEnc[i] != WIFI_AUTH_OPEN) ? "[lock] " : "[open] ";
    html += htmlEscape(_scanSsid[i]) + " (" + String(_scanRssi[i]) + " dBm)</li>";
  }
  html += "</ul>";

  html += "<form method='POST' action='/configure'>";
  html += "<label><b>SSID:</b></label><br>";
  html += "<input name='ssid' list='nets' placeholder='Type or select a network' required>";
  html += "<datalist id='nets'>";
  for (int i = 0; i < _scanCount; i++)
  {
    html += "<option value='" + htmlEscape(_scanSsid[i]) + "'>";
  }
  html += "</datalist>";
  html += "<label><b>Password:</b></label><br>";
  html += "<input type='password' name='password' placeholder='Wi-Fi password'>";
  html += "<button type='submit'>Save &amp; Connect</button>";
  html += "</form>";

  html += "<div id='status'>Status: " + _lastResult + "</div>";
  html += "<script>var m={waiting:'Waiting for configuration',testing:'Testing connection...',";
  html += "connected:'Connected! Starting up...',failed:'Connection failed. Please verify the Wi-Fi name and password.'};";
  html += "setInterval(function(){fetch('/status').then(function(r){return r.json()}).then(function(j){";
  html += "document.getElementById('status').textContent=(m[j.state]||j.state)}).catch(function(){})},2000);</script>";
  html += "</body></html>";
  return html;
}

String ProvisioningManager::buildStatusJson() const
{
  String json = "{\"state\":\"";
  switch (_state)
  {
    case ProvisioningState::Provisioning:      json += "provisioning"; break;
    case ProvisioningState::TestingConnection: json += "testing"; break;
    default:                                   json += "idle"; break;
  }
  json += "\",\"result\":\"" + _lastResult + "\"}";
  return json;
}

String ProvisioningManager::htmlEscape(const String& text) const
{
  String out;
  out.reserve(text.length() + 8);
  for (size_t i = 0; i < text.length(); i++)
  {
    char c = text[i];
    switch (c)
    {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c; break;
    }
  }
  return out;
}
}
