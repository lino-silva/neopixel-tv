#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"

#define NUM_LEDS 54
#define DATA_PIN 4
#define USB_PIN 15

//STATES
#define INITIAL_STATE 0
#define UPDATE_STATE 1

//COMMANDS
uint8_t _State;

//NeoPixel
uint8_t R = 0;
uint8_t G = 1;
uint8_t B = 2;
uint8_t A = 3;

bool updateRGB = false;
uint8_t _StepCount = 1;

CRGB _leds[NUM_LEDS];
// Set initial color
uint8_t _rgbaCurrent[4] = {255,255,255,0};
// Initialize color variables
uint8_t _rgbaUpdated[4] = {255,255,255,255};

int DEBUG = 1;

void setup() {
  pinMode(USB_PIN, INPUT);
  Serial.begin(115200);

  //NeoPixel
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(_leds, NUM_LEDS);
  FastLED.setDither(0);

  _State = UPDATE_STATE;
  updateRGB = true;
}

void loop() {
  int val = digitalRead(USB_PIN);   // read the input pin

  if (_rgbaCurrent[A] != val) {
    _State = UPDATE_STATE;
    updateRGB = true;
  }

  if (val == 0) {
    _rgbaUpdated[A] = 0;
  } else {
    _rgbaUpdated[A] = 255;
  }

  mainLoop();
}
void mainLoop(){
  if(_State==UPDATE_STATE){
    if(updateRGB){
        _rgbaCurrent[R] = calculateVal(_rgbaCurrent[R], _rgbaUpdated[R]);
        _rgbaCurrent[G] = calculateVal(_rgbaCurrent[G], _rgbaUpdated[G]);
        _rgbaCurrent[B] = calculateVal(_rgbaCurrent[B], _rgbaUpdated[B]);
        for(int j=0;j<NUM_LEDS;j++){
          _leds[j].r = _rgbaCurrent[R];
          _leds[j].g = _rgbaCurrent[G];
          _leds[j].b = _rgbaCurrent[B];
        }
    }

    _rgbaCurrent[A] = calculateVal(_rgbaCurrent[A], _rgbaUpdated[A]);
    FastLED.setBrightness(_rgbaCurrent[A]);
    FastLED.show();

    if(DEBUG){
      Serial.print(_rgbaCurrent[R]);
      Serial.print(",");
      Serial.print(_rgbaCurrent[G]);
      Serial.print(",");
      Serial.print(_rgbaCurrent[B]);
      Serial.print(",");
      Serial.println(_rgbaCurrent[A]);
    }

    if(_rgbaCurrent[R] != _rgbaUpdated[R] || _rgbaCurrent[G] != _rgbaUpdated[G] || _rgbaCurrent[B] != _rgbaUpdated[B]){
      updateRGB = true;
    }else{
      updateRGB = false;
      if(_rgbaCurrent[A] == _rgbaUpdated[A]){
        _State = INITIAL_STATE;
      }
    }
  }
}
//Serial
void receiveData(char *_serialBytes){
  Serial.println("Update Values");
  //Update values
  _rgbaUpdated[R] = _serialBytes[R];
  _rgbaUpdated[G] = _serialBytes[G];
  _rgbaUpdated[B] = _serialBytes[B];
  _rgbaUpdated[A] = percentToRawValue(_serialBytes[A]);

  calculateStep();
  updateRGB = true;

  if(DEBUG){
    Serial.println("New Values:");
    Serial.print(_rgbaUpdated[R]);
    Serial.print(",");
    Serial.print(_rgbaUpdated[G]);
    Serial.print(",");
    Serial.print(_rgbaUpdated[B]);
    Serial.print(",");
    Serial.println(_rgbaUpdated[A]);
    Serial.print("Step:");
    Serial.println(_StepCount);
  }

  _State = UPDATE_STATE;
}

/*
  NeoPixel
*/
void setBrightness(int brightness){
  FastLED.setBrightness(brightness);
  _rgbaCurrent[A] = _rgbaUpdated[A] = brightness;
  FastLED.show();
}
void setColor(CRGB color, uint8_t brightness) {
  for(int i=0;i<NUM_LEDS;i++){
    _leds[i].setRGB(color.r, color.g, color.b);
  }
  FastLED.setBrightness(brightness);
  _rgbaCurrent[R] = _rgbaUpdated[R] = color.r;
  _rgbaCurrent[G] = _rgbaUpdated[G] = color.g;
  _rgbaCurrent[B] = _rgbaUpdated[B] = color.b;
  _rgbaCurrent[A] = _rgbaUpdated[A] = brightness;
  FastLED.show();
}
int percentToRawValue(int percent){
  return 2.55*percent;
}
/*
  Test Helpers
*/
void blink(){
  setColor(CRGB(255,255,255),255);
  delay(100);
  setColor(CRGB(0,0,0),0);
}
/*
  CrossFade to Value RGBA Helpers
*/
int calculateVal(int val, int finalVal) {
  if(val!=finalVal){
    if(abs(val-finalVal) <= _StepCount-1){
      val = finalVal;
    }else{
      if (val<finalVal) {
          val += _StepCount;
      } else {
          val -= _StepCount;
      }
    }
    return val;
  }
}
int calculateStep(){
  uint8_t max = max(max(max(abs(_rgbaCurrent[R]-_rgbaUpdated[R]),abs(_rgbaCurrent[G]-_rgbaUpdated[G])),abs(_rgbaCurrent[B]-_rgbaUpdated[B])),abs(_rgbaCurrent[A]-_rgbaUpdated[A]));
  if(max>63){
    if(max>127){
      if(max>191){
        _StepCount = 5;
      }else{
        _StepCount = 3;
      }
    }else{
      _StepCount = 2;
    }
  } else {
    _StepCount = 1;
  }
}
