#include "mbed.h"
#include "ChainableLED.h"

const PinName clockPin = D6;
const PinName dataPin = D7;
const uint32_t ledCount = 8;
 
ChainableLED leds = ChainableLED(clockPin, dataPin, ledCount);

int main() {
  while (true) {
    for (uint32_t i = 0; i < ledCount; i++) {
      leds.setColorRGB(i, 255, 0, 0);
    }
    wait(1);

    for (uint32_t i = 0; i < ledCount; i++) {
      leds.setColorRGB(i, 0, 255, 0);
    }
    wait(1);

    for (uint32_t i = 0; i < ledCount; i++) {
      leds.setColorRGB(i, 0, 0, 255);
    }
    wait(1);
  }
}
