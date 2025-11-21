#include "Communication.h"

// Constructor
Communication::Communication(LoRa_E32& lora, int m0Pin, int m1Pin) 
  : _lora(lora), _m0Pin(m0Pin), _m1Pin(m1Pin), _jsonFormatter() {
}

// Initialize serial communication
void Communication::begin() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
}

// Initialize LoRa module
void Communication::initLoRa() {
  Serial.println(F("\n--- LoRa E32 Modulu Baslatiliyor ---"));
  
  // M0 ve M1 pinlerini ayarla (Normal mod: LOW, LOW)
  pinMode(_m0Pin, OUTPUT);
  pinMode(_m1Pin, OUTPUT);
  digitalWrite(_m0Pin, LOW);  // NORMAL MODE
  digitalWrite(_m1Pin, LOW);
  delay(50);
  
  // LoRa modulu baslat
  _lora.begin();
  
  Serial.print(F("LoRa VERICI hazir. Paket boyutu: "));
  Serial.println((int)sizeof(SensorDataPacket));
  Serial.print(F("LoRa RX Pin: 10, TX Pin: 11"));
  Serial.println(F("\nLoRa M0=LOW, M1=LOW (Normal Mode)"));
  Serial.println(F("LoRa Format: JSON (Yeni)"));
  Serial.println();
}

// Initialize NodeMCU communication (Serial2)
void Communication::initNodeMCU() {
  Serial2.begin(115200);  // NodeMCU ile 115200 baud
  delay(100);
  
  Serial.println(F("\n--- NodeMCU Haberlesme Baslatiliyor ---"));
  Serial.println(F("Serial2 (TX2=D16, RX2=D17) - 115200 baud"));
  Serial.println(F("NodeMCU Format: JSON (Firebase Optimize)"));
  Serial.println();
  
  // Test mesajı gönder
  Serial2.println(F("{\"status\":\"Arduino Mega Ready\"}"));
}

// Print system header
void Communication::printHeader() {
  Serial.println(F("=== Akilli Tarim Sistemi ==="));
  Serial.println(F("BH1750 (Isik) + BME680 (Hava) + MH-Z14A (CO2) + Toprak Nem"));
  Serial.println();
}

// Print command help
void Communication::printCommandHelp() {
  Serial.println(F("\n=== Serial Komutlar ==="));
  Serial.println(F("havaac    = Kapak AC + Fan AC"));
  Serial.println(F("havakapa  = Kapak KAPAT + Fan KAPAT"));
  Serial.println(F("isikac    = Isik AC"));
  Serial.println(F("isikkapa  = Isik KAPAT"));
  Serial.println(F("sulaac    = Sulama AC"));
  Serial.println(F("sulakapa  = Sulama KAPAT"));
  Serial.println();
}

// Clear screen (ANSI escape codes)
void Communication::clearScreen() {
  Serial.write(27);       // ESC
  Serial.print(F("[2J")); // Ekrani temizle
  Serial.write(27);       // ESC
  Serial.print(F("[H"));  // Imleci en uste getir
}

// Print debug message
void Communication::printDebug(const String& message) {
  Serial.print(F("[DEBUG] "));
  Serial.println(message);
}

// Print info message
void Communication::printInfo(const String& message) {
  Serial.print(F("[INFO] "));
  Serial.println(message);
}

// Print warning message
void Communication::printWarning(const String& message) {
  Serial.print(F("[UYARI] "));
  Serial.println(message);
}

// Print error message
void Communication::printError(const String& message) {
  Serial.print(F("[HATA] "));
  Serial.println(message);
}

// Print success message
void Communication::printSuccess(const String& message) {
  Serial.print(F("[OK] "));
  Serial.println(message);
}

// Calculate CRC
uint16_t Communication::calcCRC(const SensorDataPacket& packet) {
  uint16_t crc = 0;
  const uint8_t* ptr = (const uint8_t*)&packet;
  // CRC alanı hariç tüm baytları topla
  for (size_t i = 0; i < sizeof(SensorDataPacket) - sizeof(packet.crc); i++) {
    crc += ptr[i];
  }
  return crc;
}

// HEX dump function (for debugging)
void Communication::hexDump(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 16 == 0) Serial.println();
  }
  Serial.println();
}

