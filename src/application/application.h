#pragma once

#include "diagnostics/display_test.h"
#include "hardware/display/display.h"
#include "networking/network.h"
#include "services/weather/weather.h"

namespace app
{
  // Application lifecycle owner. Depends only on the display, network, and
  // weather abstractions; never on LovyanGFX, WiFi, HTTP, or a specific
  // weather provider.
  class Application
  {
  public:
    Application(display::IDisplay& display,
                diagnostics::DisplayTest& displayTest,
                networking::INetwork& network,
                weather::IWeatherService& weather);

    bool begin();
    void update();

  private:
    display::IDisplay& _display;
    diagnostics::DisplayTest& _displayTest;
    networking::INetwork& _network;
    weather::IWeatherService& _weather;

    networking::State _lastNetworkState = networking::State::Disconnected;
    String _lastIp;
    int16_t _lastRssi = 0;
    bool _weatherRequested = false;

    void drawNetworkStatus();
    void drawWeatherStatus(weather::WeatherError result, const weather::WeatherData& data);
    void fetchWeather();
  };
}
