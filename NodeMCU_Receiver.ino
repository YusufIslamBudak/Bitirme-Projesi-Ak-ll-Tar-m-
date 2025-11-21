#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>  // NTP zaman damgası için

// WiFi bilgilerinizi buraya girin
const char* ssid = "TurkTelekom_ZEHX3";        // WiFi aginizin adi
const char* password = "fE65e72Db177c";        // WiFi sifreniz

// SoftwareSerial pinleri (Arduino Mega ile CIFT YONLU haberlesme)
// D1 = GPIO5 (RX - Arduino TX2'den veri al)
// D2 = GPIO4 (TX - Arduino RX2'ye komut gonder)
const uint8_t SOFT_RX = D1;  // Arduino TX2'den buraya baglanir
const uint8_t SOFT_TX = D2;  // Arduino RX2'ye komut gonderir

// SD kart CS pini
const uint8_t SD_CS = D8;  // GPIO15

// USB Serial hizi (Serial Monitor icin)
const unsigned long SERIAL_BAUD = 115200;

// Arduino Mega ile SoftwareSerial - 9600 baud (CIFT YONLU)
const unsigned long ARDUINO_BAUD = 9600;

// NTP Sunucu Ayarları (Türkiye saati için)
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;  // GMT+3 (Türkiye)
const int daylightOffset_sec = 0;      // Yaz saati yok

// SoftwareSerial nesnesi
SoftwareSerial arduinoSerial(SOFT_RX, SOFT_TX);

// Web Server (Port 80)
ESP8266WebServer server(80);

// JSON buffer (Arduino'dan gelen sensor verileri)
String jsonBuffer = "";
bool jsonComplete = false;

// Son alinan sensor verileri (Web interface icin)
String lastSensorData = "Henuz veri yok...";

// Fonksiyon prototipleri (Forward declarations)
void sendCommandToArduino(String command);
void handleRoot();
void handleCommand();
void handleStatus();
void handleNotFound();
String getFormattedTime();

void setup() {
  // USB Serial Monitor baslat
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  // Arduino Mega ile SoftwareSerial baslat (9600 baud - KARARLI)
  arduinoSerial.begin(ARDUINO_BAUD);
  
  Serial.println("\n\n=== NodeMCU Data Logger ===");
  Serial.println("Arduino Mega'dan JSON verisi bekleniyor...");
  Serial.println("SoftwareSerial: D1(RX)=GPIO5, 9600 baud");
  
  Serial.println();
  Serial.println("WiFi'ye baglaniyor...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  
  // WiFi baglantisini baslat
  WiFi.begin(ssid, password);
  
  // Baglanti kurulana kadar bekle
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  // Baglanti durumunu kontrol et
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi'ye basariyla baglandi!");
    Serial.print("IP Adresi: ");
    Serial.println(WiFi.localIP());
    Serial.print("Sinyal Gucu (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("WiFi baglantisi basarisiz!");
    Serial.println("Lutfen SSID ve sifrenizi kontrol edin.");
  }

  Serial.println();
  Serial.print("SD kart baslatiliyor, CS=D8 (GPIO15)...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println(" HATA! SD baslatilamadi.");
  } else {
    Serial.println(" TAMAM!");
  }
  
  Serial.println("\nArduino Mega'dan JSON verisi bekleniyor...");
  Serial.println("-------------------------------------------");
  
  // NTP ile zaman senkronizasyonu
  Serial.println("\nNTP ile zaman senkronize ediliyor...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Zaman senkronize olana kadar bekle
  int ntpRetry = 0;
  while (time(nullptr) < 100000 && ntpRetry < 20) {
    delay(500);
    Serial.print(".");
    ntpRetry++;
  }
  
  if (time(nullptr) > 100000) {
    Serial.println("\nNTP zamani basariyla alindi!");
    Serial.print("Suan: ");
    Serial.println(getFormattedTime());
  } else {
    Serial.println("\nUYARI: NTP zamani alinamadi, sistem zamani kullanilacak");
  }
  
  // Web Server rotalarini tanimla
  server.on("/", handleRoot);
  server.on("/command", handleCommand);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  
  // Web Server'i baslat
  server.begin();
  Serial.println("\nWeb Server baslatildi!");
  Serial.print("Kontrol Paneli: http://");
  Serial.println(WiFi.localIP());
  Serial.println("\nKomutlar (Serial Monitor veya Web):");
  Serial.println("  havaac, havakapa, isikac, isikkapa, sulaac, sulakapa");
}

void loop() {
  // Web Server isteklerini isle
  server.handleClient();
  
  // WiFi baglantisi koptugunda yeniden baglan
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi baglantisi koptu, yeniden baglaniyor...");
    WiFi.reconnect();
    delay(5000);
  }

  // USB Serial Monitor'den komut oku (test icin)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd.length() > 0) {
      Serial.print("[NodeMCU] Komut gonderiliyor: ");
      Serial.println(cmd);
      sendCommandToArduino(cmd);
    }
  }

  // Arduino Mega'dan (SoftwareSerial) gelen JSON'u oku
  while (arduinoSerial.available()) {
    char c = (char)arduinoSerial.read();
    
    // JSON baslangici
    if (c == '{') {
      jsonBuffer = "{";
      jsonComplete = false;
    }
    // JSON bitisi
    else if (c == '}') {
      jsonBuffer += '}';
      jsonComplete = true;
    }
    // JSON icerigi
    else if (jsonBuffer.length() > 0) {
      jsonBuffer += c;
    }
  }

  // Tam JSON alindiysa isle
  if (jsonComplete) {
    Serial.println("\n>>> ARDUINO'DAN JSON ALINDI <<<");
    Serial.print("Boyut: ");
    Serial.print(jsonBuffer.length());
    Serial.println(" byte");
    Serial.println("JSON:");
    Serial.println(jsonBuffer);
    
    // Son veriyi sakla (Web interface icin)
    lastSensorData = jsonBuffer;

    // SD'ye kaydet (NTP zaman damgası ile)
    File log = SD.open("/sensor_log.txt", FILE_WRITE);
    if (log) {
      String timestamp = getFormattedTime();
      log.println(timestamp);  // İlk satır: zaman damgası
      log.println(jsonBuffer); // İkinci satır: JSON verisi
      log.println();           // Boş satır (ayırıcı)
      log.close();
      Serial.print("[SD] Veri kaydedildi: ");
      Serial.println(timestamp);
    } else {
      Serial.println("[SD] HATA: sensor_log.txt acilamadi!");
    }
    
    // Firebase'e gonder (TODO: Firebase implementasyonu)
    // sendToFirebase(jsonBuffer);
    
    Serial.println(">>> ISLEM TAMAMLANDI <<<\n");
    
    // Buffer temizle
    jsonBuffer = "";
    jsonComplete = false;
  }

  delay(10);
}

