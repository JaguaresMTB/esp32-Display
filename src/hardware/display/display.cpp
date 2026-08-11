#include "display.h"

#include <LovyanGFX.hpp>

#include "config/pins.h"

namespace display
{
namespace
{
  class LGFX : public lgfx::LGFX_Device
  {
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI _bus;

  public:
    LGFX()
    {
      {
        auto cfg = _bus.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = TFT_SPI_MODE;
        cfg.freq_write = TFT_FREQ_WRITE;
        cfg.freq_read = TFT_FREQ_READ;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = TFT_SCLK;
        cfg.pin_mosi = TFT_MOSI;
        cfg.pin_miso = -1;
        cfg.pin_dc = TFT_DC;
        _bus.config(cfg);
        _panel.setBus(&_bus);
      }

      {
        auto cfg = _panel.config();
        cfg.pin_cs = TFT_CS;
        cfg.pin_rst = TFT_RST;
        cfg.panel_width = TFT_WIDTH;
        cfg.panel_height = TFT_HEIGHT;
        cfg.offset_x = TFT_OFFSET_X;
        cfg.offset_y = TFT_OFFSET_Y;
        cfg.invert = TFT_INVERT;
        cfg.rgb_order = TFT_RGB_ORDER;
        _panel.config(cfg);
      }

      setPanel(&_panel);
    }
  };

  LGFX tft;

  class St7789Display final : public IDisplay
  {
  public:
    bool init() override
    {
      bool ok = tft.init();
      if (ok)
      {
        tft.setRotation(TFT_ROTATION);
      }
      return ok;
    }

    void clear() override
    {
      tft.fillScreen(static_cast<uint16_t>(Color::Black));
    }

    void fillScreen(Color color) override
    {
      tft.fillScreen(static_cast<uint16_t>(color));
    }

    void fillRect(int32_t x, int32_t y, int32_t width, int32_t height, Color color) override
    {
      tft.fillRect(x, y, width, height, static_cast<uint16_t>(color));
    }

    void drawText(const char* text, int32_t centerX, int32_t centerY, TextSize size) override
    {
      tft.setTextColor(static_cast<uint16_t>(_textColor));
      setTextDatum(TextAlign::Center);
      setFontAndSize(size);
      tft.drawString(text, centerX, centerY);
    }

    void drawTextAligned(const char* text, int32_t x, int32_t y, TextSize size, TextAlign align) override
    {
      tft.setTextColor(static_cast<uint16_t>(_textColor));
      setTextDatum(align);
      setFontAndSize(size);
      tft.drawString(text, x, y);
    }

    void setTextColor(Color color) override
    {
      _textColor = color;
    }

    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color) override
    {
      tft.drawLine(x0, y0, x1, y1, static_cast<uint16_t>(color));
    }

    void fillCircle(int32_t cx, int32_t cy, int32_t radius, Color color) override
    {
      tft.fillCircle(cx, cy, radius, static_cast<uint16_t>(color));
    }

    void fillEllipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, Color color) override
    {
      tft.fillEllipse(cx, cy, rx, ry, static_cast<uint16_t>(color));
    }

    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color color) override
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, static_cast<uint16_t>(color));
    }

  private:
    void setFontAndSize(TextSize size)
    {
      switch (size)
      {
        case TextSize::Small:
          tft.setFont(&fonts::Font2);
          tft.setTextSize(1);
          break;
        case TextSize::Metric:
          tft.setFont(&fonts::Font2);
          tft.setTextSize(1.25f); // 8x16 * 1.25 = 10x20 px
          break;
        case TextSize::Medium:
          tft.setFont(&fonts::Font4);
          tft.setTextSize(1);
          break;
        case TextSize::Large:
          tft.setFont(&fonts::Font4);
          tft.setTextSize(2);
          break;
        case TextSize::XLarge:
          tft.setFont(&fonts::Font7);
          tft.setTextSize(1);
          break;
      }
    }

    void setTextDatum(TextAlign align)
    {
      switch (align)
      {
        case TextAlign::Left:   tft.setTextDatum(middle_left);   break;
        case TextAlign::Center: tft.setTextDatum(middle_center); break;
        case TextAlign::Right:  tft.setTextDatum(middle_right);  break;
      }
    }

  private:
    Color _textColor = Color::White;

  public:
    int32_t width() const override { return tft.width(); }
    int32_t height() const override { return tft.height(); }
  };
}

IDisplay& getDisplay()
{
  static St7789Display instance;
  return instance;
}
}
