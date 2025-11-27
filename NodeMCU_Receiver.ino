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

// ========================================
// SENSOR VERİLERİ YAPISI
// ========================================
struct SensorData {
  float temperature;      // Sıcaklık (°C)
  float humidity;         // Nem (%)
  float pressure;         // Basınç (hPa)
  float lux;              // Işık şiddeti (lux)
  int co2;                // CO2 (ppm)
  float soilMoisture;     // Toprak nem (%)
  float dewPoint;         // Çiy noktası (°C)
  float heatIndex;        // Hissedilen sıcaklık (°C)
  bool roofOpen;          // Kapak durumu
  bool fanOn;             // Fan durumu
  bool lightOn;           // Işık durumu
  bool pumpOn;            // Pompa durumu
  unsigned long timestamp; // Zaman damgası
};

SensorData currentSensors = {0};

// ========================================
// KARAR AĞACI AYARLARI
// ========================================
bool autoControlEnabled = true;  // Otomatik kontrol aktif mi?
unsigned long lastDecisionTime = 0;
const unsigned long DECISION_INTERVAL = 10000; // Her 10 saniyede bir karar ver

// Son gönderilen komutları takip et (tekrar önleme)
String lastRoofCommand = "";
String lastLightCommand = "";
String lastWaterCommand = "";
unsigned long lastRoofCommandTime = 0;
unsigned long lastLightCommandTime = 0;
unsigned long lastWaterCommandTime = 0;
const unsigned long COMMAND_COOLDOWN = 30000; // Aynı komut 30 saniye içinde tekrar gönderilmez

// Fonksiyon prototipleri (Forward declarations)
void sendCommandToArduino(String command);
void handleRoot();
void handleCommand();
void handleStatus();
void handleNotFound();
String getFormattedTime();
float parseJsonFloat(String json, String key);
int parseJsonInt(String json, String key);
bool parseJsonBool(String json, String key);
void parseSensorData(String json);
void makeDecision();
void sendCommandSafe(String command, String& lastCmd, unsigned long& lastTime);

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

  // Otomatik karar ağacı (belirli aralıklarla)
  if (autoControlEnabled && (millis() - lastDecisionTime >= DECISION_INTERVAL)) {
    makeDecision();
    lastDecisionTime = millis();
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
    
    // JSON'u parse et ve sensör verilerini güncelle
    parseSensorData(jsonBuffer);

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
  html += "<div style='text-align:center;margin:10px 0'>";
  html += "<button style='padding:10px 20px;font-size:14px;background:";
  html += autoControlEnabled ? "#4CAF50" : "#f44336";
  html += ";color:white;border:none;border-radius:5px;cursor:pointer' onclick='toggleAuto()'>";
  html += autoControlEnabled ? "🤖 Otomatik Kontrol: AÇIK" : "🔴 Otomatik Kontrol: KAPALI";
  html += "</button>";
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
  html += "function toggleAuto(){fetch('/command?cmd=toggleauto').then(r=>r.text()).then(d=>{alert(d);location.reload()})}";
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
    
    // Otomatik kontrol aç/kapa
    if (cmd == "toggleauto") {
      autoControlEnabled = !autoControlEnabled;
      String msg = autoControlEnabled ? "Otomatik kontrol AÇILDI" : "Otomatik kontrol KAPATILDI";
      Serial.println("[WEB] " + msg);
      server.send(200, "text/plain", msg);
      return;
    }
    
    // Manuel komut listesi kontrolu
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

// ========================================
// JSON PARSING FONKSIYONLARI
// ========================================
float parseJsonFloat(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\":");
  if (keyIndex == -1) return 0.0;
  
  int valueStart = keyIndex + key.length() + 3;
  int valueEnd = json.indexOf(',', valueStart);
  if (valueEnd == -1) valueEnd = json.indexOf('}', valueStart);
  
  String value = json.substring(valueStart, valueEnd);
  return value.toFloat();
}

int parseJsonInt(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\":");
  if (keyIndex == -1) return 0;
  
  int valueStart = keyIndex + key.length() + 3;
  int valueEnd = json.indexOf(',', valueStart);
  if (valueEnd == -1) valueEnd = json.indexOf('}', valueStart);
  
  String value = json.substring(valueStart, valueEnd);
  return value.toInt();
}

