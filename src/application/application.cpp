#include "application.h"

#include <Arduino.h>

#include "common/error_log.h"
#include "common/logging.h"
#include "config/pins.h"

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
                         weather::IWeatherService& weather,
                         location::ILocationService& location)
  : _display(display), _displayTest(displayTest), _network(network), _weather(weather),
    _location(location),
    _weatherScreen(display, WEATHER_TIMEZONE_OFFSET_HOURS * 3600,
                   WEATHER_UI_LANG ? ui::Language::Spanish : ui::Language::English)
{
}

bool Application::begin()
{
  logging::info(TAG, "application startup");
  errorlog::begin();
  errorlog::dump();
  errorlog::record("boot");

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  logging::info("DISPLAY", "display initialization start");
  bool ok = _display.init();
  if (!ok)
  {
    logging::info("DISPLAY", "display initialization FAILED");
    return false;
  }
  logging::info("DISPLAY", "display initialization complete");

  _network.begin();
  _weather.begin();
  _location.begin();

  _weatherStage = ui::WeatherScreen::Stage::Pending;
  _weatherAttempt = 0;

  if (_network.isProvisioning())
  {
    renderProvisioningState();
  }
  else
  {
    forceRenderChecklist();
  }

  return true;
}

void Application::update()
{
  handleBootButton();

  _network.update();

  const bool provisioning = _network.isProvisioning();
  const bool connected = _network.isConnected();

  if (provisioning)
  {
    renderProvisioningState();
  }
  else
  {
    // Periodic / on-demand weather refresh. Scheduling is elapsed-time based.
    if (connected && millis() >= _nextWeatherRefreshAt)
    {
      _nextWeatherRefreshAt = millis() + kWeatherRetryIntervalMs; // safety default

      ensureLocation();

      if (!_hasWeatherData)
      {
        // Step-by-step checklist progress before the first data arrives.
        _weatherAttempt++;
        _weatherStage = ui::WeatherScreen::Stage::Connecting;
        forceRenderChecklist();
        delay(400);
        _weatherStage = ui::WeatherScreen::Stage::Authorizing;
        forceRenderChecklist();
        delay(400);
      }

      fetchWeather();
    }

    renderWeatherState();
    _weatherScreen.updateAnimation(millis());
  }

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
    else if (c == 'p')
    {
      logging::info(TAG, "provisioning requested");
      _network.enterProvisioningMode();
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
    _weatherStage = ui::WeatherScreen::Stage::Done;
    _nextWeatherRefreshAt = millis() + kWeatherRefreshIntervalMs;
    errorlog::record("weather_ok");
    logging::info("WEATHER", "request successful");
    logWeather(data);
    logging::info("WEATHER", "next refresh in %lu s", kWeatherRefreshIntervalMs / 1000);
  }
  else
  {
    _lastWeatherOk = false;
    _weatherStage = ui::WeatherScreen::Stage::Connecting;
    unsigned long retry = _hasWeatherData ? kWeatherRetryIntervalMs : kInitialRetryIntervalMs;
    _nextWeatherRefreshAt = millis() + retry;
    errorlog::record(("weather_fail " + String(weather::weatherErrorName(result))).c_str());
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
  logging::info("WEATHER", "rain_probability=%d %% sunrise=%lu sunset=%lu",
                data.rainProbabilityPercent, data.sunrise, data.sunset);
  logging::info("WEATHER", "timestamp=%lu", data.timestamp);
}

void Application::forceRenderChecklist()
{
  _lastUiState = UiState::None;
  renderWeatherState();
}

void Application::ensureLocation()
{
  String ssid = _network.ssid();
  if (ssid.isEmpty() || ssid == _resolvedSsid)
  {
    return;
  }

  location::Location loc;
  if (_location.resolve(loc))
  {
    _weather.setLocation(loc.latitude, loc.longitude, loc.name.c_str());
    _weatherScreen.setTimezoneOffsetSeconds(loc.utcOffsetSeconds);
    _resolvedSsid = ssid;
    logging::info(TAG, "location applied: %s (%.4f, %.4f) utc=%ld s",
                  loc.name.c_str(), loc.latitude, loc.longitude, (long)loc.utcOffsetSeconds);
  }
  else
  {
    // Keep the compile-time/default location; retried on the next refresh.
    logging::info(TAG, "location resolve failed; using configured fallback");
  }
}

void Application::handleBootButton()
{
  const bool pressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  const unsigned long now = millis();

  if (pressed)
  {
    if (!_buttonPressed)
    {
      _buttonPressed = true;
      _buttonPressStart = now;
      _buttonLongTriggered = false;
      logging::info(TAG, "BOOT pressed (pin=%d)", BOOT_BUTTON_PIN);
    }
    else if (!_buttonLongTriggered && (now - _buttonPressStart >= kBootButtonLongPressMs))
    {
      _buttonLongTriggered = true;
      logging::info(TAG, "BOOT long press -> provisioning");
      _network.enterProvisioningMode();
    }
  }
  else
  {
    if (_buttonPressed && !_buttonLongTriggered)
    {
      logging::info(TAG, "BOOT short press (ignored)");
    }
    _buttonPressed = false;
    _buttonLongTriggered = false;
  }
}

void Application::renderProvisioningState()
{
  String ap = _network.provisioningApSsid();
  String ip = _network.provisioningApIp();
  if (ap != _lastProvisioningAp || ip != _lastProvisioningIp ||
      _lastUiState != UiState::Provisioning)
  {
    _lastProvisioningAp = ap;
    _lastProvisioningIp = ip;
    _lastUiState = UiState::Provisioning;
    _weatherScreen.renderProvisioning(ap.c_str(), ip.c_str());
  }
}

ui::WeatherScreen::Stage Application::mapStage(networking::ConnectStage stage)
{
  switch (stage)
  {
    case networking::ConnectStage::Connecting:
      return ui::WeatherScreen::Stage::Connecting;
    case networking::ConnectStage::Authorizing:
      return ui::WeatherScreen::Stage::Authorizing;
    case networking::ConnectStage::Connected:
      return ui::WeatherScreen::Stage::Done;
    default:
      return ui::WeatherScreen::Stage::Pending;
  }
}

void Application::renderWeatherState()
{
  const bool connected = _network.isConnected();
  const int wifiAttempt = _network.retryCount() + 1;
  const ui::WeatherScreen::Stage wifiStage = mapStage(_network.connectStage());

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

  // Redraw the checklist when any of its inputs change.
  if (target == UiState::Loading &&
      (wifiStage != _lastWifiStage || wifiAttempt != _lastWifiAttempt ||
       _weatherStage != _lastWeatherStage || _weatherAttempt != _lastWeatherAttempt))
  {
    shouldRedraw = true;
  }

  if (shouldRedraw)
  {
    _lastUiState = target;
    _lastRenderedStamp = stamp;
    _lastWifiStage = wifiStage;
    _lastWifiAttempt = wifiAttempt;
    _lastWeatherStage = _weatherStage;
    _lastWeatherAttempt = _weatherAttempt;

    switch (target)
    {
      case UiState::Loading:
        _weatherScreen.renderChecklist(wifiStage, wifiAttempt, _weatherStage, _weatherAttempt,
                                       connected ? _network.localIp().c_str() : "");
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
