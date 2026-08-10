#pragma once

#include <cstdint>

namespace display
{
  // 16-bit 565 RGB colors, independent of the underlying graphics library.
  enum class Color : uint16_t
  {
    Black   = 0x0000,
    Red     = 0xF800,
    Green   = 0x07E0,
    Blue    = 0x001F,
    White   = 0xFFFF,
    Cyan    = 0x07FF,
    Magenta = 0xF81F,
    Yellow  = 0xFFE0,
  };

  enum class TextSize : uint8_t
  {
    Small = 1,
    Large = 2,
  };

  // Display abstraction. Application and diagnostic code depend only on this
  // interface, never on the underlying graphics library (LovyanGFX).
  class IDisplay
  {
  public:
    virtual ~IDisplay() = default;

    // Initialize the display hardware. Returns true on success.
    virtual bool init() = 0;

    // Clear the display to black.
    virtual void clear() = 0;

    // Fill the entire display with a solid color.
    virtual void fillScreen(Color color) = 0;

    // Fill a solid rectangle.
    virtual void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color) = 0;

    // Draw text centered horizontally/vertically at (centerX, centerY).
    virtual void drawText(const char* text, int32_t centerX, int32_t centerY, TextSize size) = 0;

    // Display dimensions in pixels.
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
  };

  // Factory: returns the singleton display instance. The concrete class stays
  // hidden in the implementation translation unit.
  IDisplay& getDisplay();
}
