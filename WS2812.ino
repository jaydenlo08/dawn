// Import
#include <Adafruit_NeoPixel.h>
#include <uRTCLib.h>
#include <IRremote.hpp>
#include "Pins.h"

// Constants
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uRTCLib rtc(0x68);
const char daysOfTheWeek[7][4] PROGMEM = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
struct IRCommand {
  uint16_t code;
  uint32_t color;
};

const IRCommand irCommands[] = {
  {IR_RED_0,  led.Color(255, 0, 0)},
  {IR_RED_1,  led.Color(255, 64, 0)},
  {IR_RED_2,  led.Color(255, 120, 0)},
  {IR_RED_3,  led.Color(255, 180, 0)},
  {IR_RED_4,  led.Color(255, 255, 0)},

  {IR_GREEN_0, led.Color(0, 255, 0)},
  {IR_GREEN_1, led.Color(100, 255, 0)},
  {IR_GREEN_2, led.Color(0, 255, 180)},
  {IR_GREEN_3, led.Color(0, 255, 255)},
  {IR_GREEN_4, led.Color(0, 180, 255)},

  {IR_BLUE_0,  led.Color(0, 0, 255)},
  {IR_BLUE_1,  led.Color(100, 200, 255)},
  {IR_BLUE_2,  led.Color(122, 0, 255)},
  {IR_BLUE_3,  led.Color(255, 0, 255)},
  {IR_BLUE_4,  led.Color(255, 140, 220)},

  {IR_WHITE_0, led.Color(255, 255, 255)},
  {IR_WHITE_1, led.Color(255, 153, 153)},
  {IR_WHITE_2, led.Color(204, 255, 103)},
  {IR_WHITE_3, led.Color(153, 255, 255)},
  {IR_WHITE_4, led.Color(204, 153, 255)}
};

// Settings
bool debug = true;

// Variables
bool alarmRise = false;
bool alarmShine = false;
bool alarmActive = false;
int oldSecond = 0;
uint8_t colour_current_array[3];
int brightness = LED_BRIGHTNESS_DEFAULT;
float t=0;

void setup() {
  Serial.begin(9600);
  URTCLIB_WIRE.begin();
  // Subtract 10s to upload
  //rtc.set(25, 40, 21, 1, 2, 3, 25); // 2025-01-11 06:29:57
  led.begin();
  led.setBrightness(brightness);
  ledOff();
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
}


void sunrise(float t) {
  static uint8_t colour_init_array[3] = {255,0,0};
  static uint8_t colour_final_array[3] = {255,130,0};
  static uint8_t colour_day_array[3] = {255,255,220};
  //if (t >= 1 && t <= 1.01) {
  if (t >= 0 && t <= 1) {
    for (int c = 0; c<3; c++) {
      colour_current_array[c] = round(colour_init_array[c]+(colour_final_array[c]-colour_init_array[c])*t);
    }
    led.setBrightness(round(255*t));
  } else if (t > 1 && t <= 2) {
    for (int c = 0; c<3; c++) {
      colour_current_array[c] = round(colour_final_array[c]+(colour_day_array[c]-colour_final_array[c])*(t-1));
    }
  } else {
    alarmActive = false;
  }
  uint32_t colour_current = ((uint32_t)colour_current_array[0] << 16) | ((uint32_t)colour_current_array[1] << 8) | (uint32_t)colour_current_array[2];
  if (debug == true) {
    Serial.print(t);
    Serial.print(" - 0x");
    Serial.print(colour_current, HEX);
    Serial.println();
  }
  led.fill(colour_current);
  led.show();
}

void ledOff() {
  Serial.println("<ON/OFF>");
  led.clear();
  led.show();
  alarmActive = false;
}

void ledIR(uint16_t receivedCode) {
  uint32_t selectedColor = 0;
  for (uint8_t i = 0; i < sizeof(irCommands)/sizeof(irCommands[0]); i++) {
    if (irCommands[i].code == receivedCode) {
      led.clear();
      selectedColor = irCommands[i].color;
      led.fill(selectedColor);
      led.show();
      break;
    }
  }
}

void loop() {
  rtc.refresh();

  if ((rtc.second() - oldSecond == 1) || oldSecond - rtc.second() == 59 ) {
    if ((rtc.hour() == ALARM_HOUR) && (rtc.minute() == ALARM_MINUTE)) {
      if (rtc.second() == 0) {
        alarmActive = true;
        t = LED_STEP;
      }
    }
    if (alarmActive == true) {
      sunrise(t);
      t = t+LED_STEP;
    }
  }
  if (IrReceiver.decode()) {
    IrReceiver.resume();
    Serial.println(freeMemory());
    delay(10);
    if (IrReceiver.decodedIRData.command == IR_ON_OFF) {
      ledOff();
    } else if (IrReceiver.decodedIRData.command == IR_BRIGHTNESS_UP) {
      if (brightness + LED_BRIGHTNESS_STEP > 255) {
        brightness = 255;
      } else {
        brightness = brightness + LED_BRIGHTNESS_STEP;
      }
      led.setBrightness(brightness);
      Serial.println(brightness);
      led.show();
    } else if (IrReceiver.decodedIRData.command == IR_BRIGHTNESS_DOWN) {
      if (brightness - LED_BRIGHTNESS_STEP < 1) {
        brightness = 1;
      } else {
        brightness = brightness - LED_BRIGHTNESS_STEP;
      }
      Serial.println(brightness);
      led.setBrightness(brightness);
      led.show();
    }
    else {
      ledIR(IrReceiver.decodedIRData.command);
    }
  }
  oldSecond = rtc.second();
}
