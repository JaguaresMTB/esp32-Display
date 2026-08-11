#include "weather_screen.h"

#include <math.h>
#include <stdio.h>

namespace ui
{
WeatherScreen::WeatherScreen(display::IDisplay& display, int32_t timezoneOffsetSeconds,
                             Language language)
  : _display(display), _tzOffsetSeconds(timezoneOffsetSeconds), _language(language)
{
}

void WeatherScreen::setTimezoneOffsetSeconds(int32_t offsetSeconds)
{
  _tzOffsetSeconds = offsetSeconds;
}

void WeatherScreen::renderChecklist(Stage wifiStage, int wifiAttempt, Stage weatherStage,
                                    int weatherAttempt, const char* ip)
{
  _display.clear();

  _display.fillRect(0, 0, _display.width(), 30, display::Color::Blue);
  _display.drawText(label("Weather", "Clima"), _display.width() / 2, 15, display::TextSize::Medium);

  // Wi-Fi section.
  drawChecklistSectionTitle(label("Wi-Fi", "Wi-Fi"), wifiAttempt, 48);
  drawChecklistRow(label("Connecting", "Conectando"), stepStage(wifiStage, 1), wifiAttempt, 76);
  drawChecklistRow(label("Authorizing", "Autorizando"), stepStage(wifiStage, 2), wifiAttempt, 96);
  drawChecklistRow(label("Connected", "Conectado"), stepStage(wifiStage, 3), wifiAttempt, 116);

  // Weather section.
  drawChecklistSectionTitle(label("Weather", "Clima"), weatherAttempt, 150);
  drawChecklistRow(label("Connecting", "Conectando"), stepStage(weatherStage, 1), weatherAttempt, 178);
  drawChecklistRow(label("Authorizing", "Autorizando"), stepStage(weatherStage, 2), weatherAttempt, 198);
  drawChecklistRow(label("Updated", "Actualizado"), stepStage(weatherStage, 3), weatherAttempt, 218);

  // Footer: IP once Wi-Fi is up, otherwise blank.
  if (ip != nullptr && ip[0] != '\0')
  {
    String ipText = String(label("IP: ", "IP: ")) + ip;
    _display.drawText(ipText.c_str(), _display.width() / 2, 306, display::TextSize::Small);
  }

  _hasData = false;
  _scene = Scene::None;
}

void WeatherScreen::renderProvisioning(const char* apSsid, const char* ip)
{
  _display.clear();

  drawHeader(label("Wi-Fi Setup", "Configuracion Wi-Fi"), display::Color::Orange);
  _display.drawText(label("Connect to:", "Conectate a:"), _display.width() / 2, 130,
                    display::TextSize::Medium);
  _display.drawText(apSsid, _display.width() / 2, 162, display::TextSize::Medium);
  _display.drawText(label("Open:", "Abre:"), _display.width() / 2, 202, display::TextSize::Medium);
  _display.drawText(ip, _display.width() / 2, 234, display::TextSize::Medium);

  _hasData = false;
  _scene = Scene::None;
}

void WeatherScreen::render(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Blue);

  _hasData = true;
  _scene = sceneFor(data.conditionId, isNight(data));

  drawWeatherBody(data);
  drawFooter(String(label("Updated ", "Actualizado ")) + formatTime(data.timestamp),
             display::Color::Blue);
}

void WeatherScreen::renderOffline(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Orange);

  _hasData = true;
  _scene = sceneFor(data.conditionId, isNight(data));

  drawWeatherBody(data);
  drawFooter(String(label("Wi-Fi offline  |  ", "Wi-Fi sin conexion  |  ")) +
                 formatTime(data.timestamp),
             display::Color::Orange);
}

void WeatherScreen::renderUpdateFailed(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Red);

  _hasData = true;
  _scene = sceneFor(data.conditionId, isNight(data));

  drawWeatherBody(data);
  drawFooter(String(label("Update failed  |  ", "Error de actualizacion  |  ")) +
                 formatTime(data.timestamp),
             display::Color::Red);
}

void WeatherScreen::updateAnimation(unsigned long now)
{
  if (!_hasData)
  {
    return; // checklist is static; no animation while no data
  }
  if (now - _lastFrame < kFrameIntervalMs)
  {
    return;
  }
  _lastFrame = now;

  clearZone();
  drawScene(now);
}

