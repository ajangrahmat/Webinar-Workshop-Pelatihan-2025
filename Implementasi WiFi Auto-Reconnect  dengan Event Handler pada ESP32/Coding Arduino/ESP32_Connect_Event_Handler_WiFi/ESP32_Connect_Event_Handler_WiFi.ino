#include <WiFi.h>

const char* ssid = "Kelas Robot";
const char* pass = "kumaha aa we";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, pass);
}

void loop() {
  Serial.println("Coding Utama...");
  delay(10000);
}

void WiFiEvent(WiFiEvent_t event) {

  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi gak konek, mencoba menghubungkan ulang..");
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi Udah Konek!");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println(WiFi.localIP());
      break;
    default:
      break;
  }

}
