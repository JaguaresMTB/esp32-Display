#include "location.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "common/logging.h"
#include "networking/http.h"
#include "networking/network.h"

namespace location
{
namespace
{
  const char* TAG = "LOC";
  const char* kGeoIpUrl = "https://ipapi.co/json/";
  const char* kNvsNamespace = "loc";
  const int kMaxSlots = 6;
}

int32_t parseUtcOffset(const String& s)
{
  // Accepts "-06:00" or "-0600" (both seen in the wild).
  if (s.length() < 5)
  {
    return 0;
  }
  int sign = (s[0] == '-') ? -1 : 1;
  String digits = s.substring(1);
  int hh = 0;
  int mm = 0;
  int colon = digits.indexOf(':');
  if (colon >= 0)
  {
    hh = digits.substring(0, 2).toInt();
    mm = digits.substring(3, 5).toInt();
  }
  else
  {
    hh = digits.substring(0, 2).toInt();
    mm = digits.substring(2, 4).toInt();
  }
  return sign * (hh * 3600 + mm * 60);
}

class GeoIpLocationProvider final : public ILocationService
{
public:
  bool begin() override
  {
    Preferences prefs;
    prefs.begin(kNvsNamespace, false);
    prefs.end();
    return true;
  }

  bool resolve(Location& out) override
  {
    networking::INetwork& network = networking::getNetwork();
    if (!network.isConnected())
    {
      logging::info(TAG, "resolve aborted: not connected");
      return false;
    }

    String ssid = network.ssid();
    if (ssid.length() == 0)
    {
      return false;
    }

    if (loadCached(ssid, out))
    {
      logging::info(TAG, "cached location for %s", ssid.c_str());
      return true;
    }

    logging::info(TAG, "resolving location via GeoIP (ssid=%s)", ssid.c_str());

    http::SecureClient client;
    http::Response response;
    if (!client.get(kGeoIpUrl, response))
    {
      logging::info(TAG, "GeoIP request failed (HTTP/TLS)");
      return false;
    }
    if (response.statusCode != 200)
    {
      logging::info(TAG, "GeoIP HTTP status %d", response.statusCode);
      return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response.body);
    if (err)
    {
      logging::info(TAG, "GeoIP parse failed: %s", err.c_str());
      return false;
    }
    if (!doc["latitude"].is<float>() || !doc["longitude"].is<float>())
    {
      logging::info(TAG, "GeoIP missing lat/lon");
      return false;
    }

    out.latitude = doc["latitude"] | 0.0f;
    out.longitude = doc["longitude"] | 0.0f;
    String city = doc["city"] | "";
    if (city.length() == 0)
    {
      city = doc["region"] | "";
    }
    if (city.length() == 0)
    {
      city = doc["country_name"] | "";
    }
    out.name = city;
    out.utcOffsetSeconds = parseUtcOffset(doc["utc_offset"] | "");

    saveCache(ssid, out);

    logging::info(TAG, "resolved: %s (%.4f, %.4f) utc=%ld s",
                  out.name.c_str(), out.latitude, out.longitude, (long)out.utcOffsetSeconds);
    return true;
  }

private:
  bool loadCached(const String& ssid, Location& out)
  {
    Preferences prefs;
    if (!prefs.begin(kNvsNamespace, true))
    {
      return false;
    }
    unsigned int count = prefs.getUInt("count", 0);
    int n = (int)min(count, (unsigned int)kMaxSlots);
    bool found = false;
    for (int i = 0; i < n; i++)
    {
      if (prefs.getString(("s" + String(i)).c_str(), "") == ssid)
      {
        out.name = prefs.getString(("n" + String(i)).c_str(), "");
        out.latitude = prefs.getFloat(("a" + String(i)).c_str(), 0);
        out.longitude = prefs.getFloat(("o" + String(i)).c_str(), 0);
        out.utcOffsetSeconds = prefs.getInt(("t" + String(i)).c_str(), 0);
        found = true;
        break;
      }
    }
    prefs.end();
    return found;
  }

  void saveCache(const String& ssid, const Location& loc)
  {
    Preferences prefs;
    prefs.begin(kNvsNamespace, false);
    unsigned int count = prefs.getUInt("count", 0);
    int slot = (int)(count % kMaxSlots);
    prefs.putString(("s" + String(slot)).c_str(), ssid);
    prefs.putString(("n" + String(slot)).c_str(), loc.name);
    prefs.putFloat(("a" + String(slot)).c_str(), loc.latitude);
    prefs.putFloat(("o" + String(slot)).c_str(), loc.longitude);
    prefs.putInt(("t" + String(slot)).c_str(), loc.utcOffsetSeconds);
    prefs.putUInt("count", count + 1);
    prefs.end();
  }
};

ILocationService& getLocationService()
{
  static GeoIpLocationProvider instance;
  return instance;
}
}
