#pragma once

#include "hardware/display/display.h"
#include "services/weather/weather.h"

namespace ui
{
  // Presentation-only weather screen. Depends only on display::IDisplay and
  // the provider-independent WeatherData model. Makes no network requests,
  // parses no JSON, and knows nothing about OpenWeather.
  class WeatherScreen
  {
  public:
    WeatherScreen(display::IDisplay& display, int32_t timezoneOffsetSeconds);

    // No weather data yet.
    void renderLoading();

    // Valid weather data (success state).
    void render(const weather::WeatherData& data);

    // Keep last valid data, indicate Wi-Fi is offline.
    void renderOffline(const weather::WeatherData& data);

    // Keep last valid data, indicate the last weather update failed.
    void renderUpdateFailed(const weather::WeatherData& data);

  private:
    display::IDisplay& _display;
    int32_t _tzOffsetSeconds;

    void drawHeader(const char* title, display::Color barColor);
    void drawWeatherBody(const weather::WeatherData& data);
    void drawFooter(const String& text, display::Color barColor);
    void drawTextAlignedMetric(const char* label, const String& value, int32_t y);

    String formatTime(unsigned long unixTime) const;
    String asciiFold(const String& text) const;
    String titleCase(const String& text) const;
  };
}
