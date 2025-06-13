#include "AViShaOTA.h"

// Static instance pointer
AViShaOTA* AViShaOTA::instance = nullptr;

// Constructor
AViShaOTA::AViShaOTA(const String& hostname, int port) {
  this->hostname = hostname;
  this->serverPort = port;
  this->server = new WebServer(port);
  this->otaPassword = ""; // Default password
  this->mdnsEnabled = true;
  this->serialDebug = true;

  // Initialize callbacks to nullptr
  this->onStartCallback = nullptr;
  this->onEndCallback = nullptr;
  this->onProgressCallback = nullptr;
  this->onErrorCallback = nullptr;
  this->onWiFiConnectedCallback = nullptr;
  this->onWiFiDisconnectedCallback = nullptr;

  // Set static instance
  instance = this;
}

// Destructor
AViShaOTA::~AViShaOTA() {
  if (server) {
    delete server;
  }
}

// Configuration methods
void AViShaOTA::setOTAPassword(const String& password) {
  this->otaPassword = password;
}

void AViShaOTA::setHostname(const String& name) {
  this->hostname = name;
}

void AViShaOTA::enableMDNS(bool enable) {
  this->mdnsEnabled = enable;
}

void AViShaOTA::enableSerialDebug(bool enable) {
  this->serialDebug = enable;
}

// Callback setters
void AViShaOTA::onStart(void (*callback)()) {
  this->onStartCallback = callback;
}

void AViShaOTA::onEnd(void (*callback)()) {
  this->onEndCallback = callback;
}

void AViShaOTA::onProgress(void (*callback)(unsigned int progress, unsigned int total)) {
  this->onProgressCallback = callback;
}

void AViShaOTA::onError(void (*callback)(ota_error_t error)) {
  this->onErrorCallback = callback;
}

void AViShaOTA::onWiFiConnected(void (*callback)()) {
  this->onWiFiConnectedCallback = callback;
}

void AViShaOTA::onWiFiDisconnected(void (*callback)()) {
  this->onWiFiDisconnectedCallback = callback;
}

// Main begin method
bool AViShaOTA::begin(const char* ssid, const char* password) {
  if (serialDebug) {
    Serial.println("Starting AViShaOTA...");
  }

  // Setup WiFi
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(wifiEventHandler);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);

  if (serialDebug) {
    Serial.print("Connecting to WiFi");
  }

  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    if (serialDebug) {
      Serial.print(".");
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (serialDebug) {
      Serial.println("\nFailed to connect to WiFi!");
    }
    return false;
  }

  if (serialDebug) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Setup OTA and Web Server
  setupArduinoOTA();
  setupWebServer();

  // Start MDNS if enabled
  if (mdnsEnabled) {
    if (MDNS.begin(hostname.c_str())) {
      if (serialDebug) {
        Serial.println("MDNS responder started");
      }
    } else {
      if (serialDebug) {
        Serial.println("Error starting MDNS responder!");
      }
    }
  }

  // Start services
  ArduinoOTA.begin();
  server->begin();

  if (serialDebug) {
    Serial.println("AViShaOTA started successfully!");
    Serial.print("Upload URL: ");
    Serial.println(getUploadURL());
  }

  return true;
}

// Handle method - call this in loop()
void AViShaOTA::handle() {
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    server->handleClient();
  }
}

// Setup ArduinoOTA
void AViShaOTA::setupArduinoOTA() {
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(otaPassword.c_str());

  ArduinoOTA.onStart([this]() {
    if (serialDebug) {
      Serial.println("OTA Update started...");
    }
    if (onStartCallback) {
      onStartCallback();
    }
  });

  ArduinoOTA.onEnd([this]() {
    if (serialDebug) {
      Serial.println("\nOTA Update completed!");
    }
    if (onEndCallback) {
      onEndCallback();
    }
  });

  ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
    if (serialDebug) {
      Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
    }
    if (onProgressCallback) {
      onProgressCallback(progress, total);
    }
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    if (serialDebug) {
      Serial.printf("OTA Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    }
    if (onErrorCallback) {
      onErrorCallback(error);
    }
  });
}

// Setup Web Server
void AViShaOTA::setupWebServer() {
  server->on("/", HTTP_GET, [this]() {
    handleRoot();
  });

  server->on("/update", HTTP_POST, [this]() {
    server->send(200, "text/plain", "");
  }, [this]() {
    handleUpdate();
  });
}

// Handle root request
void AViShaOTA::handleRoot() {
  server->send(200, "text/html", getUploadHTML());
}

// Handle update request
void AViShaOTA::handleUpdate() {
  HTTPUpload& upload = server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (serialDebug) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      if (serialDebug) {
        Update.printError(Serial);
      }
      server->send(500, "text/plain", "Update failed to start");
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      if (serialDebug) {
        Update.printError(Serial);
      }
      server->send(500, "text/plain", "Update write failed");
      return;
    }

    if (serialDebug) {
      Serial.printf("Progress: %d%%\r", (Update.progress() * 100) / Update.size());
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      if (serialDebug) {
        Serial.printf("\nUpdate Success: %u bytes\n", upload.totalSize);
      }
      server->send(200, "text/plain", "Update successful! ESP32 will restart...");
      delay(1000);
      ESP.restart();
    } else {
      if (serialDebug) {
        Update.printError(Serial);
      }
      server->send(500, "text/plain", "Update failed to complete");
    }
  }
}

