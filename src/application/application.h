#pragma once

#include "diagnostics/display_test.h"
#include "hardware/display/display.h"
#include "networking/network.h"

namespace app
{
  // Application lifecycle owner. Depends only on the display abstraction, the
  // diagnostic test, and the network abstraction; never on LovyanGFX or WiFi.
  class Application
  {
  public:
    Application(display::IDisplay& display,
                diagnostics::DisplayTest& displayTest,
                networking::INetwork& network);

    bool begin();
    void update();

  private:
    display::IDisplay& _display;
    diagnostics::DisplayTest& _displayTest;
    networking::INetwork& _network;

    networking::State _lastNetworkState = networking::State::Disconnected;
    String _lastIp;
    int16_t _lastRssi = 0;

    void drawNetworkStatus();
  };
}
