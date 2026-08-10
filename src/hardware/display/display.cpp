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
      tft.setTextDatum(middle_center);
      tft.setTextColor(static_cast<uint16_t>(Color::White));
      tft.setFont(size == TextSize::Large ? &fonts::Font4 : &fonts::Font2);
      tft.drawString(text, centerX, centerY);
    }

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