// Print packet summary
void Communication::printPacketSummary(const SensorDataPacket& packet) {
  Serial.println(F("\n[DEBUG] Gonderilecek Paket Ozeti:"));
  Serial.print(F("  Sicaklik: "));
  Serial.print(packet.temperature, 1);
  Serial.println(F(" C"));
  Serial.print(F("  Nem: "));
  Serial.print(packet.humidity, 1);
  Serial.println(F(" %"));
  Serial.print(F("  CO2: "));
  Serial.print(packet.co2_ppm);
  Serial.println(F(" ppm"));
  Serial.print(F("  Toprak Nem: "));
  Serial.print(packet.soil_moisture_percent, 1);
  Serial.println(F(" %"));
  Serial.print(F("  Sera Kapak: "));
  Serial.print(packet.roof_position);
  Serial.println(F(" %"));
  Serial.print(F("  Fan: "));
  Serial.println(packet.fan_state ? F("ACIK") : F("KAPALI"));
  Serial.print(F("  Isik: "));
  Serial.println(packet.light_state ? F("ACIK") : F("KAPALI"));
  Serial.print(F("  Sulama: "));
  Serial.println(packet.pump_state ? F("ACIK") : F("KAPALI"));
  Serial.print(F("  CRC: 0x"));
  Serial.println(packet.crc, HEX);
}

// Send LoRa packet
bool Communication::sendLoRaPacket(const SensorDataPacket& packet) {
  Serial.println(F("\n>>> LORA VERI GONDERIMI <<<"));
  Serial.print(F("Paket Boyutu: "));
  Serial.print((int)sizeof(SensorDataPacket));
  Serial.println(F(" byte"));
  
  // Print packet summary
  printPacketSummary(packet);
  
  // Print HEX dump
  Serial.println(F("\n[DEBUG] HEX Dump:"));
  hexDump((uint8_t*)&packet, sizeof(packet));
  
  // Send via LoRa
  ResponseStatus rs = _lora.sendMessage((uint8_t*)&packet, sizeof(packet));
  
  Serial.print(F("[LORA] Gonderim Sonucu: "));
  Serial.println(rs.getResponseDescription());
  
  bool success = (rs.code == 1);
  if (success) {
    Serial.println(F("[LORA] *** PAKET BASARIYLA GONDERILDI ***"));
  } else {
    Serial.println(F("[LORA] !!! GONDERIM HATASI !!!"));
  }
  
  Serial.println(F(">>> LORA GONDERIM BITTI <<<\n"));
  
  return success;
}

// Send LoRa packet (JSON format - NEW)
bool Communication::sendLoRaJSON(const char* jsonString) {
  Serial.println(F("\n>>> LORA JSON GONDERIMI <<<"));
  
  // JSON boyutunu kontrol et
  uint16_t jsonSize = strlen(jsonString);
  Serial.print(F("JSON Boyutu: "));
  Serial.print(jsonSize);
  Serial.println(F(" byte"));
  
  // JSON'ı yazdır
  Serial.println(F("\n[DEBUG] JSON Verisi:"));
  Serial.println(jsonString);
  
  // LoRa maksimum paket boyutu kontrolü (genelde 200-240 byte)
  if (jsonSize > 200) {
    Serial.println(F("[LORA] !!! JSON cok buyuk, gonderim iptal !!!"));
    Serial.println(F(">>> LORA GONDERIM BITTI <<<\n"));
    return false;
  }
  
  // Send via LoRa
  ResponseStatus rs = _lora.sendMessage((uint8_t*)jsonString, jsonSize);
  
  Serial.print(F("[LORA] Gonderim Sonucu: "));
  Serial.println(rs.getResponseDescription());
  
  bool success = (rs.code == 1);
  if (success) {
    Serial.println(F("[LORA] *** JSON BASARIYLA GONDERILDI ***"));
  } else {
    Serial.println(F("[LORA] !!! GONDERIM HATASI !!!"));
  }
  
  Serial.println(F(">>> LORA GONDERIM BITTI <<<\n"));
  
  return success;
}

// Send to NodeMCU (Serial2 - JSON format)
void Communication::sendToNodeMCU(const char* jsonString) {
  Serial.println(F("\n>>> NodeMCU'YA VERI GONDERIMI <<<"));
  
  // JSON boyutunu göster
  uint16_t jsonSize = strlen(jsonString);
  Serial.print(F("JSON Boyutu: "));
  Serial.print(jsonSize);
  Serial.println(F(" byte"));
  
  // JSON'ı Serial2'ye gönder (NodeMCU)
  Serial2.println(jsonString);
  
  // Doğrulama
  Serial.println(F("[Serial2] JSON NodeMCU'ya gonderildi"));
  Serial.println(F(">>> NodeMCU GONDERIM BITTI <<<\n"));
}
