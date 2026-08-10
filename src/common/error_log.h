#pragma once

// Persistent boot/error log stored in flash (NVS). Records a small ring of
// the most recent events so failures can be reviewed after a reboot/unplug.
namespace errorlog
{
  // Open the NVS log. Must be called once at boot before record()/dump().
  void begin();

  // Append an event (short string, no secrets). Oldest entries are dropped.
  void record(const char* event);

  // Print all stored entries to serial.
  void dump();
}
