#pragma once

#include "diagnostics/display_test.h"
#include "hardware/display/display.h"
#include "networking/network.h"
#include "services/weather/weather.h"
#include "ui/screens/weather_screen.h"

namespace app
{
  // Application lifecycle owner. Depends only on the display, network, weather,
  // and weather-screen abstractions; never on LovyanGFX, WiFi, HTTP, or a
  // specific weather provider.
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
    static constexpr unsigned long kWeatherRefreshIntervalMs = 15UL * 60 * 1000;
    static constexpr unsigned long kWeatherRetryIntervalMs = 5UL * 60 * 1000;
    static constexpr unsigned long kInitialRetryIntervalMs = 30UL * 1000;

    enum class UiState
    {
      None,
      Loading,
      Ready,
      Offline,
      UpdateFailed,
    };

    display::IDisplay& _display;
    diagnostics::DisplayTest& _displayTest;
    networking::INetwork& _network;
    weather::IWeatherService& _weather;
    ui::WeatherScreen _weatherScreen;

    bool _hasWeatherData = false;
    bool _lastWeatherOk = false;
    unsigned long _nextWeatherRefreshAt = 0;
    weather::WeatherData _weatherData;
    UiState _lastUiState = UiState::None;
    unsigned long _lastRenderedStamp = 0;
    int _lastLoadingAttempt = 0;

    void fetchWeather();
    void renderWeatherState();
    void logWeather(const weather::WeatherData& data) const;
  };
}