bool parseJsonBool(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\":");
  if (keyIndex == -1) return false;
  
  int valueStart = keyIndex + key.length() + 3;
  String substr = json.substring(valueStart, valueStart + 4);
  
  return (substr.indexOf("true") >= 0);
}

// ========================================
// SENSOR VERİLERİNİ PARSE ET
// ========================================
void parseSensorData(String json) {
  currentSensors.temperature = parseJsonFloat(json, "temp");
  currentSensors.humidity = parseJsonFloat(json, "hum");
  currentSensors.pressure = parseJsonFloat(json, "pres");
  currentSensors.lux = parseJsonFloat(json, "lux");
  currentSensors.co2 = parseJsonInt(json, "co2");
  currentSensors.soilMoisture = parseJsonFloat(json, "soil");
  currentSensors.dewPoint = parseJsonFloat(json, "dew");
  currentSensors.heatIndex = parseJsonFloat(json, "heat");
  currentSensors.roofOpen = parseJsonBool(json, "roof");
  currentSensors.fanOn = parseJsonBool(json, "fan");
  currentSensors.lightOn = parseJsonBool(json, "light");
  currentSensors.pumpOn = parseJsonBool(json, "pump");
  currentSensors.timestamp = millis();
  
  Serial.println("\n[SENSOR DATA PARSED]");
  Serial.print("Temp: "); Serial.print(currentSensors.temperature); Serial.println(" °C");
  Serial.print("Hum: "); Serial.print(currentSensors.humidity); Serial.println(" %");
  Serial.print("CO2: "); Serial.print(currentSensors.co2); Serial.println(" ppm");
  Serial.print("Soil: "); Serial.print(currentSensors.soilMoisture); Serial.println(" %");
  Serial.print("Lux: "); Serial.println(currentSensors.lux);
}

// ========================================
// GÜVENLİ KOMUT GÖNDERME (Tekrar Önleme)
// ========================================
void sendCommandSafe(String command, String& lastCmd, unsigned long& lastTime) {
  unsigned long now = millis();
  
  // Aynı komut çok yakın zamanda gönderildiyse atla
  if (command == lastCmd && (now - lastTime) < COMMAND_COOLDOWN) {
    Serial.print("[SKIP] Komut yakın zamanda gönderildi: ");
    Serial.println(command);
    return;
  }
  
  // Komutu gönder
  sendCommandToArduino(command);
  lastCmd = command;
  lastTime = now;
  
  Serial.print("[AUTO] Komut gönderildi: ");
  Serial.println(command);
}

