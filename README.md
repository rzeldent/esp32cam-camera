# esp32cam-camera

## Simple camera capture and storage on the SD card.

When the ESP32 is turned on it takes a picture and stores it on the SD card.
Then the device goes back to deep sleep...


Special features:
- Does not turn on the flash LED

When the application starts, it will:
- Initialize the flash led, MMC and camera
- read the integer id value from the EEPROM and increase it
- take a picture
- save image file file to the flash
- go back to deep sleep.