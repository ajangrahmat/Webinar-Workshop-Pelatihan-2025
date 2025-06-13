#include "AViShaOTA.h"

AViShaOTA ota("ardumeka");

void setup() {
    Serial.begin(115200);
    ota.setOTAPassword("admin123");
    ota.begin("Kelas Robot", "kumaha aa we");
}

void loop() {
    ota.handle();
}
