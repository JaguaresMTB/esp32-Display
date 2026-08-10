#pragma once

#include <Arduino.h>

namespace weather
{
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

    String condition;            // e.g. "Clear"
    String conditionDescription; // e.g. "clear sky"

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
  };

  // Factory: returns the singleton weather service instance.
  IWeatherService& getWeatherService();
}
