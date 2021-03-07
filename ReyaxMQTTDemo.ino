/*
  Code by: 老明
  Youtube Channel: https://www.youtube.com/channel/UCVoGtuR1U5B-KK1kgMIoJWA
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 更換這裏
const char* ssid = "********";
const char* password = "********";
const char* mqttServer = "iot.reyax.com";
const int mqttPort = 1883;
const char* mqttUser = "********";
const char* mqttPassword = "********";
const char* mqttDeviceId = "********";
const char* mqttTopicPublish = "api/request";
const char* mqttTopicSubscribe = "api/command/********/#";
// 到這裏

WiFiClient espClient;
PubSubClient client(espClient);
DynamicJsonDocument doc(1024);
JsonObject jsonObject;

#define LED_PIN 32
int ledState = 0;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setBufferSize(1024);
  client.setKeepAlive(60);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setup_wifi() {
  delay(10);
  // 開始連接Wifi
  Serial.println();
  Serial.print("連接 ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi 已連接");
  Serial.println("IP 地址: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // 解讀json
  deserializeJson(doc, payload);
  jsonObject = doc.as<JsonObject>();

  if (jsonObject["action"].as<String>() == "command/insert") {
    String commandStr = jsonObject["command"]["command"].as<String>();
    if (commandStr == "switchLight") {
      switchLed(doc["command"]["result"]["status"].as<String>());
    }
  }
}

void reconnect() {
  // Loop到連接爲止
  while (!client.connected()) {
    Serial.print("嘗試連接MQTT...");
    // 創建新clientId
    String clientId = String(mqttUser) + "_" + String(random(0xffff), HEX);
    // 嘗試連接
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("已連接");
      // 發佈notification
      sendStatus();

      // 訂閲command
      client.subscribe(mqttTopicSubscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" 5秒後再嘗試連接");
      delay(5000);
    }
  }
}

void switchLed(String status) {
  if (status == "on") {
    ledState = 1;
  } else {
    ledState = 0;
  }
  digitalWrite(LED_PIN, ledState);
  sendStatus();
}

void sendStatus() {
  // 準備reyax需要的payload格式
  String payload = String("") + "{\"action\":\"notification/insert\",\"deviceId\":\"" + mqttDeviceId + "\",\"notification\":{\"notification\":\"ledStatus\",\"parameters\":{\"status\":" + (ledState ? "on" : "off") + "}}}";
  // 發佈notification
  client.publish(mqttTopicPublish, payload.c_str());
}
