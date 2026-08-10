#pragma once

#include "hardware/display/display.h"
#include "services/weather/weather.h"

namespace ui
{
  enum class Language
  {
    English = 0,
    Spanish = 1,
  };

  // Presentation-only weather screen. Depends only on display::IDisplay and the
  // provider-independent WeatherData model. Makes no network requests, parses
  // no JSON, and knows nothing about OpenWeather.
  class WeatherScreen
  {
  public:
    WeatherScreen(display::IDisplay& display, int32_t timezoneOffsetSeconds, Language language);

    // No weather data yet. Shows connection progress: the target SSID and
    // attempt number while connecting, or "connected + updating weather" once
    // Wi-Fi is up but no data has arrived.
    void renderConnecting(const char* ssid, int attempt, bool connected, const char* ip);

    // Valid weather data (success state).
    void render(const weather::WeatherData& data);

    // Keep last valid data, indicate Wi-Fi is offline.
    void renderOffline(const weather::WeatherData& data);

    // Keep last valid data, indicate the last weather update failed.
    void renderUpdateFailed(const weather::WeatherData& data);

    // Called every loop; throttles internally and redraws only the animation
    // zone. Shows a spinner while loading, or a condition-based scene when
    // weather data is available.
    void updateAnimation(unsigned long now);

  private:
    static constexpr int32_t kZoneY = 36;
    static constexpr int32_t kZoneH = 104;
    static constexpr unsigned long kFrameIntervalMs = 80;

    enum class Scene
    {
      None,
      Sun,
      Clouds,
      Rain,
      Storm,
      Snow,
      Fog,
    };

    display::IDisplay& _display;
    int32_t _tzOffsetSeconds;
    Language _language;

    bool _hasData = false;
    bool _loading = false;
    Scene _scene = Scene::None;
    unsigned long _lastFrame = 0;

    static Scene sceneFor(weather::Condition condition);

    void drawHeader(const char* title, display::Color barColor);
    void drawWeatherBody(const weather::WeatherData& data);
    void drawFooter(const String& text, display::Color barColor);
    void drawTextAlignedMetric(const char* label, const String& value, int32_t y);

    const char* label(const char* en, const char* es) const;

    void clearZone();
    void drawScene(unsigned long now);
    void drawSun(unsigned long now);
    void drawClouds(unsigned long now);
    void drawRain(unsigned long now);
    void drawStorm(unsigned long now);
    void drawSnow(unsigned long now);
    void drawFog(unsigned long now);
    void drawSpinner(unsigned long now);
    void drawCloudShape(int32_t cx, int32_t cy);

    int32_t drift(unsigned long now, unsigned long period, int32_t offset) const;

    String formatTime(unsigned long unixTime) const;
    String asciiFold(const String& text) const;
    String titleCase(const String& text) const;
  };
}
