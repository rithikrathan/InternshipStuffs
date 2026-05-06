#include <ArduinoJson.h> // https://arduinojson.org/
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <Preferences.h>
#include <SocketIOclient.h>
#include <WebSocketsClient.h> // download and install from https://github.com/Links2004/arduinoWebSockets
#include <WiFiManager.h>

#define DATA_PIN 1
#define NUM_LEDS 8
#define BRIGHTNESS 50
#define SERVER                                                                 \
  "homecontrol.shinecrafttechnologies.com" // Server URL (without https://www)
#define PORT 80

const long interval = 2000; // 2 seconds

CRGB leds[NUM_LEDS];
unsigned long previousMillis = 0, currentMillis;
SocketIOclient socketIO;
char code[4];
DHT ins[2];
char roomId[24];
char homeId[24];

String BOARDID = "ETSS401006";
String newHostname = "IntelliGB_ControlNode";
String userId = "", boardCreatedId = "";

int inps = 0;

WiFiManager wifiManager;
WiFiManagerParameter codeParamater("linkCode", "Link Code", code, 4);
WiFiManagerParameter roomIdParameter("roomId", "Room Id", roomId, 24);
WiFiManagerParameter homeIdParamater("homeId", "Home Id", homeId, 24);

Preferences preferences;
// int data = 1, times = 100, MIN = 1, INTERVAL = 60;

DynamicJsonDocument doc(1024);
String output;
JsonArray array;
JsonObject params;
int values = 2;

void messageHandler(uint8_t *payload) {
  StaticJsonDocument<64> doc;
  Serial.println((char *)payload);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println(error.f_str());
    return;
  }
  String eventId = doc[0];
  if (eventId == "SYNC") {
    StaticJsonDocument<256> devicesStates;
    deserializeJson(devicesStates, doc[1]);
    JsonArray devices = devicesStates.as<JsonArray>();
    for (JsonVariant device : devices) {
      bool state = device["state"];
      uint8_t pin = device["pin"];
      digitalWrite(pin, state);
      delay(100);
    }
  } else if (eventId == "LINKED:" + BOARDID) {
    StaticJsonDocument<1024> linkedConfig;
    deserializeJson(linkedConfig, doc[1]);
    preferences.putString("access_token",
                          linkedConfig["access_token"].as<String>());
    preferences.putString("refresh_token",
                          linkedConfig["refresh_token"].as<String>());
    preferences.putString("userId", linkedConfig["userId"].as<String>());
    preferences.putString("boardCreatedId",
                          linkedConfig["boardCreatedId"].as<String>());
    preferences.end();
    delay(1000);
    ESP.reset();
  }

  else if (boardCreatedId != "" && eventId == boardCreatedId) {
    StaticJsonDocument<512> devicesStates;
    deserializeJson(devicesStates, doc[1]);
    JsonArray devices = devicesStates.as<JsonArray>();
    for (JsonVariant device : devices) {
      bool state = device["state"];
      uint8_t pin = device["pin"];
      digitalWrite(pin, state);
      delay(100);
    }
  } else if (eventId == "PINSYNC") {
    StaticJsonDocument<256> pins;
    deserializeJson(pins, doc[1]);
    JsonArray outPins = pins["outs"].as<JsonArray>();
    JsonArray insPins = pins["ins"].as<JsonArray>();

    for (uint8_t pin : outPins) {
      pinMode(pin, OUTPUT);
    }
    for (uint8_t pin : insPins) {
      ins[inps++] = DHT(pin, DHT11);
      ins[inps - 1].begin();
      // pinMode(pin, INPUT);
    }
  }
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload,
                   size_t length) {
  switch (type) {
  case sIOtype_DISCONNECT:
    Serial.println("Disconnected!");
    break;

  case sIOtype_CONNECT:
    Serial.printf("Connected to url: %s%s\n", SERVER, payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    break;

  case sIOtype_EVENT:
    messageHandler(payload);
    break;
  default:
    // Serial.println(type);
    break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Begining.");

  pinMode(A0, INPUT_PULLUP);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  preferences.begin("IntelliGB");
  if (analogRead(A0) >= 700) {
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Black;
    FastLED.show();

    wifiManager.resetSettings();
    delay(2000);
    preferences.clear();
    ESP.eraseConfig();
  }

  wifiManager.addParameter(&codeParamater);
  wifiManager.addParameter(&homeIdParamater);
  wifiManager.addParameter(&roomIdParameter);
  wifiManager.autoConnect("IntelliGB", "11223344556677889900");
  boolean result = wifiManager.autoConnect();

  if (result == true) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::White;
      FastLED.show();
      delay(100);
      leds[i] = CRGB::Black;
    }

    delay(500);
  } else {
    leds[1] = CRGB::White;
    FastLED.show();
    delay(100);
    leds[1] = CRGB::Black;
    FastLED.show();
  }

  WiFi.hostname(newHostname.c_str());

  userId = preferences.getString("userId", "");
  String refresh_token = preferences.getString("refresh_token", "");
  String access_token = preferences.getString("access_token", "");
  boardCreatedId = preferences.getString("boardCreatedId", "");
  DynamicJsonDocument doc(1024);
  String postData;
  String dd = "client: ";

  if (access_token == "" || refresh_token == "" || userId == "" ||
      boardCreatedId == "" || access_token == "null" ||
      refresh_token == "null" || userId == "null" || boardCreatedId == "null") {
    const char *linkCode = codeParamater.getValue();
    const char *roomId = roomIdParameter.getValue();
    const char *homeId = homeIdParamater.getValue();
    Serial.println(linkCode);
    Serial.println(roomId);
    Serial.println(homeId);
    doc["linkCode"] = linkCode;
    doc["homeId"] = homeId;
    doc["roomId"] = roomId;
    doc["boardId"] = BOARDID;
    doc["isRegister"] = true;
  } else {
    doc["access_token"] = access_token;
    doc["refresh_token"] = refresh_token;
    doc["boardId"] = BOARDID;
    doc["boardCreatedId"] = boardCreatedId;
  }

  serializeJson(doc, postData);
  dd += postData;
  socketIO.setExtraHeaders(dd.c_str());
  socketIO.begin(SERVER, PORT, "/socket.io/?EIO=4");
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;

    for (int i = 0; i < inps; i++) {
      float data = ins[i].readTemperature();
      array = doc.to<JsonArray>();
      array.add(BOARDID + ":UPLOAD");
      params = array.createNestedObject();

      if (isnan(data)) {
        values--;
      } else {
        params["temp"] = data;
      }

      data = ins[i].readHumidity();
      if (isnan(data)) {
        values--;
      } else {
        params["humidity"] = data;
      }

      if (values != 0) {
        params["pin"] = ins[i].getPin();
        serializeJson(doc, output);
        socketIO.sendEVENT(output);
        doc.clear();
        output.clear();
        params.clear();
        array.clear();
        values = 2;
      }
    }
  }

  leds[5] = CRGB::White;
  FastLED.show();
  delay(100);
  leds[5] = CRGB::Red;
  FastLED.show();

  socketIO.loop();
}
