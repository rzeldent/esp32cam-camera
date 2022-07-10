#include <Arduino.h>

#include "soc/rtc_cntl_reg.h"

#include <FS.h>
#include <SD_MMC.h>

#include <EEPROM.h>

#include <esp_camera.h>

constexpr camera_config_t esp32cam = camera_config_t{.pin_pwdn = -1, .pin_reset = 15, .pin_xclk = 27, .pin_sscb_sda = 25, .pin_sscb_scl = 23, .pin_d7 = 19, .pin_d6 = 36, .pin_d5 = 18, .pin_d4 = 39, .pin_d3 = 5, .pin_d2 = 34, .pin_d1 = 35, .pin_d0 = 17, .pin_vsync = 22, .pin_href = 26, .pin_pclk = 21, .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0, .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_SVGA, .jpeg_quality = 12, .fb_count = 2};
constexpr camera_config_t ai_thinker = camera_config_t{.pin_pwdn = 32, .pin_reset = -1, .pin_xclk = 0, .pin_sscb_sda = 26, .pin_sscb_scl = 27, .pin_d7 = 35, .pin_d6 = 34, .pin_d5 = 39, .pin_d4 = 36, .pin_d3 = 21, .pin_d2 = 19, .pin_d1 = 18, .pin_d0 = 5, .pin_vsync = 25, .pin_href = 23, .pin_pclk = 22, .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_1, .ledc_channel = LEDC_CHANNEL_1, .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_SVGA, .jpeg_quality = 12, .fb_count = 2};
constexpr camera_config_t ttgo_cam = camera_config_t{.pin_pwdn = 26, .pin_reset = -1, .pin_xclk = 32, .pin_sscb_sda = 13, .pin_sscb_scl = 12, .pin_d7 = 39, .pin_d6 = 36, .pin_d5 = 23, .pin_d4 = 18, .pin_d3 = 15, .pin_d2 = 4, .pin_d1 = 14, .pin_d0 = 5, .pin_vsync = 27, .pin_href = 25, .pin_pclk = 19, .xclk_freq_hz = 20000000, .ledc_timer = LEDC_TIMER_0, .ledc_channel = LEDC_CHANNEL_0, .pixel_format = PIXFORMAT_JPEG, .frame_size = FRAMESIZE_SVGA, .jpeg_quality = 12, .fb_count = 2};

camera_config_t camera_config = ai_thinker;

/*
 * SD Card | ESP32
 *    D2       12
 *    D3       13
 *    CMD      15
 *    VSS      GND
 *    VDD      3.3V
 *    CLK      14
 *    VSS      GND
 *    D0       2  (add 1K pull up after flashing)
 *    D1       4
 */

// put your setup code here, to run once:
void setup()
{
  // Disable brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Initialize FLASH_GPIO_NUM pin
  pinMode(FLASH_GPIO_NUM, OUTPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  if (psramFound())
  {
    log_i("PSRAM found. Setting UXGA resolution");
    camera_config.frame_size = FRAMESIZE_UXGA;
    camera_config.jpeg_quality = 10;
    camera_config.fb_count = 1;
  }
  else
  {
    log_i("No PSRAM found. Setting SVGA resolution");
    camera_config.frame_size = FRAMESIZE_SVGA;
    camera_config.jpeg_quality = 12;
    camera_config.fb_count = 1;
  }

  log_i("Initialize the camera");
  auto err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    log_e("Camera init failed with error 0x%x", err);
    return;
  }

  log_i("Initialize SD card");
  // Set mode to 1 bit so D4 is not used for SS. Otherwise it would turn on the flash.
  if (!SD_MMC.begin("/sdcard", true))
  {
    log_e("Initialization of SD Card Failed");
    return;
  }

  auto cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE)
  {
    log_e("No SD Card attached");
    return;
  }

  ulong id;

  // initialize EEPROM. We read just one int; the id
  EEPROM.begin(sizeof(id));
  EEPROM.get(0, id);
  id++;
  EEPROM.put(0, id);
  EEPROM.commit();

  // Turn on flash
  digitalWrite(FLASH_GPIO_NUM, HIGH);
  // Take Picture with Camera
  auto fb = esp_camera_fb_get();
  // Turn off flash
  digitalWrite(FLASH_GPIO_NUM, LOW);
  if (!fb)
  {
    log_e("Camera capture failed");
    return;
  }

  auto file_name = "/image" + String(id) + ".jpg";
  log_i("Saving image to %s", file_name.c_str());
  auto file = SD_MMC.open(file_name.c_str(), FILE_WRITE);
  if (!file)
  {
    log_e("Failed to open file for writing");
    esp_camera_fb_return(fb);
    return;
  }

  file.write(fb->buf, fb->len);
  file.close();
  log_i("Image %s saved", file_name.c_str());

  esp_camera_fb_return(fb);

  digitalWrite(FLASH_GPIO_NUM, LOW);
  // Hold the value during deep sleep
  gpio_hold_en(FLASH_GPIO_NUM);

  log_i("Going to deep sleep");
  Serial.flush();

  esp_deep_sleep_start();
}

// put your main code here, to run repeatedly:
void loop()
{
}