// Static WiFi event handler
void AViShaOTA::wifiEventHandler(WiFiEvent_t event) {
  if (instance) {
    instance->handleWiFiEvent(event);
  }
}

// WiFi event handler
void AViShaOTA::handleWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      if (serialDebug) {
        Serial.println("WiFi disconnected, attempting to reconnect...");
      }
      if (onWiFiDisconnectedCallback) {
        onWiFiDisconnectedCallback();
      }
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      if (serialDebug) {
        Serial.println("WiFi connected!");
      }
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      if (serialDebug) {
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Upload URL: ");
        Serial.println(getUploadURL());
      }
      if (onWiFiConnectedCallback) {
        onWiFiConnectedCallback();
      }
      break;
    default:
      break;
  }
}

// Utility methods
String AViShaOTA::getLocalIP() {
  return WiFi.localIP().toString();
}

String AViShaOTA::getUploadURL() {
  return "http://" + getLocalIP() + "/";
}

bool AViShaOTA::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void AViShaOTA::restart() {
  ESP.restart();
}

// Get HTML template
const char* AViShaOTA::getUploadHTML() {
  static const char* uploadHTML = R"(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>AViSha OTA Update</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Oxygen,
        Ubuntu, Cantarell, "Open Sans", "Helvetica Neue", sans-serif;
      margin: 0;
      padding: 0;
      background: #f2f2f7;
    }

    .container {
      max-width: 400px;
      margin: 80px auto;
      background: #fff;
      border-radius: 20px;
      box-shadow: 0 8px 20px rgba(0, 0, 0, 0.08);
      padding: 30px;
      text-align: center;
    }

    h1 {
      font-size: 24px;
      margin-bottom: 10px;
      color: #111;
    }

    p {
      color: #555;
      font-size: 14px;
      margin-bottom: 20px;
    }

    .upload-area {
      border: 2px dashed #d1d1d6;
      border-radius: 12px;
      padding: 30px 10px;
      background-color: #fafafa;
      transition: background 0.3s ease;
    }

    .upload-area:hover {
      background: #f0f0f5;
    }

    input[type="file"] {
      margin-top: 15px;
      font-size: 14px;
      cursor: pointer;
    }

    button {
      margin-top: 20px;
      background-color: #007aff;
      color: white;
      border: none;
      padding: 12px 24px;
      font-size: 16px;
      border-radius: 12px;
      cursor: pointer;
      transition: background 0.3s ease;
    }

    button:hover {
      background-color: #005ed9;
    }

    .progress {
      width: 100%;
      background-color: #e5e5ea;
      border-radius: 12px;
      margin-top: 25px;
      height: 12px;
      overflow: hidden;
      display: none;
    }

    .progress-bar {
      height: 100%;
      width: 0%;
      background-color: #34c759;
      transition: width 0.3s ease;
    }

    #status {
      margin-top: 25px;
      font-size: 14px;
      color: #333;
    }

    .status-success {
      color: #28a745;
    }

    .status-error {
      color: #ff3b30;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 OTA Update</h1>
    <p>Unggah file .bin untuk memperbarui firmware perangkat Anda</p>

    <div class="upload-area">
      <form id="uploadForm" enctype="multipart/form-data">
        <input type="password" name="password" id="passwordInput" placeholder="Masukkan Password" required />
        <br />
        <input type="file" name="update" id="fileInput" accept=".bin" required />
        <br />
        <button type="submit">Upload dan Perbarui</button>
      </form>
    </div>

    <div class="progress" id="progressContainer">
      <div class="progress-bar" id="progressBar"></div>
    </div>

    <div id="status"></div>
  </div>

  <script>
    const uploadForm = document.getElementById("uploadForm");
    const fileInput = document.getElementById("fileInput");
    const progressContainer = document.getElementById("progressContainer");
    const progressBar = document.getElementById("progressBar");
    const status = document.getElementById("status");

    uploadForm.addEventListener("submit", function (e) {
      e.preventDefault();

      const file = fileInput.files[0];

      if (!file || !file.name.endsWith(".bin")) {
        alert("Silakan pilih file .bin yang valid.");
        return;
      }

      const formData = new FormData();
      formData.append("update", file);

      const xhr = new XMLHttpRequest();
      progressContainer.style.display = "block";

      xhr.upload.addEventListener("progress", function (e) {
        if (e.lengthComputable) {
          const percent = (e.loaded / e.total) * 100;
          progressBar.style.width = percent + "%";
        }
      });

      xhr.addEventListener("load", function () {
        progressContainer.style.display = "none";
        if (xhr.status === 200) {
          status.innerHTML = `<p class="status-success">Pembaruan berhasil. ESP32 akan memulai ulang.</p>`;
          setTimeout(() => location.reload(), 3000);
        } else {
          status.innerHTML = `<p class="status-error">Pembaruan gagal: ${xhr.responseText}</p>`;
        }
      });

      xhr.addEventListener("error", function () {
        progressContainer.style.display = "none";
        status.innerHTML = `<p class="status-error">Terjadi kesalahan saat mengunggah file.</p>`;
      });

      xhr.open("POST", "/update");
      xhr.send(formData);
    });
  </script>
</body>
</html>

)";
    return uploadHTML;
}
