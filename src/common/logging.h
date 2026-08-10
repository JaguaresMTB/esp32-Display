#pragma once

#include <Arduino.h>
#include <stdarg.h>

// Minimal, consistent serial logging convention.
// Usage: logging::info("TAG", "message") or logging::info("TAG", "value=%d", n);
namespace logging
{
  inline void begin(unsigned long baud = 115200)
  {
    Serial.begin(baud);
  }

  inline void info(const char* tag, const char* fmt, ...)
  {
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Serial.print("[");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(buffer);
  }
}
