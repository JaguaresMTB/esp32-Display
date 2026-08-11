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
    // Progress stage for the boot checklist.
    enum class Stage
    {
      Pending,
      Connecting,
      Authorizing,
      Done,
    };

    WeatherScreen(display::IDisplay& display, int32_t timezoneOffsetSeconds, Language language);

    // Valid weather data (success state).
    void render(const weather::WeatherData& data);

    // Keep last valid data, indicate Wi-Fi is offline.
    void renderOffline(const weather::WeatherData& data);

    // Keep last valid data, indicate the last weather update failed.
    void renderUpdateFailed(const weather::WeatherData& data);

    // Update the local timezone offset (seconds) used for the last-update time.
    void setTimezoneOffsetSeconds(int32_t offsetSeconds);

    // No data yet — step-by-step boot checklist (Wi-Fi + weather sections)
    // with attempt numbers. No animation.
    void renderChecklist(Stage wifiStage, int wifiAttempt, Stage weatherStage,
                         int weatherAttempt, const char* ip);

    // Wi-Fi provisioning setup screen.
    void renderProvisioning(const char* apSsid, const char* ip);

    // Called every loop; throttled internally and redraws only the animation
    // zone. Does nothing while no weather data exists (checklist is static).
    void updateAnimation(unsigned long now);

  private:
    static constexpr int32_t kZoneY = 36;
    static constexpr int32_t kZoneH = 74;
    static constexpr unsigned long kFrameIntervalMs = 80;

    enum class Scene
    {
      None,
      Sun,
      Moon,
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
    bool _night = false;
    Scene _scene = Scene::None;
    unsigned long _lastFrame = 0;

    static Scene sceneFor(weather::Condition condition, bool isNight);
    static Stage stepStage(Stage overall, int step);
    static const char* windCardinal(int degrees);
    bool isNight(const weather::WeatherData& data) const;
    display::Color backgroundColor() const;

    void drawHeader(const char* title, display::Color barColor);
    void drawWeatherBody(const weather::WeatherData& data);
    void drawFooter(const String& text, display::Color barColor);
    void drawTextAlignedMetric(const char* label, const String& value, int32_t y);

    void drawChecklistSectionTitle(const char* title, int attempt, int32_t y);
    void drawChecklistRow(const char* label, Stage stage, int attempt, int32_t y);
    void drawCheckMark(int32_t x, int32_t y);

    const char* label(const char* en, const char* es) const;
    String attemptString(int attempt) const;

    void clearZone();
    void drawScene(unsigned long now);
    void drawSun(unsigned long now);
    void drawMoon(unsigned long now);
    void drawClouds(unsigned long now);
    void drawRain(unsigned long now);
    void drawStorm(unsigned long now);
    void drawSnow(unsigned long now);
    void drawFog(unsigned long now);
    void drawCloudShape(int32_t cx, int32_t cy);

    int32_t drift(unsigned long now, unsigned long period, int32_t offset) const;

    String formatTime(unsigned long unixTime) const;
    String asciiFold(const String& text) const;
    String titleCase(const String& text) const;
  };
}
