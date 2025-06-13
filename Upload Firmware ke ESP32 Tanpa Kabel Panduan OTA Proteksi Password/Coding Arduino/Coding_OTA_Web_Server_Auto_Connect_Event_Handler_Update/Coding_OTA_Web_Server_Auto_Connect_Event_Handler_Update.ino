#include<WiFi.h>
#include<ArduinoOTA.h>
#include<WebServer.h>
#include<ESPmDNS.h>
#include<Update.h>

const char* ssid = "Kelas Robot";
const char* pass = "kumaha aa we";
const char* OTAPassword = "admin1234";

WebServer server(80);

// HTML untuk halaman upload
const char* uploadHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Update</title>
    <style>
        body { font-family: Arial; margin: 50px; }
        .container { max-width: 600px; margin: 0 auto; }
        .upload-area { 
            border: 2px dashed #ccc; 
            padding: 20px; 
            text-align: center; 
            margin: 20px 0;
        }
        input[type="file"] { margin: 10px 0; }
        button { 
            background: #4CAF50; 
            color: white; 
            padding: 10px 20px; 
            border: none; 
            cursor: pointer;
            font-size: 16px;
        }
        button:hover { background: #45a049; }
        .progress { 
            width: 100%; 
            background-color: #f0f0f0; 
            margin: 10px 0;
            display: none;
        }
        .progress-bar { 
            width: 0%; 
            height: 30px; 
            background-color: #4CAF50; 
            text-align: center; 
            line-height: 30px; 
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>AViShaOTA Update</h1>
        <p>Upload file .bin untuk update firmware</p>
        
        <div class="upload-area">
            <form id="uploadForm" enctype="multipart/form-data">
                <input type="file" name="update" id="fileInput" accept=".bin" required>
                <br><br>
                <button type="submit">Upload & Update</button>
            </form>
        </div>
        
        <div class="progress" id="progressContainer">
            <div class="progress-bar" id="progressBar">0%</div>
        </div>
        
        <div id="status"></div>
    </div>

    <script>
        document.getElementById('uploadForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const fileInput = document.getElementById('fileInput');
            const file = fileInput.files[0];
            
            if (!file) {
                alert('Pilih file .bin terlebih dahulu!');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                alert('File harus berformat .bin!');
                return;
            }
            
            const formData = new FormData();
            formData.append('update', file);
            
            const xhr = new XMLHttpRequest();
            const progressContainer = document.getElementById('progressContainer');
            const progressBar = document.getElementById('progressBar');
            const status = document.getElementById('status');
            
            // Show progress bar
            progressContainer.style.display = 'block';
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = (e.loaded / e.total) * 100;
                    progressBar.style.width = percentComplete + '%';
                    progressBar.textContent = Math.round(percentComplete) + '%';
                }
            });
            
            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    status.innerHTML = '<p style="color: green;">Update berhasil! ESP32 akan restart...</p>';
                    setTimeout(() => {
                        location.reload();
                    }, 3000);
                } else {
                    status.innerHTML = '<p style="color: red;">Update gagal: ' + xhr.responseText + '</p>';
                }
                progressContainer.style.display = 'none';
            });
            
            xhr.addEventListener('error', function() {
                status.innerHTML = '<p style="color: red;">Error saat upload!</p>';
                progressContainer.style.display = 'none';
            });
            
            xhr.open('POST', '/update');
            xhr.send(formData);
        });
    </script>
</body>
</html>
)";

void handleRoot() {
  server.send(200, "text/html", uploadHTML);
}

void handleUpdate() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // Mulai update
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      server.send(500, "text/plain", "Update gagal dimulai");
      return;
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Tulis data ke flash
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      server.send(500, "text/plain", "Update gagal menulis");
      return;
    }
    
    // Tampilkan progress
    Serial.printf("Progress: %d%%\r", (Update.progress() * 100) / Update.size());
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("\nUpdate berhasil: %u bytes\n", upload.totalSize);
      server.send(200, "text/plain", "Update berhasil! ESP32 akan restart...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      server.send(500, "text/plain", "Update gagal diselesaikan");
    }
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi gak konek, mencoba menghubungkan ulang..");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("WiFi Udah Konek!");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Upload URL: http://");
      Serial.print(WiFi.localIP());
      Serial.println("/");
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
  
  // EVENT OTA
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
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleUpdate);
  
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  
  // Mulai layanan setelah WiFi terhubung
  ArduinoOTA.begin();
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  } else {
    Serial.println("Error starting MDNS responder!");
  }
  server.begin();
  Serial.println("Web server started");
  Serial.print("Upload URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  // Hanya panggil handler saat WiFi terhubung
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    server.handleClient();
  }
  delay(2);
}