WeatherScreen::Scene WeatherScreen::sceneFor(weather::Condition condition, bool isNight)
{
  switch (condition)
  {
    case weather::Condition::Clear:
      return isNight ? Scene::Moon : Scene::Sun;
    case weather::Condition::Clouds: return Scene::Clouds;
    case weather::Condition::Drizzle:
    case weather::Condition::Rain: return Scene::Rain;
    case weather::Condition::Thunderstorm: return Scene::Storm;
    case weather::Condition::Snow: return Scene::Snow;
    case weather::Condition::Mist:
    case weather::Condition::Fog:
    case weather::Condition::Haze:
    case weather::Condition::Smoke:
    case weather::Condition::Dust:
    case weather::Condition::Sand:
    case weather::Condition::Ash: return Scene::Fog;
    default: return Scene::Clouds;
  }
}

const char* WeatherScreen::windCardinal(int degrees)
{
  static const char* points[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
  int index = ((degrees % 360) + 360) % 360;
  index = (index + 22) / 45; // round to nearest 45° sector
  return points[index % 8];
}

bool WeatherScreen::isNight(const weather::WeatherData& data) const
{
  if (data.sunrise == 0 || data.sunset == 0)
  {
    return false; // unknown schedule -> treat as day
  }
  return (data.timestamp < data.sunrise) || (data.timestamp >= data.sunset);
}

void WeatherScreen::drawHeader(const char* title, display::Color barColor)
{
  _display.fillRect(0, 0, _display.width(), 30, barColor);
  _display.drawText(title, _display.width() / 2, 15, display::TextSize::Medium);
}

void WeatherScreen::drawWeatherBody(const weather::WeatherData& data)
{
  {
    String temp = String(data.temperatureC, 1) + " C";
    _display.drawText(temp.c_str(), _display.width() / 2, 136, display::TextSize::XLarge);
  }

  {
    String feels = String(label("Feels like ", "Sensacion ")) + String(data.feelsLikeC, 1) + " C";
    _display.drawText(feels.c_str(), _display.width() / 2, 180, display::TextSize::Medium);
  }

  _display.drawText(titleCase(data.conditionDescription).c_str(), _display.width() / 2, 208,
                    display::TextSize::Medium);

  _display.drawLine(16, 224, _display.width() - 16, 224, display::Color::White);

  drawTextAlignedMetric(label("Humidity", "Humedad"), String(data.humidityPercent) + " %", 246);
  {
    String wind = String(data.windSpeed * 3.6f, 1) + " km/h " + windCardinal(data.windDirection);
    drawTextAlignedMetric(label("Wind", "Viento"), wind, 266);
  }
  drawTextAlignedMetric(label("Rain", "Lluvia"), String(data.rainProbabilityPercent) + " %", 292);
}

void WeatherScreen::drawTextAlignedMetric(const char* label, const String& value, int32_t y)
{
  _display.drawTextAligned(label, 12, y, display::TextSize::Metric, display::TextAlign::Left);
  _display.drawTextAligned(value.c_str(), _display.width() - 12, y, display::TextSize::Metric,
                           display::TextAlign::Right);
}

void WeatherScreen::drawFooter(const String& text, display::Color barColor)
{
  (void)barColor;
  _display.drawText(text.c_str(), _display.width() / 2, 312, display::TextSize::Small);
}

void WeatherScreen::drawChecklistSectionTitle(const char* title, int attempt, int32_t y)
{
  _display.drawTextAligned(title, 12, y, display::TextSize::Medium, display::TextAlign::Left);
  String attemptText = attemptString(attempt);
  if (attemptText.length() > 0)
  {
    _display.drawTextAligned(attemptText.c_str(), _display.width() - 12, y,
                             display::TextSize::Medium, display::TextAlign::Right);
  }
}

void WeatherScreen::drawChecklistRow(const char* label, Stage stage, int attempt, int32_t y)
{
  int32_t ix = 16;
  switch (stage)
  {
    case Stage::Pending:
      _display.fillCircle(ix, y, 4, display::Color::Gray);
      break;
    case Stage::Connecting:
    case Stage::Authorizing:
      _display.fillCircle(ix, y, 5, display::Color::Orange);
      break;
    case Stage::Done:
      drawCheckMark(ix, y);
      break;
  }

  _display.drawTextAligned(label, 32, y, display::TextSize::Small, display::TextAlign::Left);
  String attemptText = attemptString(attempt);
  if (attemptText.length() > 0)
  {
    _display.drawTextAligned(attemptText.c_str(), _display.width() - 12, y,
                             display::TextSize::Small, display::TextAlign::Right);
  }
}

void WeatherScreen::drawCheckMark(int32_t x, int32_t y)
{
  _display.drawLine(x - 6, y, x - 1, y + 5, display::Color::Green);
  _display.drawLine(x - 1, y + 5, x + 6, y - 4, display::Color::Green);
}

WeatherScreen::Stage WeatherScreen::stepStage(Stage overall, int step)
{
  switch (overall)
  {
    case Stage::Pending:
      return Stage::Pending;
    case Stage::Connecting:
      return step == 1 ? Stage::Connecting : Stage::Pending;
    case Stage::Authorizing:
      if (step == 1)
      {
        return Stage::Done;
      }
      return step == 2 ? Stage::Connecting : Stage::Pending;
    case Stage::Done:
      return Stage::Done;
  }
  return Stage::Pending;
}

const char* WeatherScreen::label(const char* en, const char* es) const
{
  return (_language == Language::Spanish) ? es : en;
}

String WeatherScreen::attemptString(int attempt) const
{
  if (attempt < 1)
  {
    return String("");
  }
  return String(label("Attempt ", "Intento ")) + String(attempt);
}

void WeatherScreen::clearZone()
{
  _display.fillRect(0, kZoneY, _display.width(), kZoneH, display::Color::Black);
}

void WeatherScreen::drawScene(unsigned long now)
{
  switch (_scene)
  {
    case Scene::Sun: drawSun(now); break;
    case Scene::Moon: drawMoon(now); break;
    case Scene::Clouds: drawClouds(now); break;
    case Scene::Rain: drawRain(now); break;
    case Scene::Storm: drawStorm(now); break;
    case Scene::Snow: drawSnow(now); break;
    case Scene::Fog: drawFog(now); break;
    default: break;
  }
}

void WeatherScreen::drawSun(unsigned long now)
{
  int32_t cx = _display.width() / 2;
  int32_t cy = kZoneY + kZoneH / 2;

  _display.fillCircle(cx, cy, 18, display::Color::Orange);

  double base = (now / 4000.0) * 2.0 * M_PI;
  for (int k = 0; k < 8; k++)
  {
    double a = base + k * 2.0 * M_PI / 8.0;
    int32_t x0 = cx + (int32_t)(cos(a) * 24.0);
    int32_t y0 = cy + (int32_t)(sin(a) * 24.0);
    int32_t x1 = cx + (int32_t)(cos(a) * 32.0);
    int32_t y1 = cy + (int32_t)(sin(a) * 32.0);
    _display.drawLine(x0, y0, x1, y1, display::Color::Orange);
  }
}

void WeatherScreen::drawMoon(unsigned long now)
{
  int32_t cx = _display.width() / 2;
  int32_t cy = kZoneY + kZoneH / 2;

  // Crescent moon: full circle, then an offset background circle carves it.
  _display.fillCircle(cx, cy, 16, display::Color::White);
  _display.fillCircle(cx + 9, cy - 4, 14, display::Color::Black);

  // A few twinkling stars.
  static const int16_t starX[] = { 28, 214, 58, 184, 120 };
  static const int16_t starY[] = { 50, 46, 66, 58, 52 };
  for (int i = 0; i < 5; i++)
  {
    bool on = ((now / 700) + i) % 2 == 0;
    if (on)
    {
      _display.fillCircle(starX[i], starY[i], 2, display::Color::White);
    }
  }
}

void WeatherScreen::drawClouds(unsigned long now)
{
  drawCloudShape(drift(now, 20, 0), 88);
  drawCloudShape(drift(now, 32, 130), 60);
}

void WeatherScreen::drawRain(unsigned long now)
{
  drawCloudShape(drift(now, 25, 0), 66);

  for (int i = 0; i < 14; i++)
  {
    int32_t x = (i * 31 + (int32_t)(now / 12)) % _display.width();
    int32_t y = kZoneY + ((int32_t)(now / 4) + i * 47) % (kZoneH - 36);
    _display.drawLine(x, y, x - 2, y + 8, display::Color::Cyan);
  }
}

void WeatherScreen::drawStorm(unsigned long now)
{
  drawCloudShape(drift(now, 25, 0), 66);

  if (((now / 250) % 2) == 0)
  {
    int32_t cx = _display.width() / 2;
    int32_t cy = 80;
    _display.fillTriangle(cx - 6, cy - 12, cx + 6, cy - 12, cx, cy + 14, display::Color::Orange);
    _display.fillTriangle(cx - 3, cy + 10, cx + 3, cy + 10, cx, cy + 26, display::Color::Orange);
  }
}

void WeatherScreen::drawSnow(unsigned long now)
{
  drawCloudShape(drift(now, 25, 0), 66);

  for (int i = 0; i < 18; i++)
  {
    int32_t x = (i * 43 + (int32_t)(now / 9)) % _display.width();
    int32_t y = kZoneY + ((int32_t)(now / 12) + i * 61) % (kZoneH - 26);
    _display.fillCircle(x, y, 2, display::Color::White);
  }
}

void WeatherScreen::drawFog(unsigned long now)
{
  for (int band = 0; band < 3; band++)
  {
    int32_t y = kZoneY + 6 + band * 22;
    int32_t x = drift(now, 50 + band * 20, band * 90);
    _display.fillRect(x, y, 100, 10, display::Color::Gray);
  }
}

void WeatherScreen::drawCloudShape(int32_t cx, int32_t cy)
{
  _display.fillEllipse(cx, cy, 28, 12, display::Color::White);
  _display.fillCircle(cx - 18, cy - 2, 12, display::Color::White);
  _display.fillCircle(cx + 16, cy - 4, 10, display::Color::White);
}

int32_t WeatherScreen::drift(unsigned long now, unsigned long period, int32_t offset) const
{
  int32_t span = _display.width() + 140;
  return (int32_t)((now / period) + offset) % span - 70;
}

String WeatherScreen::formatTime(unsigned long unixTime) const
{
  long t = (long)unixTime + _tzOffsetSeconds;
  if (t < 0)
  {
    t += 86400L;
  }
  long secs = t % 86400L;
  int hh = (int)(secs / 3600);
  int mm = (int)((secs % 3600) / 60);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
  return String(buf);
}

// Fold common UTF-8 accented Latin characters to plain ASCII so the built-in
// ASCII fonts render correctly.
String WeatherScreen::asciiFold(const String& text) const
{
  String out;
  out.reserve(text.length());
  size_t i = 0;
  while (i < text.length())
  {
    unsigned char c = (unsigned char)text[i];
    if (c < 0x80)
    {
      out += (char)c;
      i++;
    }
    else if (c == 0xC3 && i + 1 < text.length()) // U+0080..U+00FF
    {
      unsigned char c2 = (unsigned char)text[i + 1];
      switch (c2)
      {
        case 0xA0: out += 'a'; break; // à
        case 0xA1: out += 'a'; break; // á
        case 0xA4: out += 'a'; break; // ä
        case 0xA8: out += 'e'; break; // è
        case 0xA9: out += 'e'; break; // é
        case 0xAA: out += 'e'; break; // ê
        case 0xAD: out += 'i'; break; // í
        case 0xB1: out += 'n'; break; // ñ
        case 0xB3: out += 'o'; break; // ó
        case 0xB6: out += 'o'; break; // ö
        case 0xBA: out += 'u'; break; // ú
        case 0xBC: out += 'u'; break; // ü
        default:   out += '?'; break;
      }
      i += 2;
    }
    else
    {
      // Skip other multi-byte sequences.
      i++;
      while (i < text.length() && ((unsigned char)text[i] & 0xC0) == 0x80)
      {
        i++;
      }
      out += '?';
    }
  }
  return out;
}

// Convert to title case (first letter of each word uppercase, rest lowercase)
// after folding accents to ASCII.
String WeatherScreen::titleCase(const String& text) const
{
  String ascii = asciiFold(text);
  String out;
  out.reserve(ascii.length());
  bool upperNext = true;
  for (size_t i = 0; i < ascii.length(); i++)
  {
    char c = ascii[i];
    if (c == ' ')
    {
      out += c;
      upperNext = true;
    }
    else if (upperNext)
    {
      out += (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
      upperNext = false;
    }
    else
    {
      out += (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
  }
  return out;
}
}
