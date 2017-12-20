#include "FastLED.h"

#define DEBUG 0

#define NUM_LEDS 54
#define DATA_PIN 4
#define USB_PIN 15

#define MAX_MQTT_CONNECT_TRY 30
#define MQTT_SERVER "[MQTT_SERVER]"
#define MQTT_PORT 1883
#define MQTT_MESSAGE_INTERVAL 5000

#define MAX_WIFI_CONNECT_TRY 1000
#define WLAN_SSID "[SSID]"
#define WLAN_PWD "[PASSWORD]"

#define MQTT_TOPIC_SUB "home/livingroom/tvbacklight/#"
#define MQTT_TOPIC_POWER "home/livingroom/tvbacklight/powerStatus"
#define MQTT_TOPIC_SETPOWER "home/livingroom/tvbacklight/setPowerStatus"
#define MQTT_TOPIC_BRIGHTNESS "home/livingroom/tvbacklight/brightness"
#define MQTT_TOPIC_SETBRIGHTNESS "home/livingroom/tvbacklight/setBrightness"
#define MQTT_TOPIC_HUE "home/livingroom/tvbacklight/hue"
#define MQTT_TOPIC_SETHUE "home/livingroom/tvbacklight/setHue"
#define MQTT_TOPIC_SATURATION "home/livingroom/tvbacklight/saturation"
#define MQTT_TOPIC_SETSATURATION "home/livingroom/tvbacklight/setSaturation"

void mqtt_callback(const char* topic, byte* payload, unsigned int length);
void bindOTAEvents();
void setup_wifi();
void reconnect();
void handleRoot();
int calculateStep();
int calculateVal(int val, int finalVal);
void blink();
void setColor(CHSV color);
void setBrightness(int brightness);
void setNewColor(CHSV hsv);
void mainLoop();
void loop();
void setup();
