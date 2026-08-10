#include <Arduino.h>

#include "application/application.h"
#include "common/logging.h"
#include "diagnostics/display_test.h"
#include "hardware/display/display.h"

// Composition root: wire the concrete display implementation to the
// application. No display-specific logic lives here.
static display::IDisplay& g_display = display::getDisplay();
static diagnostics::DisplayTest g_displayTest(g_display);
static app::Application g_application(g_display, g_displayTest);

void setup(void)
{
  logging::begin(115200);
  g_application.begin();
}

void loop(void)
{
  g_application.update();
}
