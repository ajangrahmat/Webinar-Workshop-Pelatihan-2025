#include<WiFi.h>
#include<ArduinoOTA.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char* ssid = "Kelas Robot";
const char* pass = "kumaha aa we";
const char* OTAPassword = "admin1234";
WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "hello from ArduMeka!");
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi gak konek, mencoba menghubungkan ulang..");
      // Tidak perlu memanggil WiFi.reconnect() karena WiFi.setAutoReconnect(true) sudah diatur
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi Udah Konek!");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      // Mulai OTA dan WebServer hanya setelah mendapatkan IP
      if (!ArduinoOTA.isRunning()) { // Pastikan tidak dimulai berkali-kali
        ArduinoOTA.begin();
      }
      if (!MDNS.isRunning()) { // Pastikan tidak dimulai berkali-kali
        if (MDNS.begin("esp32")) {
          Serial.println("MDNS responder started");
        } else {
          Serial.println("Error starting MDNS responder!");
        }
      }
      if (!server.hasClient()) { // Pastikan tidak dimulai berkali-kali
         server.begin();
         Serial.println("Web server started");
      }
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(WiFiEvent);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, pass);

  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");

  ArduinoOTA.setHostname("ARDUMEKA_ESP32");
  ArduinoOTA.setPassword(OTAPassword);

  //EVENT OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Mulai update...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("Berhasil Update!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Sedang update: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]:", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Passwordnya salah");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Gagal memulai OTA");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Gagal Konek ke OTA");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Gagal Menerima");
    } else if (error == OTA_END_ERROR) {
      Serial.println("Gagal Selesai");
    }
  });

  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  // Karena kita menunggu koneksi di setup, kita bisa memulai OTA dan server di sini
  // Ini memastikan mereka dimulai hanya sekali dan setelah mendapatkan IP
  ArduinoOTA.begin();
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  } else {
    Serial.println("Error starting MDNS responder!");
  }
  server.begin();
  Serial.println("Web server started");

}

void loop() {
  // Hanya panggil handler saat WiFi terhubung
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    server.handleClient();
  }
  delay(2);
}
