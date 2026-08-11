#include "wifi_credentials.h"

#include <Preferences.h>

namespace networking
{
namespace
{
  const char* kNvsNamespace = "wifi";
  const char* kKeySsid = "ssid";
  const char* kKeyPassword = "pass";
}

void WifiCredentialStore::begin()
{
  Preferences prefs;
  prefs.begin(kNvsNamespace, false);
  prefs.end();
}

bool WifiCredentialStore::hasCredentials() const
{
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true))
  {
    return false;
  }
  bool has = prefs.isKey(kKeySsid);
  prefs.end();
  return has;
}

bool WifiCredentialStore::load(WiFiCredentials& out) const
{
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true))
  {
    return false;
  }
  String ssid = prefs.getString(kKeySsid, "");
  String password = prefs.getString(kKeyPassword, "");
  prefs.end();

  if (ssid.length() == 0)
  {
    return false;
  }
  out.ssid = ssid;
  out.password = password;
  return true;
}

void WifiCredentialStore::save(const WiFiCredentials& creds)
{
  Preferences prefs;
  prefs.begin(kNvsNamespace, false);
  if (creds.ssid.length() > 0)
  {
    prefs.putString(kKeySsid, creds.ssid);
    prefs.putString(kKeyPassword, creds.password);
  }
  prefs.end();
}

void WifiCredentialStore::clear()
{
  Preferences prefs;
  prefs.begin(kNvsNamespace, false);
  prefs.remove(kKeySsid);
  prefs.remove(kKeyPassword);
  prefs.end();
}
}
