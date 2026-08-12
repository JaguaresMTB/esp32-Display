#include "weather.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "common/logging.h"
#include "networking/http.h"
#include "networking/network.h"

// Location fallback comes from src/config/weather_credentials.h (gitignored);
// the build succeeds without it (coordinates default to 0 -> GeoIP normally
// supplies the real location at runtime).
#if __has_include("config/weather_credentials.h")
#include "config/weather_credentials.h"
#endif

#ifndef WEATHER_LATITUDE
#define WEATHER_LATITUDE 0.0f
#endif
#ifndef WEATHER_LONGITUDE
#define WEATHER_LONGITUDE 0.0f
#endif
#ifndef WEATHER_LOCATION_NAME
#define WEATHER_LOCATION_NAME ""
#endif

namespace weather
{
namespace
{
  const char* TAG = "WEATHER";
  const char* OPEN_METEO_BASE_URL = "https://api.open-meteo.com/v1/forecast";
}

const char* weatherErrorName(WeatherError error)
{
  switch (error)
  {
    case WeatherError::Ok:           return "OK";
    case WeatherError::NotConfigured: return "NOT_CONFIGURED";
    case WeatherError::NotConnected:  return "NOT_CONNECTED";
    case WeatherError::HttpError:     return "HTTP_ERROR";
    case WeatherError::TlsError:      return "TLS_ERROR";
    case WeatherError::BadStatus:     return "BAD_STATUS";
    case WeatherError::ApiError:      return "API_ERROR";
    case WeatherError::InvalidJson:   return "INVALID_JSON";
    case WeatherError::MissingField:  return "MISSING_FIELD";
    case WeatherError::Timeout:       return "TIMEOUT";
  }
  return "UNKNOWN";
}

namespace
{
  // WMO weather code -> provider-independent condition group.
  Condition toCondition(int code)
  {
    if (code == 0)                return Condition::Clear;
    if (code >= 1 && code <= 3)   return Condition::Clouds;
    if (code == 45 || code == 48) return Condition::Fog;
    if (code >= 51 && code <= 57) return Condition::Drizzle;
    if (code >= 61 && code <= 67) return Condition::Rain;
    if (code >= 71 && code <= 77) return Condition::Snow;
    if (code >= 80 && code <= 82) return Condition::Rain;
    if (code == 85 || code == 86) return Condition::Snow;
    if (code >= 95 && code <= 99) return Condition::Thunderstorm;
    return Condition::Unknown;
  }

  const char* codeDescription(int code)
  {
    if (code == 0)                return "clear sky";
    if (code >= 1 && code <= 3)   return "cloudy";
    if (code == 45 || code == 48) return "fog";
    if (code >= 51 && code <= 57) return "drizzle";
    if (code >= 61 && code <= 67) return "rain";
    if (code >= 71 && code <= 77) return "snow";
    if (code >= 80 && code <= 82) return "rain showers";
    if (code == 85 || code == 86) return "snow showers";
    if (code >= 95 && code <= 99) return "thunderstorm";
    return "unknown";
  }

  class OpenMeteoProvider final : public IWeatherService
  {
  public:
    bool begin() override
    {
      logging::info(TAG, "initialization (Open-Meteo)");
      return true;
    }

    WeatherError getCurrentWeather(WeatherData& data) override
    {
      networking::INetwork& network = networking::getNetwork();
      if (!network.isConnected())
      {
        logging::info(TAG, "request aborted: not connected");
        return WeatherError::NotConnected;
      }

      String url = buildUrl();
      logging::info(TAG, "requesting current weather");

      http::Response response;
      WeatherError err = fetch(url, response, 2);
      if (err != WeatherError::Ok)
      {
        return err;
      }

      return parse(response.body, data);
    }

  private:
    bool _hasLocation = false;
    float _lat = 0.0f;
    float _lon = 0.0f;
    String _name;

    void setLocation(float latitude, float longitude, const char* name) override
    {
      _lat = latitude;
      _lon = longitude;
      _name = name;
      _hasLocation = true;
      logging::info(TAG, "location set: %s (%.4f, %.4f)", name, latitude, longitude);
    }

