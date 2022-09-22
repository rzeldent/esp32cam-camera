#include <Arduino.h>

#include "soc/rtc_cntl_reg.h"

#include <FS.h>
#include <SD_MMC.h>

#include <EEPROM.h>

#include <esp_camera_io.h>

// Change the line below to reflect the camera you are using!
camera_config_t camera_config = esp32cam_aithinker_settings;

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
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  /*
  It is possible to increase resolution but AGC and AWB are not working because they need to settle.
  This can be seen especially in low light conditions; a blueish glare is present or worse!
  */

  // Use UXGA (1600x1200)
  camera_config.frame_size = FRAMESIZE_UXGA;
  camera_config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  log_i("Initialize the camera");
  auto err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    log_e("Camera init failed with error 0x%x", err);
    return;
  }

  log_i("Initialize SD card");
  // Set mode to 1 bit so D4 is not used for SS. Otherwise it would turn on the flash led.
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
}

// put your main code here, to run repeatedly:
void loop()
{
  ulong id;
  // initialize EEPROM. We read just one int; the id
  EEPROM.begin(sizeof(id));
  EEPROM.get(0, id);
  id++;
  EEPROM.put(0, id);
  EEPROM.commit();

  // Take Picture with Camera
  auto fb = esp_camera_fb_get();
  if (fb)
  {
    auto file_name = "/image" + String(id) + ".jpg";
    log_i("Saving image to %s", file_name.c_str());
    auto file = SD_MMC.open(file_name.c_str(), FILE_WRITE);
    if (file)
    {
      file.write(fb->buf, fb->len);
      file.close();
      log_i("Image %s saved", file_name.c_str());

      esp_camera_fb_return(fb);
    }
    else
      log_e("Failed to open file for writing");
  }
  else
    log_e("Camera capture failed");

  digitalWrite(LED_BUILTIN, LOW);
  //  Hold the value during deep sleep
  gpio_hold_en((gpio_num_t)LED_BUILTIN);

  log_i("Going to deep sleep");
  Serial.flush();

  esp_deep_sleep_start();
}