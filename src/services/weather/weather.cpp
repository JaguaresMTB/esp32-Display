#include "weather.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "common/logging.h"
#include "networking/http.h"
#include "networking/network.h"

// Real credentials come from src/config/weather_credentials.h (gitignored).
// Without that file the build still succeeds with an empty key.
#if __has_include("config/weather_credentials.h")
#include "config/weather_credentials.h"
#endif

#ifndef OPENWEATHER_API_KEY
#define OPENWEATHER_API_KEY ""
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
#ifndef WEATHER_LANG
#define WEATHER_LANG ""
#endif

namespace weather
{
namespace
{
  const char* TAG = "WEATHER";
  const char* CURRENT_WEATHER_URL = "https://api.openweathermap.org/data/2.5/weather";
  const char* FORECAST_URL = "https://api.openweathermap.org/data/2.5/forecast";
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
  class OpenWeatherProvider final : public IWeatherService
  {
  public:
    bool begin() override
    {
      logging::info(TAG, "initialization");
      _configured = OPENWEATHER_API_KEY[0] != '\0';
      if (!_configured)
      {
        logging::info(TAG, "no API key configured; weather disabled");
      }
      return true;
    }

    WeatherError getCurrentWeather(WeatherData& data) override
    {
      if (!_configured)
      {
        logging::info(TAG, "request aborted: not configured");
        return WeatherError::NotConfigured;
      }

      networking::INetwork& network = networking::getNetwork();
      if (!network.isConnected())
      {
        logging::info(TAG, "request aborted: not connected");
        return WeatherError::NotConnected;
      }

      String currentUrl = buildUrl(CURRENT_WEATHER_URL, false);
      String forecastUrl = buildUrl(FORECAST_URL, true);

      logging::info(TAG, "requesting current weather");

      // Current observed conditions (retry once on transient failure).
      http::Response response;
      WeatherError err = fetch(currentUrl, response, 2);
      if (err != WeatherError::Ok)
      {
        return err;
      }

      err = parse(response.body, data);
      if (err != WeatherError::Ok)
      {
        return err;
      }

      // Rain probability comes from the forecast API; optional (not a hard
      // failure if the forecast call fails).
      http::Response forecast;
      if (fetch(forecastUrl, forecast, 1) == WeatherError::Ok)
      {
        data.rainProbabilityPercent = parseRainProbability(forecast.body);
      }
      else
      {
        logging::info(TAG, "rain probability unavailable; using 0");
        data.rainProbabilityPercent = 0;
      }

      return WeatherError::Ok;
    }

  private:
    bool _configured = false;
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
          if (response.statusCode == 401)
          {
            logging::info(TAG, "invalid API key (401)");
            return WeatherError::ApiError;
          }
          return WeatherError::BadStatus;
        }
        return WeatherError::Ok;
      }
      return WeatherError::HttpError;
    }

    String buildUrl(const char* base, bool forecast) const
    {
      String url = String(base);
      url += "?lat=";
      url += String(_hasLocation ? _lat : WEATHER_LATITUDE, 6);
      url += "&lon=";
      url += String(_hasLocation ? _lon : WEATHER_LONGITUDE, 6);
      url += "&units=metric";
      if (forecast)
      {
        url += "&cnt=1";
      }
      if (WEATHER_LANG[0] != '\0')
      {
        url += "&lang=";
        url += WEATHER_LANG;
      }
      url += "&appid=";
      url += OPENWEATHER_API_KEY;
      return url;
    }

    static Condition toCondition(const String& group)
    {
      if (group == "Clear")        return Condition::Clear;
      if (group == "Clouds")       return Condition::Clouds;
      if (group == "Drizzle")      return Condition::Drizzle;
      if (group == "Rain")         return Condition::Rain;
      if (group == "Thunderstorm") return Condition::Thunderstorm;
      if (group == "Snow")         return Condition::Snow;
      if (group == "Mist")         return Condition::Mist;
      if (group == "Fog")          return Condition::Fog;
      if (group == "Haze")         return Condition::Haze;
      if (group == "Smoke")        return Condition::Smoke;
      if (group == "Dust")         return Condition::Dust;
      if (group == "Sand")         return Condition::Sand;
      if (group == "Ash")          return Condition::Ash;
      if (group == "Squall")       return Condition::Squall;
      if (group == "Tornado")      return Condition::Tornado;
      return Condition::Unknown;
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

      // If the API returned an error envelope, it carries "cod"/"message".
      if (doc["cod"].is<int>() && (int)doc["cod"] != 200)
      {
        logging::info(TAG, "API error: cod=%d message=%s",
                      (int)doc["cod"], (const char*)(doc["message"] | ""));
        return WeatherError::ApiError;
      }

      JsonObject main = doc["main"];
      JsonObject wind = doc["wind"];

      if (!main["temp"].is<float>() || !main["feels_like"].is<float>() ||
          !main["humidity"].is<int>() || !doc["name"].is<const char*>())
      {
        logging::info(TAG, "parse failed: missing required field(s)");
        return WeatherError::MissingField;
      }

      data.locationName = doc["name"] | _name;
      data.latitude = doc["coord"]["lat"] | 0.0f;
      data.longitude = doc["coord"]["lon"] | 0.0f;

      data.temperatureC = main["temp"] | 0.0f;
      data.feelsLikeC = main["feels_like"] | 0.0f;
      data.humidityPercent = main["humidity"] | 0;
      data.pressureHpa = main["pressure"] | 0;

      data.windSpeed = wind["speed"] | 0.0f;
      data.windDirection = wind["deg"] | 0;

      data.condition = doc["weather"][0]["main"] | "";
      data.conditionId = toCondition(data.condition);
      data.conditionDescription = doc["weather"][0]["description"] | "";

      data.sunrise = doc["sys"]["sunrise"] | 0UL;
      data.sunset = doc["sys"]["sunset"] | 0UL;
      data.timestamp = doc["dt"] | 0UL; // observation time (~now)

      return WeatherError::Ok;
    }

    int parseRainProbability(const String& body) const
    {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, body);
      if (err)
      {
        return 0;
      }
      // pop is 0..1 -> percent.
      return (int)((doc["list"][0]["pop"] | 0.0f) * 100.0f + 0.5f);
    }
  };
}

IWeatherService& getWeatherService()
{
  static OpenWeatherProvider instance;
  return instance;
}
}
