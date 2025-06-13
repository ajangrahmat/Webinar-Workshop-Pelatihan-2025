#include<WiFi.h>
#include<ArduinoOTA.h>


const char* ssid = "Kelas Robot";
const char* pass = "kumaha aa we";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  ArduinoOTA.setHostname("ARDUMEKA_ESP32");

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

  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
}
