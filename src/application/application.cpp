#include "application.h"

#include <Arduino.h>

#include "common/logging.h"

// Timezone offset for the "last update" display (from the local weather config).
#if __has_include("config/weather_credentials.h")
#include "config/weather_credentials.h"
#endif
#ifndef WEATHER_TIMEZONE_OFFSET_HOURS
#define WEATHER_TIMEZONE_OFFSET_HOURS 0
#endif
#ifndef WEATHER_UI_LANG
#define WEATHER_UI_LANG 1
#endif

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
  : _display(display), _displayTest(displayTest), _network(network), _weather(weather),
    _weatherScreen(display, WEATHER_TIMEZONE_OFFSET_HOURS * 3600,
                   WEATHER_UI_LANG ? ui::Language::Spanish : ui::Language::English)
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

  _weatherScreen.renderLoading(false);
  _lastUiState = UiState::Loading;
  _lastLoadingOffline = false;

  return true;
}

void Application::update()
{
  _network.update();

  const bool connected = _network.isConnected();

  // Periodic / on-demand weather refresh. Scheduling is elapsed-time based;
  // the request itself is a single bounded call.
  if (connected && millis() >= _nextWeatherRefreshAt)
  {
    _nextWeatherRefreshAt = millis() + kWeatherRetryIntervalMs; // safety default
    fetchWeather();
  }

  renderWeatherState();

  _weatherScreen.updateAnimation(millis());

  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 'r')
    {
      logging::info(TAG, "rerun requested");
      _displayTest.run();
      _lastUiState = UiState::None; // force the weather screen to redraw
    }
    else if (c == 'w')
    {
      logging::info(TAG, "weather refresh requested");
      _nextWeatherRefreshAt = millis();
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
    _weatherData = data;
    _hasWeatherData = true;
    _lastWeatherOk = true;
    _nextWeatherRefreshAt = millis() + kWeatherRefreshIntervalMs;
    logging::info("WEATHER", "request successful");
    logWeather(data);
    logging::info("WEATHER", "next refresh in %lu s", kWeatherRefreshIntervalMs / 1000);
  }
  else
  {
    _lastWeatherOk = false;
    // Retry faster while we still have no data to show; once data exists,
    // fall back to the longer failure cadence.
    unsigned long retry = _hasWeatherData ? kWeatherRetryIntervalMs : kInitialRetryIntervalMs;
    _nextWeatherRefreshAt = millis() + retry;
    logging::info("WEATHER", "request failed: %s (next in %lu s)",
                  weather::weatherErrorName(result), retry / 1000);
  }
}

void Application::logWeather(const weather::WeatherData& data) const
{
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

void Application::renderWeatherState()
{
  const bool connected = _network.isConnected();
  const bool offline = _network.state() == networking::State::Disconnected;

  UiState target;
  if (!_hasWeatherData)
  {
    target = UiState::Loading;
  }
  else if (!connected)
  {
    target = UiState::Offline;
  }
  else if (!_lastWeatherOk)
  {
    target = UiState::UpdateFailed;
  }
  else
  {
    target = UiState::Ready;
  }

  const unsigned long stamp = _hasWeatherData ? _weatherData.timestamp : 0;
  bool shouldRedraw = (target != _lastUiState || stamp != _lastRenderedStamp);
  if (target == UiState::Loading && offline != _lastLoadingOffline)
  {
    _lastLoadingOffline = offline;
    shouldRedraw = true;
  }

  if (shouldRedraw)
  {
    _lastUiState = target;
    _lastRenderedStamp = stamp;

    switch (target)
    {
      case UiState::Loading:
        _weatherScreen.renderLoading(offline);
        break;
      case UiState::Ready:
        _weatherScreen.render(_weatherData);
        break;
      case UiState::Offline:
        _weatherScreen.renderOffline(_weatherData);
        break;
      case UiState::UpdateFailed:
        _weatherScreen.renderUpdateFailed(_weatherData);
        break;
      default:
        break;
    }
  }
}
}
