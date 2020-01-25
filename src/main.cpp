#include <Arduino.h>

// TODO (jipp): upload info via web services, list files, delete files, upload files

#define ARDUINOJSON_ENABLE_STD_STRING 1
#define ENABLE_GxEPD2_GFX 0

#include <iostream>

#include <ArduinoJson.h>
#include <Bounce2.h>
#include <DNSServer.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WebServer.h>
#include <WiFi.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

const uint8_t BUTTON_1 = 39;
const uint8_t BUTTON_2 = 37;
const uint8_t BUTTON_3 = 38;

const int SLEEP_TIME = 120; // seconds
const int GRACE_TIME = 10;  // seconds

DNSServer dnsServer;
WebServer webServer(80);

GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=5*/ 5, /*DC=*/17, /*RST=*/16, /*BUSY=*/4));

DynamicJsonDocument jsonDocConfig(2048);

Bounce button1 = Bounce();
Bounce button2 = Bounce();
Bounce button3 = Bounce();
Ticker goToSleep = Ticker();

const char *filename = "/config.json";

static const uint16_t input_buffer_pixels = 800;      // may affect performance
static const uint16_t max_row_width = 800;            // for up to 7.5" display 800x480
static const uint16_t max_palette_pixels = 256;       // for depth <= 8
uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8];    // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8];   // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

// bmp handling
uint16_t read16(fs::File &f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB

  return result;
}

uint32_t read32(fs::File &f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB

  return result;
}

int drawBitmap(const char *filename, int16_t x, int16_t y, bool with_color)
{
  fs::File file;
  bool valid = false; // valid format to be handled
  bool flip = true;   // bitmap is stored bottom-to-top
  uint32_t startTime = millis();
  uint32_t value;

  if ((x >= display.width()) || (y >= display.height()))
    return 0;

  if (!SPIFFS.exists(filename))
  {
    std::cout << "File not found" << std::endl;

    return 0;
  }

  std::cout << "Loading image '" << filename << '\'' << std::endl;
  file = SPIFFS.open(filename, FILE_READ);

  // Parse BMP header
  if (read16(file) == 0x4D42)
  { // BMP signature
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file);
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);

    if ((planes == 1) && ((format == 0) || (format == 3)))
    { // uncompressed is handled, 565 also
      std::cout << "File size: " << fileSize << std::endl;
      std::cout << "Image Offset: " << imageOffset << std::endl;
      std::cout << "Header size: " << headerSize << std::endl;
      std::cout << "Bit Depth: " << depth << std::endl;
      std::cout << "Image size: " << width << 'x' << height << std::endl;
      // BMP rows are padded (if needed) to 4-byte boundary
      value = width;
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;

      if (depth < 8)
      {
        rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      }

      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;

      if ((x + w - 1) >= display.width())
      {
        w = display.width() - x;
      }
      if ((y + h - 1) >= display.height())
      {
        h = display.height() - y;
      }
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish, colored;

      if (depth == 1)
      {
        with_color = false;
      }
      if (depth <= 8)
      {
        if (depth < 8)
        {
          bitmask >>= depth;
        }
        file.seek(54); //palette is always @ 54

        for (uint16_t pn = 0; pn < (1 << depth); pn++)
        {
          blue = file.read();
          green = file.read();
          red = file.read();
          file.read();
          whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));
          // reddish or yellowish?
          if (0 == pn % 8)
          {
            mono_palette_buffer[pn / 8] = 0;
          }
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;

          if (0 == pn % 8)
          {
            color_palette_buffer[pn / 8] = 0;
          }
          color_palette_buffer[pn / 8] |= colored << pn % 8;
        }
      }

      display.fillScreen(GxEPD_WHITE);
      uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;

      for (uint16_t row = 0; row < h; row++, rowPosition += rowSize)
      { // for each line
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0; // for depth <= 8
        uint8_t in_bits = 0; // for depth <= 8
        uint16_t color = GxEPD_WHITE;
        file.seek(rowPosition);

        for (uint16_t col = 0; col < w; col++)
        { // for each pixel
          // Time to read more pixel data?

          if (in_idx >= in_bytes)
          { // ok, exact match for 24bit also (size IS multiple of 3)
            in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
            in_remain -= in_bytes;
            in_idx = 0;
          }

          switch (depth)
          {
          case 24:
            blue = input_buffer[in_idx++];
            green = input_buffer[in_idx++];
            red = input_buffer[in_idx++];
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
            break;
          case 16:
          {
            uint8_t lsb = input_buffer[in_idx++];
            uint8_t msb = input_buffer[in_idx++];
            if (format == 0)
            { // 555
              blue = (lsb & 0x1F) << 3;
              green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
              red = (msb & 0x7C) << 1;
            }
            else
            { // 565
              blue = (lsb & 0x1F) << 3;
              green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
              red = (msb & 0xF8);
            }
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
          }
          break;
          case 1:
          case 4:
          case 8:
          {
            if (0 == in_bits)
            {
              in_byte = input_buffer[in_idx++];
              in_bits = 8;
            }
            uint16_t pn = (in_byte >> bitshift) & bitmask;
            whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
            colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
            in_byte <<= depth;
            in_bits -= depth;
          }
          break;
          }
          if (whitish)
          {
            color = GxEPD_WHITE;
          }
          else if (colored && with_color)
          {
            color = GxEPD_RED;
          }
          else
          {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.drawPixel(x + col, yrow, color);
        } // end pixel
      }   // end line

      std::cout << "loaded in " << millis() - startTime << " ms" << std::endl;
    }
  }

  file.close();

  if (!valid)
  {
    std::cout << "bitmap format not handled." << std::endl;
    return 0;
  }

  return value;
}

