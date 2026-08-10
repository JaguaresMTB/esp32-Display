#include "application.h"

#include <Arduino.h>

#include "common/logging.h"

namespace app
{
namespace
{
  const char* TAG = "APP";
}

Application::Application(display::IDisplay& display, diagnostics::DisplayTest& displayTest)
  : _display(display), _displayTest(displayTest)
{
}

bool Application::begin()
{
  logging::info(TAG, "application startup");

  logging::info("DISPLAY", "display initialization start");
  bool ok = _display.init();
  if (!ok)
  {
    logging::info("DISPLAY", "display initialization FAILED");
    return false;
  }
  logging::info("DISPLAY", "display initialization complete");

  _displayTest.run();
  return true;
}

void Application::update()
{
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 'r')
    {
      logging::info(TAG, "rerun requested");
      _displayTest.run();
    }
  }

  static unsigned long lastBeat = 0;
  if (millis() - lastBeat >= 2000)
  {
    lastBeat = millis();
    Serial.println("alive");
  }
  delay(50);
}
}
