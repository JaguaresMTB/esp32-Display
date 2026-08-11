#pragma once

#include <Arduino.h>

namespace weather
{
  // Provider-independent weather condition group (used by the UI for
  // animation/scenes). Mapped from the provider's condition group.
  enum class Condition : uint8_t
  {
    Clear,
    Clouds,
    Drizzle,
    Rain,
    Thunderstorm,
    Snow,
    Mist,
    Fog,
    Haze,
    Smoke,
    Dust,
    Sand,
    Ash,
    Squall,
    Tornado,
    Unknown,
  };

  // Provider-independent weather data model.
  struct WeatherData
  {
    String locationName;
    float latitude = 0.0f;
    float longitude = 0.0f;

    float temperatureC = 0.0f;
    float feelsLikeC = 0.0f;
    int humidityPercent = 0;
    int pressureHpa = 0;

    float windSpeed = 0.0f; // m/s
    int windDirection = 0;  // degrees (meteorological)

    String condition;            // raw condition group, e.g. "Clouds"
    Condition conditionId = Condition::Unknown; // typed group for the UI
    String conditionDescription; // localized description, e.g. "nubes dispersas"

    int rainProbabilityPercent = 0; // probability of precipitation (0-100)

    unsigned long sunrise = 0; // unix seconds (for day/night)
    unsigned long sunset = 0;  // unix seconds

    unsigned long timestamp = 0; // unix seconds
  };

  enum class WeatherError
  {
    Ok,
    NotConfigured,
    NotConnected,
    HttpError,
    TlsError,
    BadStatus,
    ApiError,
    InvalidJson,
    MissingField,
    Timeout,
  };

  // Returns a human-readable name for a WeatherError (for logging/display).
  const char* weatherErrorName(WeatherError error);

  // Weather service abstraction. Application code depends only on this
  // interface and never on a specific provider (e.g. OpenWeather) or on
  // HTTP/JSON details. Implemented in weather.cpp.
  class IWeatherService
  {
  public:
    virtual ~IWeatherService() = default;

    // Initialize the provider (reads configuration).
    virtual bool begin() = 0;

    // Perform one current-weather request (bounded timeout). Fills `data`
    // on success. Returns a meaningful error otherwise; reason is logged
    // internally. Never blocks indefinitely.
    virtual WeatherError getCurrentWeather(WeatherData& data) = 0;

    // Set the request location at runtime (overrides the compile-time
    // defaults). Called with the location resolved from the Wi-Fi network.
    virtual void setLocation(float latitude, float longitude, const char* name) = 0;
  };

  // Factory: returns the singleton weather service instance.
  IWeatherService& getWeatherService();
}
