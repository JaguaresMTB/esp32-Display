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
    Orange  = 0xFD20,
    Gray    = 0x8410,
    SkyBlue = 0x5D1B,   // day background (RGB 90,160,220)
    NightBlue = 0x114B, // night background / dark text (RGB 20,40,90)
  };

  enum class TextSize : uint8_t
  {
    Small = 1,
    Medium = 2,
    Large = 3,
    XLarge = 4,
    Metric = 5, // ~20 px (between Small and Medium)
    Bold = 6,   // ~16 px bold (FreeSansBold12pt)
  };

  enum class TextAlign : uint8_t
  {
    Left = 0,
    Center = 1,
    Right = 2,
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

    // Draw text with the given horizontal alignment; `x` is the anchor (left
    // edge for Left, center for Center, right edge for Right). Vertically
    // centered at `y`.
    virtual void drawTextAligned(const char* text, int32_t x, int32_t y, TextSize size, TextAlign align) = 0;

    // Set the color used by drawText/drawTextAligned (default White).
    virtual void setTextColor(Color color) = 0;

    // Draw a single-pixel line.
    virtual void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color) = 0;

    // Filled circle.
    virtual void fillCircle(int32_t cx, int32_t cy, int32_t radius, Color color) = 0;

    // Filled ellipse.
    virtual void fillEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, Color color) = 0;

    // Filled triangle.
    virtual void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color color) = 0;

    // Display dimensions in pixels.
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
  };

  // Factory: returns the singleton display instance. The concrete class stays
  // hidden in the implementation translation unit.
  IDisplay& getDisplay();
}
