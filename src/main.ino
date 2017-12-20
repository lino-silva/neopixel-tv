#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "config.h"

#include <PubSubClient.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

inline int max(int a, int b) { return ((a)>(b)?(a):(b)); }
inline int min(int a, int b) { return ((a)<(b)?(a):(b)); }

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer webServer(80);

bool OTAUpdate = false;
long OTATimeout = 60*1000;
long OTAend=0;

int lastMsg;
CHSV lastPublishedColor;

//COMMANDS
uint8_t _colorNeedsUpdating;
uint8_t tv_on = 0;

uint8_t _StepCount = 1;

CRGB _leds[NUM_LEDS];
static CHSV _hsvStartup = {255, 0, 255};
static CHSV _hsvOff = {0, 0, 0};
// Set initial color
CHSV _hsvCurrent = _hsvOff;
// Initialize color variables
CHSV _hsvUpdated = _hsvOff;

void setup() {
  pinMode(USB_PIN, INPUT);
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  // WIFI and OTA
  setup_wifi();
  webServer.on("/update", handleRoot);
  webServer.begin();

  #ifdef DEBUG
  Serial.println("HTTP server started");
  #endif

  bindOTAEvents();

  // MQTT
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqtt_callback);

  //NeoPixel
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(_leds, NUM_LEDS);
  FastLED.setDither(0);

  setNewColor(_hsvStartup);
}

void loop() {
  if (OTAUpdate && millis() - OTAend > 0) {
    ArduinoOTA.handle();
  } else {
    OTAUpdate=false;
    webServer.handleClient();

    if (!client.connected()) {
      #ifdef DEBUG
      Serial.println("Not connected");
      #endif
      reconnect();
    }

    client.loop();

    // read the input pin
    uint8_t is_tv_on = digitalRead(USB_PIN);

    long now = millis();
    if (now - lastMsg > MQTT_MESSAGE_INTERVAL) {
      lastMsg = now;

      if (lastPublishedColor == _hsvCurrent) {
        lastPublishedColor = _hsvCurrent;

        bool isPowerOn = _hsvCurrent.h != 0 && _hsvCurrent.s != 0 && _hsvCurrent.v != 0;
        const char *powerOn = isPowerOn?"true":"false";
        char currentHue[4];
        char currentSat[4];
        char currentVal[4];
        sprintf(currentHue, "%03i", _hsvCurrent.h * 360 / 255);
        sprintf(currentSat, "%03i", _hsvCurrent.s * 100 / 255);
        sprintf(currentVal, "%03i", _hsvCurrent.v * 100 / 255);

        // client.publish(MQTT_TOPIC_POWER, powerOn);
        // client.publish(MQTT_TOPIC_BRIGHTNESS, currentVal);
        // client.publish(MQTT_TOPIC_HUE, currentHue);
        // client.publish(MQTT_TOPIC_SATURATION, currentSat);
      }
    }

    // if (is_tv_on != tv_on) {
    //   _colorNeedsUpdating = true;
    // }
    //
    // if (tv_on == 0) {
    //   _hsvUpdated.v = 0;
    // } else {
    //   _hsvUpdated.v = 100;
    // }

    mainLoop();
  }
}

void mainLoop() {
  if (_colorNeedsUpdating) {
    _hsvCurrent.h = calculateVal(_hsvCurrent.h, _hsvUpdated.h);
    _hsvCurrent.s = calculateVal(_hsvCurrent.s, _hsvUpdated.s);
    _hsvCurrent.v = calculateVal(_hsvCurrent.v, _hsvUpdated.v);
    FastLED.show();
    // fill_solid_skip_first(_leds, NUM_LEDS, _hsvCurrent);
    fill_solid(_leds, NUM_LEDS, _hsvCurrent);

    #ifdef DEBUG
    Serial.print(_hsvCurrent.h);
    Serial.print(",");
    Serial.print(_hsvCurrent.s);
    Serial.print(",");
    Serial.println(_hsvCurrent.v);
    #endif

    if (_hsvCurrent.h == _hsvUpdated.h &&
        _hsvCurrent.s == _hsvUpdated.s &&
        _hsvCurrent.v == _hsvUpdated.v) {
      _colorNeedsUpdating = false;
    }
  }
}

void setNewColor(CHSV hsv) {
  #ifdef DEBUG
  Serial.println("Update Values");
  #endif

  //Update values
  _hsvUpdated.h = hsv.h;
  _hsvUpdated.s = hsv.s;
  _hsvUpdated.v = hsv.v;

  calculateStep();

  #ifdef DEBUG
  Serial.println("New Values:");
  Serial.print(_hsvUpdated.h);
  Serial.print(",");
  Serial.print(_hsvUpdated.s);
  Serial.print(",");
  Serial.println(_hsvUpdated.v);
  Serial.print("Step:");
  Serial.println(_StepCount);
  #endif

  _colorNeedsUpdating = true;
}