// ========================================
// ARDUINO'YA KOMUT GONDERME FONKSIYONU
// ========================================
void sendCommandToArduino(String command) {
  // Komutu Arduino'ya gonder (SoftwareSerial TX pin'i uzerinden)
  arduinoSerial.println(command);
  arduinoSerial.flush();  // Gonderimin tamamlanmasini bekle
  
  Serial.print("[TX Arduino] ");
  Serial.println(command);
}

// ========================================
// WEB SERVER FONKSIYONLARI
// ========================================

// Ana sayfa (Kontrol Paneli)
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Sera Kontrol Paneli</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0}";
  html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
  html += "h1{color:#333;text-align:center}";
  html += ".status{background:#e8f5e9;padding:15px;border-radius:5px;margin:20px 0}";
  html += ".controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:20px 0}";
  html += "button{padding:15px;font-size:16px;border:none;border-radius:5px;cursor:pointer;transition:0.3s}";
  html += ".btn-on{background:#4CAF50;color:white}.btn-on:hover{background:#45a049}";
  html += ".btn-off{background:#f44336;color:white}.btn-off:hover{background:#da190b}";
  html += ".btn-water{background:#2196F3;color:white}.btn-water:hover{background:#0b7dda}";
  html += ".info{color:#666;font-size:14px;margin-top:20px}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>🌱 Sera Kontrol Paneli</h1>";
  html += "<div class='status' id='sensorData'>";
  html += "<strong>Son Sensor Verisi:</strong><br>";
  html += "<small id='updateTime'>" + getFormattedTime() + "</small><br>";
  html += "<pre style='overflow-x:auto'>" + lastSensorData + "</pre>";
  html += "</div>";
  html += "<div class='controls'>";
  html += "<button class='btn-on' onclick=\"cmd('havaac')\">🌬️ Hava Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('havakapa')\">🔒 Hava Kapat</button>";
  html += "<button class='btn-on' onclick=\"cmd('isikac')\">💡 Işık Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('isikkapa')\">🌙 Işık Kapat</button>";
  html += "<button class='btn-water' onclick=\"cmd('sulaac')\">💧 Sulama Aç</button>";
  html += "<button class='btn-off' onclick=\"cmd('sulakapa')\">🛑 Sulama Kapat</button>";
  html += "</div>";
  html += "<div class='info'>";
  html += "<strong>IP Adresi:</strong> " + WiFi.localIP().toString() + "<br>";
  html += "<strong>RSSI:</strong> " + String(WiFi.RSSI()) + " dBm<br>";
  html += "<strong>Uptime:</strong> " + String(millis()/1000) + " saniye";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function cmd(c){fetch('/command?cmd='+c).then(r=>r.text()).then(d=>{alert(d);updateStatus()})}";
  html += "function updateStatus(){fetch('/status').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('sensorData').innerHTML='<strong>Son Sensor Verisi:</strong><br><small>'+new Date().toLocaleString('tr-TR')+'</small><br><pre style=\"overflow-x:auto\">'+d.lastData+'</pre>';";
  html += "})}";
  html += "setInterval(updateStatus,3000);";
  html += "updateStatus();";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Komut isleme endpoint
void handleCommand() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    cmd.toLowerCase();
    
    // Komut listesi kontrolu
    if (cmd == "havaac" || cmd == "havakapa" || cmd == "isikac" || 
        cmd == "isikkapa" || cmd == "sulaac" || cmd == "sulakapa") {
      
      sendCommandToArduino(cmd);
      server.send(200, "text/plain", "Komut gonderildi: " + cmd);
    } else {
      server.send(400, "text/plain", "Gecersiz komut: " + cmd);
    }
  } else {
    server.send(400, "text/plain", "Komut parametresi eksik!");
  }
}

// Durum sorgulama endpoint
void handleStatus() {
  String json = "{";
  json += "\"uptime\":" + String(millis()/1000) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"lastData\":\"" + lastSensorData + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// 404 Hata sayfasi
void handleNotFound() {
  server.send(404, "text/plain", "404: Sayfa bulunamadi!");
}

// ========================================
// NTP ZAMAN DAMGASI FONKSIYONU
// ========================================
String getFormattedTime() {
  time_t now = time(nullptr);
  
  // Zaman senkronize olmamışsa sistem zamanı kullan
  if (now < 100000) {
    return String(millis() / 1000) + "s (NTP yok)";
  }
  
  struct tm* timeinfo = localtime(&now);
  
  char buffer[64];
  // Format: 2025-11-21 14:30:45
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  
  return String(buffer);
}
