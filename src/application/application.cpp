#include "application.h"

#include <Arduino.h>

#include "common/logging.h"

namespace app
{
namespace
{
  const char* TAG = "APP";
}

Application::Application(display::IDisplay& display,
                         diagnostics::DisplayTest& displayTest,
                         networking::INetwork& network)
  : _display(display), _displayTest(displayTest), _network(network)
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

  _network.begin();

  return true;
}

void Application::update()
{
  _network.update();

  if (_network.state() != _lastNetworkState ||
      (_network.state() == networking::State::Connected &&
       (_network.localIp() != _lastIp || _network.rssi() != _lastRssi)))
  {
    _lastNetworkState = _network.state();
    _lastIp = _network.localIp();
    _lastRssi = _network.rssi();
    drawNetworkStatus();
  }

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
  delay(5);
}

void Application::drawNetworkStatus()
{
  _display.clear();

  _display.fillRect(0, 0, _display.width(), 28, display::Color::Blue);
  _display.drawText("Wi-Fi", _display.width() / 2, 14, display::TextSize::Small);

  switch (_network.state())
  {
    case networking::State::Connected:
      _display.drawText("Connected", _display.width() / 2, 120, display::TextSize::Large);
      _display.drawText(_network.localIp().c_str(), _display.width() / 2, 160, display::TextSize::Small);
      {
        String rssiText = "RSSI " + String(_network.rssi()) + " dBm";
        _display.drawText(rssiText.c_str(), _display.width() / 2, 185, display::TextSize::Small);
      }
      break;

    case networking::State::Connecting:
    case networking::State::Reconnecting:
      _display.drawText("Connecting...", _display.width() / 2, 120, display::TextSize::Large);
      break;

    case networking::State::Disconnected:
      _display.drawText("No network", _display.width() / 2, 120, display::TextSize::Large);
      break;
  }
}
}
