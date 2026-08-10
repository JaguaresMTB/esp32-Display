#pragma once

#include "diagnostics/display_test.h"
#include "hardware/display/display.h"

namespace app
{
  // Application lifecycle owner. Depends only on the display abstraction and
  // the diagnostic test; never on LovyanGFX.
  class Application
  {
  public:
    Application(display::IDisplay& display, diagnostics::DisplayTest& displayTest);

    bool begin();
    void update();

  private:
    display::IDisplay& _display;
    diagnostics::DisplayTest& _displayTest;
  };
}
