#include "display_test.h"

#include <Arduino.h>

#include "common/logging.h"

namespace diagnostics
{
namespace
{
  const char* TAG = "TEST";
}

DisplayTest::DisplayTest(display::IDisplay& display) : _display(display) {}

void DisplayTest::run()
{
  logging::info(TAG, "diagnostic test start");

  _display.fillScreen(display::Color::Red);
  logging::info(TAG, "RED");
  delay(2000);

  _display.fillScreen(display::Color::Green);
  logging::info(TAG, "GREEN");
  delay(2000);

  _display.fillScreen(display::Color::Blue);
  logging::info(TAG, "BLUE");
  delay(2000);

  _display.fillScreen(display::Color::White);
  logging::info(TAG, "WHITE");
  delay(2000);

  _display.clear();
  _display.setTextColor(display::Color::White);
  _display.fillRect(10, 120, _display.width() - 20, 80, display::Color::Red);
  _display.drawText("Hello World!", _display.width() / 2, 160, display::TextSize::Medium);

  logging::info(TAG, "diagnostic test complete");
}
}
