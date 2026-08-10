#include "error_log.h"

#include <Arduino.h>
#include <Preferences.h>

namespace errorlog
{
namespace
{
  const int kSlots = 10;

  Preferences prefs;
  bool opened = false;

  String slotKey(int slot)
  {
    return "e" + String(slot);
  }
}

void begin()
{
  prefs.begin("bootlog", false);
  opened = true;
}

void record(const char* event)
{
  if (!opened || event == nullptr)
  {
    return;
  }
  unsigned int count = prefs.getUInt("count", 0);
  int slot = (int)(count % kSlots);
  prefs.putString(slotKey(slot).c_str(), event);
  prefs.putUInt("count", count + 1);
}

void dump()
{
  if (!opened)
  {
    return;
  }
  unsigned int count = prefs.getUInt("count", 0);
  unsigned int start = count > (unsigned int)kSlots ? count - kSlots : 0;

  Serial.print("[LOG] --- boot log (");
  Serial.print(count);
  Serial.println(" events) ---");

  for (unsigned int i = start; i < count; i++)
  {
    String entry = prefs.getString(slotKey((int)(i % kSlots)).c_str(), "");
    if (entry.length() > 0)
    {
      Serial.print("[LOG] ");
      Serial.println(entry);
    }
  }
}
}
