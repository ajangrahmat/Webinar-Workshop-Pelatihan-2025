#include <WiFi.h>

const char* ssid = "Kelas Robot";
const char* pass = "kumaha aa we";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(500);
  }
  Serial.println("Connected");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting...");
      delay(500);
    }
  }

  Serial.println("Coding Utama...");
  delay(10000);
}
