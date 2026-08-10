#include <Arduino.h>

#include "application/application.h"
#include "common/logging.h"
#include "diagnostics/display_test.h"
#include "hardware/display/display.h"
#include "networking/network.h"

// Composition root: wire the concrete display and network implementations to
// the application. No display- or network-specific logic lives here.
static display::IDisplay& g_display = display::getDisplay();
static diagnostics::DisplayTest g_displayTest(g_display);
static networking::INetwork& g_network = networking::getNetwork();
static app::Application g_application(g_display, g_displayTest, g_network);

void setup(void)
{
  logging::begin(115200);
  g_application.begin();
}

void loop(void)
{
  g_application.update();
}
