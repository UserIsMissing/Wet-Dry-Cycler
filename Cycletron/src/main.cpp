#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>


const char* ssid = "UCSC-Devices";
const char* password = "o9ANAjrZ9zkjYKy2yL";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int LED_PIN = 2; // Built-in LED
void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    Serial0.printf("Received data (length %d): ", len);
    for (size_t i = 0; i < len; i++) {
      Serial0.print((char)data[i]);
    }
    Serial0.println();

    StaticJsonDocument<100> doc;
    DeserializationError    error = deserializeJson(doc, data, len);

    if (error) {
      Serial0.print("deSerializeJson() failed: ");
      Serial0.println(error.c_str());
      return;
    }

    bool state = doc["state"];

    if (state) {
      digitalWrite(LED_PIN, HIGH);
      Serial0.println("LED turned ON");
    } else {
      digitalWrite(LED_PIN, LOW);
      Serial0.println("LED turned OFF");
    }
  }
}





void setup() {
  Serial0.begin(115200);
  delay(3000); // Let Serial0 USB settle


  Serial0.print("ESP32 MAC Address: ");
  Serial0.println(WiFi.macAddress());
  Serial0.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial0.print(".");
  }

  Serial0.println("");
  Serial0.println("WiFi connected!");
  Serial0.print("ESP32 IP Address: ");
  Serial0.println(WiFi.localIP());  
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  Serial0.println("WebSocket server started and ready!");
}

void loop(){
}