void deepSleep()
{
  std::cout << "going to sleep!" << std::endl;
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 0);
  esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000);
  esp_deep_sleep_start();
}

bool loadConfiguration(const char *filename, DynamicJsonDocument &jsonDoc)
{
  fs::File file;

  std::cout << "load configuration" << std::endl;

  if (SPIFFS.exists(filename))
  {
    std::cout << "open file" << std::endl;
    file = SPIFFS.open(filename, FILE_READ);
  }
  else
  {
    std::cout << "file does not exist" << std::endl;
  }

  if (deserializeJson(jsonDoc, file))
  {
    std::cout << "Failed to read file, using default configuration" << std::endl;
    jsonDoc["active"] = 0;
    jsonDoc["screens"][0]["bmp"] = "";
    jsonDoc["screens"][0]["title"] = "title0";
    jsonDoc["screens"][0]["text"][0] = "text0";
    jsonDoc["screens"][0]["text"][1] = "text1";

    return false;
  }

  file.close();

  return true;
}

void saveConfiguration(const char *filename, DynamicJsonDocument &jsonDoc)
{
  fs::File file;

  std::cout << "save configuration" << std::endl;

  SPIFFS.remove(filename);
  file = SPIFFS.open(filename, FILE_WRITE);

  if (!file)
  {
    std::cout << "Failed to create file" << std::endl;
  }

  if (serializeJsonPretty(jsonDoc, file) == 0)
  {
    std::cout << "Failed to write to file" << std::endl;
  }

  file.close();
}

void printFile(const char *filename)
{
  fs::File file = SPIFFS.open(filename, FILE_READ);

  if (!file)
  {
    std::cout << "Failed to read file" << std::endl;
    return;
  }

  while (file.available())
  {
    std::cout << (char)file.read();
  }

  std::cout << std::endl;

  file.close();
}

void displayScreen(DynamicJsonDocument &jsonDoc, int active, int total)
{
  JsonArray array;
  int line = 1;
  int offsetFont = 20;
  int offsetPicture = 8;

  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  offsetPicture += drawBitmap(jsonDoc["screens"][jsonDoc["active"].as<signed int>()]["bmp"].as<char *>(), 0, 0, true);

  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(offsetPicture, 18 + offsetFont);
  display.println(jsonDoc["screens"][jsonDoc["active"].as<signed int>()]["title"].as<char *>());

  display.setFont(&FreeMonoBold12pt7b);

  array = jsonDoc["screens"][jsonDoc["active"].as<signed int>()]["text"];

  for (JsonVariant v : array)
  {
    line++;
    display.setCursor(offsetPicture, 30 + line * offsetFont);
    display.println(v.as<char *>());
  }

  display.setFont(&FreeMono9pt7b);
  display.setCursor(display.width() - 40, display.height() - 10);
  display.print(active);
  display.print("/");
  display.println(total);

  display.display(false);

  display.hibernate();
}

void switchScreen(DynamicJsonDocument &jsonDoc, int increment)
{
  int total = jsonDoc["screens"].size();
  int active = jsonDoc["active"].as<signed int>();

  active = (active + increment) % total;

  if (active < 0)
  {
    active = total - 1;
  }

  jsonDoc["active"] = active;
  std::cout << "screen: " << active + 1 << "/" << total << std::endl;

  displayScreen(jsonDoc, active + 1, total);
}

void handleNotFound()
{
  webServer.send(404, "text/plain", "File Not Found");
}

void startNetwork()
{
  std::cout << "start Network" << std::endl;
  WiFi.softAP("theBadge");
  dnsServer.start(53, "*", WiFi.softAPIP());

  webServer.onNotFound(handleNotFound);
  webServer.serveStatic("/index.html", SPIFFS, "/www/index.html");
  webServer.begin();
}

void setup()
{
  Serial.begin(SPEED);

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  btStop();
  WiFi.mode(WIFI_OFF);
  display.init(SPEED);

  if (!SPIFFS.begin())
    std::cout << "SPIFFS problem!" << std::endl;

  button1.attach(BUTTON_1);
  button1.interval(5);
  button2.attach(BUTTON_2);
  button2.interval(5);
  button3.attach(BUTTON_3);
  button3.interval(5);

  if (!loadConfiguration(filename, jsonDocConfig))
  {
    saveConfiguration(filename, jsonDocConfig);
  }

  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_TIMER:
    std::cout << "timer wakepup" << std::endl;
    switchScreen(jsonDocConfig, 1);
    saveConfiguration(filename, jsonDocConfig);
    deepSleep();
    break;
  case ESP_SLEEP_WAKEUP_EXT0:
    std::cout << "ext0 wakepup" << std::endl;
    switchScreen(jsonDocConfig, 1);
    saveConfiguration(filename, jsonDocConfig);
    deepSleep();
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    std::cout << "ext1 wakepup" << std::endl;
    break;
  default:
    std::cout << "normal wakepup" << std::endl;
  }

  printFile(filename);

  switchScreen(jsonDocConfig, 0);

  goToSleep.once(GRACE_TIME, deepSleep);
}

void loop()
{
  if (WiFi.getMode() == WIFI_AP)
  {
    dnsServer.processNextRequest();
    webServer.handleClient();
  }

  button1.update();

  if (button1.fell())
  {
    switchScreen(jsonDocConfig, 1);
    saveConfiguration(filename, jsonDocConfig);
  }

  button2.update();

  if (button2.fell())
  {
  }

  button3.update();

  if (button3.fell())
  {
    std::cout << "timer off" << std::endl;
    goToSleep.detach();
    startNetwork();
  }
}