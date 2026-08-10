#include "weather_screen.h"

#include <stdio.h>

namespace ui
{
WeatherScreen::WeatherScreen(display::IDisplay& display, int32_t timezoneOffsetSeconds)
  : _display(display), _tzOffsetSeconds(timezoneOffsetSeconds)
{
}

void WeatherScreen::renderLoading()
{
  _display.clear();
  drawHeader("Weather", display::Color::Blue);
  _display.drawText("Updating...", _display.width() / 2, 150, display::TextSize::Large);
}

void WeatherScreen::render(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Blue);
  drawWeatherBody(data);
  drawFooter("Updated " + formatTime(data.timestamp), display::Color::Blue);
}

void WeatherScreen::renderOffline(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Yellow);
  drawWeatherBody(data);
  drawFooter("Wi-Fi offline  |  " + formatTime(data.timestamp), display::Color::Yellow);
}

void WeatherScreen::renderUpdateFailed(const weather::WeatherData& data)
{
  _display.clear();
  drawHeader(titleCase(data.locationName).c_str(), display::Color::Red);
  drawWeatherBody(data);
  drawFooter("Update failed  |  " + formatTime(data.timestamp), display::Color::Red);
}

void WeatherScreen::drawHeader(const char* title, display::Color barColor)
{
  _display.fillRect(0, 0, _display.width(), 30, barColor);
  _display.drawText(title, _display.width() / 2, 15, display::TextSize::Small);
}

void WeatherScreen::drawWeatherBody(const weather::WeatherData& data)
{
  // Temperature (hero).
  {
    String temp = String(data.temperatureC, 1) + " C";
    _display.drawText(temp.c_str(), _display.width() / 2, 105, display::TextSize::XLarge);
  }

  // Feels-like.
  {
    String feels = "Feels like " + String(data.feelsLikeC, 1) + " C";
    _display.drawText(feels.c_str(), _display.width() / 2, 150, display::TextSize::Small);
  }

  // Condition.
  _display.drawText(titleCase(data.conditionDescription).c_str(), _display.width() / 2, 182,
                    display::TextSize::Large);

  // Separator.
  _display.drawLine(16, 205, _display.width() - 16, 205, display::Color::White);

  // Supporting metrics.
  drawTextAlignedMetric("Humidity", String(data.humidityPercent) + " %", 225);
  drawTextAlignedMetric("Wind", String(data.windSpeed, 1) + " m/s", 246);
  drawTextAlignedMetric("Direction", String(data.windDirection) + " deg", 267);
}

void WeatherScreen::drawTextAlignedMetric(const char* label, const String& value, int32_t y)
{
  _display.drawTextAligned(label, 16, y, display::TextSize::Small, display::TextAlign::Left);
  _display.drawTextAligned(value.c_str(), _display.width() - 16, y, display::TextSize::Small,
                           display::TextAlign::Right);
}

void WeatherScreen::drawFooter(const String& text, display::Color barColor)
{
  _display.drawText(text.c_str(), _display.width() / 2, 296, display::TextSize::Small);
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
