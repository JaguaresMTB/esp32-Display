#pragma once

#include "hardware/display/display.h"

namespace diagnostics
{
  // Manual display validation: full-screen color cycle + text frame.
  // Uses only the display abstraction, so it works on any display driver.
  class DisplayTest
  {
  public:
    explicit DisplayTest(display::IDisplay& display);

    // Runs: RED, GREEN, BLUE, WHITE, then a text frame.
    void run();

  private:
    display::IDisplay& _display;
  };
}
