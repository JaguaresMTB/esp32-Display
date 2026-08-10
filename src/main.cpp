#include <LovyanGFX.hpp>
#include "tft_config.h"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus.config();
      cfg.spi_host = TFT_SPI_HOST;
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

static LGFX tft;

static void runDisplayTest(void)
{
  Serial.println("Display initialization started");
  bool ok = tft.init();
  if (!ok)
  {
    Serial.println("Display initialization FAILED");
    return;
  }
  Serial.println("Display initialization completed");

  tft.setRotation(TFT_ROTATION);

  Serial.println("Color test started");
  tft.fillScreen(TFT_RED);
  Serial.println("RED");
  delay(2000);

  tft.fillScreen(TFT_GREEN);
  Serial.println("GREEN");
  delay(2000);

  tft.fillScreen(TFT_BLUE);
  Serial.println("BLUE");
  delay(2000);

  tft.fillScreen(TFT_WHITE);
  Serial.println("WHITE");
  delay(2000);

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setFont(&fonts::Font4);
  tft.fillRect(10, 120, tft.width() - 20, 80, TFT_RED);
  tft.drawString("Hello World!", tft.width() / 2, 160, &fonts::Font4);
  Serial.println("Test text displayed");
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Boot started");

  runDisplayTest();
}

void loop(void)
{
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 'r')
    {
      Serial.println("Re-run requested");
      runDisplayTest();
    }
  }

  static unsigned long lastBeat = 0;
  if (millis() - lastBeat >= 2000)
  {
    lastBeat = millis();
    Serial.println("alive");
  }
  delay(50);
}
