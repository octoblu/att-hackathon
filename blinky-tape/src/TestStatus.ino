#include <FastSPI_LED2.h>
#include <Animation.h>

#define LED_COUNT 60
#define MAX_BRIGHTNESS 93
#define PIN_SIGNAL 13
#define INTERVAL 50 // In milliseconds

struct CRGB leds[LED_COUNT];
int mLevel;
int mActualLevel;
unsigned long previousMillis;
unsigned long currentMillis;

int charToInt(char c){
  int i = (int) c;
  if(i < 48 || 58 < i){
    return -1;
  }
  return (i - 48) * 6;
}

void decLevel(){
  setLevel(mLevel - 1);
}

void rainbow() {
  int interval = 254 / LED_COUNT;
  for(int i=0; i < LED_COUNT; i += 1) {
    if(i < mActualLevel){
      leds[i] = CRGB((i * interval), 0, 254 - (i * interval));
    } else {
      leds[i] = CRGB(0, 0, 0);
    }
  }
  LEDS.show();
}

void setLevel(int level){
  if(level < 0) {
    mLevel = 0;
  } else if (level > LED_COUNT) {
    mLevel = LED_COUNT;
  } else {
    mLevel = level;
  }
}

void updateActualLevel(){
  mActualLevel = (mActualLevel + mLevel) / 2.0;
}

void setup(){
  previousMillis = millis();
  mLevel = 60;

  LEDS.addLeds<WS2811, PIN_SIGNAL, GRB>(leds, LED_COUNT);
  LEDS.showColor(CRGB(0, 0, 0));
  LEDS.setBrightness(MAX_BRIGHTNESS);

  rainbow();
}

void loop(){
  while(Serial.available() > 0) {
    int level = charToInt(Serial.read());
    if(level < 0){
      continue;
    }
    setLevel(level);
  }

  currentMillis = millis();

  if(currentMillis - previousMillis > INTERVAL) {
    previousMillis += INTERVAL;
    updateActualLevel();
    rainbow();
  }
}