// ========================================
// AKILLI KARAR AĞACI - SERA KONTROL SİSTEMİ
// ========================================
void makeDecision() {
  Serial.println("\n========================================");
  Serial.println("KARAR AĞACI ÇALIŞIYOR...");
  Serial.println("========================================");
  
  float temp = currentSensors.temperature;
  float hum = currentSensors.humidity;
  float pres = currentSensors.pressure;
  int co2 = currentSensors.co2;
  float soil = currentSensors.soilMoisture;
  float lux = currentSensors.lux;
  float dewPoint = currentSensors.dewPoint;
  
  // Geçersiz veri kontrolü
  if (temp == 0 && hum == 0) {
    Serial.println("[UYARI] Sensör verileri henüz alınmadı, karar atlanıyor...");
    return;
  }
  
  // ============================================
  // KRİTİK ÖNCELIK 1: DONMA RİSKİ
  // ============================================
  if (temp < 10.0 || dewPoint < 5.0) {
    Serial.println(">>> KOD-7: DONMA RİSKİ! <<<");
    Serial.print("Sıcaklık: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Çiy Noktası: "); Serial.print(dewPoint); Serial.println(" °C");
    
    if (currentSensors.roofOpen || currentSensors.fanOn) {
      sendCommandSafe("havakapa", lastRoofCommand, lastRoofCommandTime);
    }
    if (currentSensors.pumpOn) {
      sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    }
    return; // Acil durum, diğer kontrollere gerek yok
  }
  
  // ============================================
  // KRİTİK ÖNCELIK 2: FIRTINA RİSKİ (Düşük Basınç)
  // ============================================
  if (pres < 985.0 && pres > 0) {
    Serial.println(">>> KOD-8: DÜŞÜK BASINÇ! Fırtına koruması <<<");
    Serial.print("Basınç: "); Serial.print(pres); Serial.println(" hPa");
    
    if (currentSensors.roofOpen || currentSensors.fanOn) {
      sendCommandSafe("havakapa", lastRoofCommand, lastRoofCommandTime);
    }
    if (currentSensors.pumpOn) {
      sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    }
    return;
  }
  
  // ============================================
  // YÜKSEK ÖNCELİK: AŞIRI SICAK + NEM
  // ============================================
  if (temp > 32.0 && hum > 70.0) {
    Serial.println(">>> KOD-1: AŞIRI SICAK+NEM! <<<");
    Serial.print("Sıcaklık: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Nem: "); Serial.print(hum); Serial.println(" %");
    
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    if (currentSensors.pumpOn) {
      sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    }
    return;
  }
  
  // ============================================
  // YÜKSEK SICAKLIK + YÜKSEK CO2
  // ============================================
  if (temp > 28.0 && co2 > 800 && co2 < 5000) {
    Serial.println(">>> KOD-2: YÜKSEK SICAK+CO2 <<<");
    Serial.print("Sıcaklık: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("CO2: "); Serial.print(co2); Serial.println(" ppm");
    
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    return;
  }
  
  // ============================================
  // YÜKSEK CO2 - Hava Değişimi Gerekli
  // ============================================
  if (co2 > 1500 && co2 < 5000 && temp > 20.0) {
    Serial.println(">>> KOD-3: YÜKSEK CO2 - Hava değişimi <<<");
    Serial.print("CO2: "); Serial.print(co2); Serial.println(" ppm");
    
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    return;
  }
  
  // ============================================
  // YÜKSEK NEM - Küf Riski
  // ============================================
  if (hum > 85.0 && temp < 25.0 && (temp - dewPoint) < 3.0) {
    Serial.println(">>> KOD-4: YÜKSEK NEM - Küf riski <<<");
    Serial.print("Nem: "); Serial.print(hum); Serial.println(" %");
    Serial.print("Temp-DewPoint: "); Serial.print(temp - dewPoint); Serial.println(" °C");
    
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    return;
  }
  
  // ============================================
  // GECE MODU - Soğuk Koruma
  // ============================================
  if (lux < 50.0 && temp < 18.0) {
    Serial.println(">>> KOD-6: GECE MODU - Soğuk koruma <<<");
    Serial.print("Işık: "); Serial.print(lux); Serial.println(" lux");
    Serial.print("Sıcaklık: "); Serial.print(temp); Serial.println(" °C");
    
    if (currentSensors.roofOpen || currentSensors.fanOn) {
      sendCommandSafe("havakapa", lastRoofCommand, lastRoofCommandTime);
    }
    if (!currentSensors.lightOn) {
      sendCommandSafe("isikac", lastLightCommand, lastLightCommandTime);
    }
    return;
  }
  
  // ============================================
  // GÜNDÜZ HAVALANDIRMASı - Normal Hava Akışı
  // ============================================
  if (lux > 10000.0 && temp > 22.0 && temp < 28.0 && co2 < 1000 && co2 > 0) {
    Serial.println(">>> KOD-5: GÜNDÜZ HAVALANDIRMASı <<<");
    Serial.print("Işık: "); Serial.print(lux); Serial.println(" lux");
    
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    if (currentSensors.lightOn && lux > 20000.0) {
      sendCommandSafe("isikkapa", lastLightCommand, lastLightCommandTime);
    }
  }
  
  // ============================================
  // SULAMA KONTROL - ACİL SULAMA
  // ============================================
  if (soil < 20.0 && soil > 0 && temp > 28.0) {
    Serial.println(">>> SULAMA-1: ACİL SULAMA! <<<");
    Serial.print("Toprak Nem: "); Serial.print(soil); Serial.println(" %");
    Serial.print("Sıcaklık: "); Serial.print(temp); Serial.println(" °C");
    
    if (!currentSensors.pumpOn) {
      sendCommandSafe("sulaac", lastWaterCommand, lastWaterCommandTime);
    }
    return;
  }
  
  // ============================================
  // SULAMA KONTROL - NORMAL SULAMA
  // ============================================
  if (soil < 40.0 && soil > 0 && temp > 20.0 && lux > 1000.0) {
    Serial.println(">>> SULAMA-2: NORMAL SULAMA <<<");
    Serial.print("Toprak Nem: "); Serial.print(soil); Serial.println(" %");
    
    if (!currentSensors.pumpOn) {
      sendCommandSafe("sulaac", lastWaterCommand, lastWaterCommandTime);
    }
    return;
  }
  
  // ============================================
  // SULAMA KONTROL - AKŞAM SULAMASI (Optimal)
  // ============================================
  if (soil < 50.0 && soil > 0 && lux < 1000.0 && temp > 15.0) {
    Serial.println(">>> SULAMA-3: AKŞAM SULAMASI (Optimal) <<<");
    Serial.print("Toprak Nem: "); Serial.print(soil); Serial.println(" %");
    
    if (!currentSensors.pumpOn) {
      sendCommandSafe("sulaac", lastWaterCommand, lastWaterCommandTime);
    }
    return;
  }
  
  // ============================================
  // AŞIRI SULAMA KORUMASIM
  // ============================================
  if (soil > 90.0 && currentSensors.pumpOn) {
    Serial.println(">>> SULAMA-5: AŞIRI SULAMA KORUMASI! <<<");
    Serial.print("Toprak Nem: "); Serial.print(soil); Serial.println(" %");
    
    sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    
    // Kurutma için havalandır
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    return;
  }
  
  // ============================================
  // YAĞMUR İPTALI
  // ============================================
  if (pres < 990.0 && pres > 0 && hum > 85.0 && currentSensors.pumpOn) {
    Serial.println(">>> SULAMA-4: YAĞMUR İPTALİ <<<");
    Serial.print("Basınç: "); Serial.print(pres); Serial.println(" hPa");
    Serial.print("Nem: "); Serial.print(hum); Serial.println(" %");
    
    sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    return;
  }
  
  // ============================================
  // KÜF RİSKİ - Sulama Durdur
  // ============================================
  if (soil > 80.0 && hum > 85.0 && temp < 22.0 && currentSensors.pumpOn) {
    Serial.println(">>> SULAMA-6: KÜF RİSKİ - Sulama durdur <<<");
    
    sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    
    // Havalandırma
    if (!currentSensors.roofOpen || !currentSensors.fanOn) {
      sendCommandSafe("havaac", lastRoofCommand, lastRoofCommandTime);
    }
    return;
  }
  
  // ============================================
  // İDEAL DURUM - Enerji Tasarrufu
  // ============================================
  if (temp >= 20.0 && temp <= 26.0 && 
      hum >= 50.0 && hum <= 70.0 && 
      co2 >= 400 && co2 <= 1000 &&
      soil >= 50.0 && soil <= 70.0) {
    
    Serial.println(">>> KOD-9: OPTIMAL KOŞULLAR - Sistem stabil <<<");
    Serial.print("Temp: "); Serial.print(temp); Serial.println(" °C");
    Serial.print("Nem: "); Serial.print(hum); Serial.println(" %");
    Serial.print("CO2: "); Serial.print(co2); Serial.println(" ppm");
    Serial.print("Toprak: "); Serial.print(soil); Serial.println(" %");
    
    // Enerji tasarrufu için gereksiz sistemleri kapat
    if (currentSensors.roofOpen || currentSensors.fanOn) {
      if (co2 < 600 && temp < 24.0) {
        sendCommandSafe("havakapa", lastRoofCommand, lastRoofCommandTime);
      }
    }
    
    if (currentSensors.pumpOn) {
      sendCommandSafe("sulakapa", lastWaterCommand, lastWaterCommandTime);
    }
    
    // Gündüz ise ışığı kapat
    if (currentSensors.lightOn && lux > 10000.0) {
      sendCommandSafe("isikkapa", lastLightCommand, lastLightCommandTime);
    }
    
    Serial.println("Sistem enerji tasarrufu modunda.");
  }
  
  // ============================================
  // GECE AYDINLATMA KONTROLÜ
  // ============================================
  if (lux < 50.0 && !currentSensors.lightOn && temp > 10.0) {
    Serial.println(">>> GECE AYDINLATMA <<<");
    Serial.print("Işık: "); Serial.print(lux); Serial.println(" lux");
    
    sendCommandSafe("isikac", lastLightCommand, lastLightCommandTime);
  }
  
  Serial.println("========================================\n");
}