    WeatherError fetch(const String& url, http::Response& response, int maxAttempts) const
    {
      for (int attempt = 1; attempt <= maxAttempts; attempt++)
      {
        http::SecureClient client;
        if (!client.get(url, response))
        {
          if (attempt < maxAttempts)
          {
            logging::info(TAG, "request failed (HTTP/TLS/timeout), retrying once");
            continue;
          }
          logging::info(TAG, "request failed: HTTP/TLS/timeout error");
          return WeatherError::HttpError;
        }
        if (response.statusCode != 200)
        {
          logging::info(TAG, "request failed: HTTP status %d", response.statusCode);
          return WeatherError::BadStatus;
        }
        return WeatherError::Ok;
      }
      return WeatherError::HttpError;
    }

    String buildUrl() const
    {
      String url = String(OPEN_METEO_BASE_URL);
      url += "?latitude=";
      url += String(_hasLocation ? _lat : WEATHER_LATITUDE, 6);
      url += "&longitude=";
      url += String(_hasLocation ? _lon : WEATHER_LONGITUDE, 6);
      url += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl";
      url += "&hourly=precipitation_probability";
      url += "&daily=sunrise,sunset";
      url += "&wind_speed_unit=ms";
      url += "&timeformat=unixtime";
      url += "&timezone=auto";
      url += "&forecast_days=1";
      return url;
    }

    WeatherError parse(const String& body, WeatherData& data) const
    {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, body);
      if (err)
      {
        logging::info(TAG, "parse failed: invalid JSON (%s)", err.c_str());
        return WeatherError::InvalidJson;
      }

      JsonObject current = doc["current"];
      if (!current["temperature_2m"].is<float>() || !current["relative_humidity_2m"].is<int>() ||
          !current["weather_code"].is<int>())
      {
        logging::info(TAG, "parse failed: missing required field(s)");
        return WeatherError::MissingField;
      }

      data.temperatureC = current["temperature_2m"] | 0.0f;
      data.feelsLikeC = current["apparent_temperature"] | 0.0f;
      data.humidityPercent = current["relative_humidity_2m"] | 0;
      data.pressureHpa = (int)(current["pressure_msl"] | 0.0f);
      data.windSpeed = current["wind_speed_10m"] | 0.0f; // m/s (wind_speed_unit=ms)
      data.windDirection = current["wind_direction_10m"] | 0;

      int code = current["weather_code"] | 0;
      data.condition = codeDescription(code);
      data.conditionId = toCondition(code);
      data.conditionDescription = codeDescription(code);

      data.timestamp = current["time"] | 0UL; // observation time (unix, ~now)
      data.sunrise = doc["daily"]["sunrise"][0] | 0UL;
      data.sunset = doc["daily"]["sunset"][0] | 0UL;

      data.rainProbabilityPercent = currentHourRainProbability(doc);

      data.locationName = _name;
      if (data.locationName.length() == 0)
      {
        data.locationName = WEATHER_LOCATION_NAME;
      }
      data.latitude = _hasLocation ? _lat : WEATHER_LATITUDE;
      data.longitude = _hasLocation ? _lon : WEATHER_LONGITUDE;

      return WeatherError::Ok;
    }

    // The hourly array starts at local midnight; pick the slot nearest the
    // current observation time for the current-hour rain probability.
    int currentHourRainProbability(const JsonDocument& doc) const
    {
      JsonArrayConst times = doc["hourly"]["time"].as<JsonArrayConst>();
      JsonArrayConst probs = doc["hourly"]["precipitation_probability"].as<JsonArrayConst>();
      if (times.isNull() || probs.isNull())
      {
        return 0;
      }
      unsigned long now = doc["current"]["time"] | 0UL;
      int best = -1;
      unsigned long bestDiff = 0xFFFFFFFFUL;
      int n = times.size();
      for (int i = 0; i < n; i++)
      {
        unsigned long t = times[i] | 0UL;
        unsigned long diff = (t > now) ? (t - now) : (now - t);
        if (diff < bestDiff)
        {
          bestDiff = diff;
          best = i;
        }
      }
      if (best < 0 || best >= probs.size())
      {
        return 0;
      }
      return probs[best] | 0;
    }
  };
}

IWeatherService& getWeatherService()
{
  static OpenMeteoProvider instance;
  return instance;
}
}