/*
  CrossFade to Value RGBA Helpers
*/
int calculateVal(int val, int finalVal) {
  if (val != finalVal) {
    if (abs(val - finalVal) <= _StepCount - 1){
      val = finalVal;
    }else{
      if (val < finalVal) {
          val += _StepCount;
      } else {
          val -= _StepCount;
      }
    }
    return val;
  }
}

int calculateStep () {
  // uint8_t hdiff = abs(_hsvCurrent.h-_hsvUpdated.h);
  // uint8_t sdiff = abs(_hsvCurrent.s-_hsvUpdated.s);
  // uint8_t vdiff = abs(_hsvCurrent.v-_hsvUpdated.v);
  // int max_val = max(max(hdiff, sdiff), vdiff);
  // if (max_val > 63){
  //   if (max_val > 127){
  //     if (max_val > 191){
  //       _StepCount = 5;
  //     } else {
  //       _StepCount = 3;
  //     }
  //   } else {
  //     _StepCount = 2;
  //   }
  // } else {
  //   _StepCount = 1;
  // }
  _StepCount = 1;
}

void mqtt_callback (const char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  char* msg = (char*) payload;

  #ifdef DEBUG
  Serial.print("Got message from ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(msg);
  Serial.println(atoi(msg));
  #endif

  CHSV newColor;
  bool colorChanged = false;

  if (strcmp(topic, MQTT_TOPIC_SETPOWER) == 0) {
    bool setPowerOn = strcmp(msg, "true") == 0;
    if (setPowerOn && _hsvCurrent.v == 0) {
      newColor = _hsvStartup;
      colorChanged = true;
    } else if (!setPowerOn && _hsvCurrent.v != 0) {
      newColor = _hsvOff;
      colorChanged = true;
    }
  } else if (strcmp(topic, MQTT_TOPIC_SETBRIGHTNESS) == 0) {
    newColor = CHSV(_hsvCurrent.h, _hsvCurrent.s, atoi(msg) * 255 / 100);
    colorChanged = true;
  } else if (strcmp(topic, MQTT_TOPIC_SETHUE) == 0) {
    newColor = CHSV(atoi(msg) * 255 / 360, _hsvCurrent.s, _hsvCurrent.v);
    colorChanged = true;
  } else if (strcmp(topic, MQTT_TOPIC_SETSATURATION) == 0) {
    newColor = CHSV(_hsvCurrent.h, atoi(msg) * 255 / 100, _hsvCurrent.v);
    colorChanged = true;
  }

  if (colorChanged) {
    #ifdef DEBUG
    Serial.println("Color changed, updating...");
    #endif
    setNewColor(newColor);
  }
}

void bindOTAEvents() {
  #ifdef DEBUG
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating ");
  });
  #endif

  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nEnd");
    #endif

    OTAUpdate =false;
    delay(100);
    ESP.restart();
  });

  #ifdef DEBUG
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  #endif

  ArduinoOTA.onError([](ota_error_t error) {

    #ifdef DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif

    delay(100);
    ESP.restart();
  });

  ArduinoOTA.setPort(9069);
  ArduinoOTA.setPassword("M4yTh3F0rc3B3W1thY0u");
  ArduinoOTA.begin();
}

void setup_wifi() {
  int attempts = 0;
  delay(10);

  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  #endif

  WiFi.begin(WLAN_SSID, WLAN_PWD);

  while (WiFi.status() != WL_CONNECTED && ++attempts < MAX_WIFI_CONNECT_TRY) {
    delay(1000);

    #ifdef DEBUG
    Serial.print(".");
    #endif
  }

  if (WiFi.status() != WL_CONNECTED) {

    #ifdef DEBUG
    Serial.println("Wifi Failed.. Restarting");
    #endif

    delay(100);
    ESP.restart();
  }

  #ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  #endif
}

void reconnect() {
  int attempts = 0;

  while (!client.connected() && ++attempts < MAX_MQTT_CONNECT_TRY) {
    #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
    Serial.print(attempts);
    #endif

    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      #ifdef DEBUG
      Serial.println("connected");
      #endif

      client.subscribe(MQTT_TOPIC_SUB);

      #ifdef DEBUG
      Serial.println("Subscribed to MQTT topics");
      #endif
    } else {
      #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      #endif

      delay(5000);
    }
  }

  if (!client.connected()) {
    #ifdef DEBUG
    Serial.println("Mqtt Failed.. Restarting");
    #endif

    delay(100);
    ESP.restart();
  }
}

void handleRoot() {
  #ifdef DEBUG
  Serial.println("GOT OTA cmd");
  #endif

  OTAUpdate = true;
  OTAend = millis() + OTATimeout;
  webServer.send(200, "text/plain", "GO");

  #ifdef DEBUG
  Serial.println("Start OTA");
  #endif
}
