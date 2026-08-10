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
                         networking::INetwork& network,
                         weather::IWeatherService& weather)
  : _display(display), _displayTest(displayTest), _network(network), _weather(weather)
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
  _weather.begin();

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

  if (!_weatherRequested && _network.isConnected())
  {
    _weatherRequested = true;
    fetchWeather();
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

void Application::fetchWeather()
{
  weather::WeatherData data;
  weather::WeatherError result = _weather.getCurrentWeather(data);

  if (result == weather::WeatherError::Ok)
  {
    logging::info("WEATHER", "request successful");
    logging::info("WEATHER", "location=%s lat=%.4f lon=%.4f",
                  data.locationName.c_str(), data.latitude, data.longitude);
    logging::info("WEATHER", "temperature=%.1f C feels_like=%.1f C",
                  data.temperatureC, data.feelsLikeC);
    logging::info("WEATHER", "humidity=%d %% pressure=%d hPa",
                  data.humidityPercent, data.pressureHpa);
    logging::info("WEATHER", "wind=%.1f m/s dir=%d deg",
                  data.windSpeed, data.windDirection);
    logging::info("WEATHER", "condition=%s (%s)",
                  data.condition.c_str(), data.conditionDescription.c_str());
    logging::info("WEATHER", "timestamp=%lu", data.timestamp);
  }
  else
  {
    logging::info("WEATHER", "request failed: %s", weather::weatherErrorName(result));
  }

  drawWeatherStatus(result, data);
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

void Application::drawWeatherStatus(weather::WeatherError result, const weather::WeatherData& data)
{
  _display.clear();

  _display.fillRect(0, 0, _display.width(), 28, display::Color::Blue);
  _display.drawText("Weather", _display.width() / 2, 14, display::TextSize::Small);

  if (result == weather::WeatherError::Ok)
  {
    String tempText = String(data.temperatureC, 1) + " C";
    _display.drawText("Weather OK", _display.width() / 2, 120, display::TextSize::Large);
    _display.drawText(tempText.c_str(), _display.width() / 2, 165, display::TextSize::Large);
    _display.drawText(data.conditionDescription.c_str(), _display.width() / 2, 205, display::TextSize::Small);
  }
  else
  {
    _display.drawText("Weather", _display.width() / 2, 100, display::TextSize::Large);
    _display.drawText("ERROR", _display.width() / 2, 140, display::TextSize::Large);
    _display.drawText(weather::weatherErrorName(result), _display.width() / 2, 185, display::TextSize::Small);
  }
}
}
