/*
 * =====================================================
 * AKILLI TARIM SISTEMI - YER ISTASYONU (ALICI)
 * =====================================================
 * 
 * Bu kod LoRa E32 modülü ile gelen sensör verilerini alır
 * ve detaylı bir şekilde Serial monitörde gösterir.
 * 
 * Arduino IDE Kurulum:
 * 1. Library Manager'dan "EByte LoRa E32 library" kütüphanesini kurun
 * 2. Bu kodu Arduino IDE'ye yükleyin
 * 3. Serial Monitor'u açın (9600 baud)
 * 
 * Donanım Bağlantıları:
 * - LoRa RX -> Arduino Pin 10
 * - LoRa TX -> Arduino Pin 11
 * - LoRa M0 -> Arduino Pin 6
 * - LoRa M1 -> Arduino Pin 7
 * - LoRa VCC -> 5V
 * - LoRa GND -> GND
 * 
 * Hazırlayan: Akıllı Tarım Sistemi
 * Tarih: 2025
 * =====================================================
 */

#include "Arduino.h"
#include "LoRa_E32.h"

// LoRa Pin Tanımlamaları
#define M0_PIN 6
#define M1_PIN 7

// LoRa E32 modülü (RX=10, TX=11)
LoRa_E32 e32ttl100(10, 11);

// Veri paketi yapısı - VERİCİ ile AYNI OLMALI!
#pragma pack(push,1)
struct SensorDataPacket {
  // BME680 verileri
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
  
  // BH1750 verileri
  float lux;
  
  // MH-Z14A verileri
  uint16_t co2_ppm;
  int8_t co2_temperature;
  
  // Toprak nem sensoru
  float soil_moisture_percent;
  uint16_t soil_moisture_raw;
  
  // Kontrol durumlari
  uint8_t roof_position;        // 0-100%
  uint8_t pump_state;           // 0=KAPALI, 1=ACIK
  uint16_t irrigation_duration; // Sulama suresi (saniye)
  
  // Hesaplanan degerler
  float dew_point;
  float heat_index;
  float absolute_humidity;
  
  // Sistem durumu
  uint32_t uptime;              // Sistem calisma suresi (saniye)
  uint8_t mhz14a_ready;         // MH-Z14A hazir mi? (0/1)
  
  // CRC kontrol
  uint16_t crc;
};
#pragma pack(pop)

// İstatistik değişkenleri
unsigned long totalPacketsReceived = 0;
unsigned long totalPacketsCorrupted = 0;
unsigned long lastPacketTime = 0;
unsigned long sessionStartTime = 0;

// Fonksiyon prototipleri
uint16_t calcCRC(const SensorDataPacket &packet);
void printPacketData(const SensorDataPacket &packet);
void printStatistics();
void printHeader();
void printSeparator();
String getAirQualityStatus(float gas_resistance);
String getLightStatus(float lux);
String getSoilStatus(float moisture);
String getTemperatureStatus(float temp);
String getHumidityStatus(float humidity);
String getCO2Status(uint16_t co2);

void setup() {
  // Seri haberleşmeyi başlat
  Serial.begin(9600);
  delay(500);
  
  // Başlangıç mesajı
  Serial.println(F("====================================================="));
  Serial.println(F("   AKILLI TARIM SISTEMI - YER ISTASYONU (ALICI)    "));
  Serial.println(F("====================================================="));
  Serial.println();
  
  // M0 ve M1 pinlerini ayarla (Normal mod: LOW, LOW)
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);  // NORMAL MODE
  digitalWrite(M1_PIN, LOW);
  delay(50);
  
  // LoRa modülünü başlat
  e32ttl100.begin();
  
  Serial.println(F("--- LoRa E32 Modulu Ayarlari ---"));
  Serial.println(F("Mod: NORMAL (M0=LOW, M1=LOW)"));
  Serial.println(F("RX Pin: 10"));
  Serial.println(F("TX Pin: 11"));
  Serial.print(F("Beklenen Paket Boyutu: "));
  Serial.print((int)sizeof(SensorDataPacket));
  Serial.println(F(" byte"));
  Serial.println();
  
  Serial.println(F("ALICI HAZIR - Veri bekleniyor..."));
  Serial.println();
  
  sessionStartTime = millis();
}

void loop() {
  // LoRa'dan veri geldi mi kontrol et
  if (e32ttl100.available() > 0) {
    
    // Paket alma zamanını kaydet
    lastPacketTime = millis();
    
    // Binary veri al
    ResponseStructContainer rsc = e32ttl100.receiveMessage(sizeof(SensorDataPacket));
    
    // Veriyi struct'a kopyala
    SensorDataPacket packet;
    memcpy(&packet, rsc.data, sizeof(SensorDataPacket));
    rsc.close();
    
    // CRC hesapla ve doğrula
    uint16_t crc_calculated = calcCRC(packet);
    
    // Başlık yazdır
    printHeader();
    
    if (crc_calculated == packet.crc) {
      // CRC DOĞRU - Veriyi göster
      totalPacketsReceived++;
      
      Serial.println(F("*** PAKET BASARIYLA ALINDI ***"));
      Serial.println();
      
      printPacketData(packet);
      
    } else {
      // CRC HATALI
      totalPacketsCorrupted++;
      
      Serial.println(F("!!! CRC HATALI - PAKET BOZUK !!!"));
      Serial.println();
      Serial.print(F("Beklenen CRC : 0x"));
      Serial.println(packet.crc, HEX);
      Serial.print(F("Hesaplanan   : 0x"));
      Serial.println(crc_calculated, HEX);
      Serial.println();
    }
    
    // İstatistikleri göster
    printStatistics();
    
  } else {
    // Bağlantı kontrol mesajı (30 saniyede bir)
    if (millis() - lastPacketTime > 30000 && totalPacketsReceived > 0) {
      Serial.println(F("--- Veri bekleniyor... Son paket 30+ saniye once ---"));
      lastPacketTime = millis(); // Spam önleme
    }
  }
  
  delay(100); // CPU rahatlatma
}

// ========================================
// FONKSIYONLAR
// ========================================

// CRC hesaplama fonksiyonu (VERİCİ ile AYNI!)
uint16_t calcCRC(const SensorDataPacket &packet) {
  uint16_t crc = 0;
  const uint8_t* ptr = (const uint8_t*)&packet;
  // CRC alanı hariç tüm baytları topla
  for (size_t i = 0; i < sizeof(SensorDataPacket) - sizeof(packet.crc); i++) {
    crc += ptr[i];
  }
  return crc;
}

// Başlık yazdır
void printHeader() {
  Serial.println(F("====================================================="));
  Serial.println(F("        AKILLI TARIM SISTEMI - CANLI VERI           "));
  Serial.println(F("====================================================="));
  Serial.println();
}

// Ayırıcı çizgi
void printSeparator() {
  Serial.println(F("-----------------------------------------------------"));
}

// Paket verilerini detaylı yazdır
void printPacketData(const SensorDataPacket &packet) {
  
  // ========== SİSTEM BİLGİLERİ ==========
  printSeparator();
  Serial.println(F(">>> SISTEM BILGILERI <<<"));
  printSeparator();
  
  Serial.print(F("Sistem Calisma Suresi: "));
  unsigned long hours = packet.uptime / 3600;
  unsigned long minutes = (packet.uptime % 3600) / 60;
  unsigned long seconds = packet.uptime % 60;
  Serial.print(hours);
  Serial.print(F("s "));
  Serial.print(minutes);
  Serial.print(F("dk "));
  Serial.print(seconds);
  Serial.println(F("sn"));
  
  Serial.print(F("MH-Z14A CO2 Sensor: "));
  Serial.println(packet.mhz14a_ready ? F("HAZIR") : F("ISINMA ASAMASINDA"));
  
  Serial.print(F("CRC Kontrolu: 0x"));
  Serial.println(packet.crc, HEX);
  Serial.println();
  
  // ========== HAVA KALİTESİ (BME680) ==========
  printSeparator();
  Serial.println(F(">>> HAVA KALITESI (BME680) <<<"));
  printSeparator();
  
  Serial.print(F("Sicaklik       : "));
  Serial.print(packet.temperature, 2);
  Serial.print(F(" C  ["));
  Serial.print(getTemperatureStatus(packet.temperature));
  Serial.println(F("]"));
  
  Serial.print(F("Nem            : "));
  Serial.print(packet.humidity, 2);
  Serial.print(F(" %  ["));
  Serial.print(getHumidityStatus(packet.humidity));
  Serial.println(F("]"));
  
  Serial.print(F("Basinc         : "));
  Serial.print(packet.pressure, 2);
  Serial.println(F(" hPa"));
  
  Serial.print(F("Gaz Direnci    : "));
  Serial.print(packet.gas_resistance, 2);
  Serial.print(F(" KOhm  ["));
  Serial.print(getAirQualityStatus(packet.gas_resistance));
  Serial.println(F("]"));
  Serial.println();
  
  // ========== IŞIK SEVİYESİ (BH1750) ==========
  printSeparator();
  Serial.println(F(">>> ISIK SEVIYESI (BH1750) <<<"));
  printSeparator();
  
  Serial.print(F("Isik Siddeti   : "));
  Serial.print(packet.lux, 1);
  Serial.print(F(" lux  ["));
  Serial.print(getLightStatus(packet.lux));
  Serial.println(F("]"));
  
  // Foot-candles dönüşümü
  float fc = packet.lux / 10.764;
  Serial.print(F("               = "));
  Serial.print(fc, 2);
  Serial.println(F(" fc (foot-candles)"));
  Serial.println();
  
  // ========== CO2 SEVİYESİ (MH-Z14A) ==========
  printSeparator();
  Serial.println(F(">>> CO2 SEVIYESI (MH-Z14A) <<<"));
  printSeparator();
  
  Serial.print(F("CO2 Konsant.   : "));
  Serial.print(packet.co2_ppm);
  Serial.print(F(" ppm  ["));
  Serial.print(getCO2Status(packet.co2_ppm));
  Serial.println(F("]"));
  
  Serial.print(F("Sensor Sicak.  : "));
  Serial.print(packet.co2_temperature);
  Serial.println(F(" C"));
  
  Serial.print(F("Sensor Durum   : "));
  Serial.println(packet.mhz14a_ready ? F("Stabil") : F("Isinma devam ediyor"));
  Serial.println();
  
  // ========== TOPRAK NEMİ ==========
  printSeparator();
  Serial.println(F(">>> TOPRAK NEM SENSORU <<<"));
  printSeparator();
  
  Serial.print(F("Toprak Nemi    : "));
  Serial.print(packet.soil_moisture_percent, 1);
  Serial.print(F(" %  ["));
  Serial.print(getSoilStatus(packet.soil_moisture_percent));
  Serial.println(F("]"));
  
  Serial.print(F("Ham Deger      : "));
  Serial.println(packet.soil_moisture_raw);
  Serial.println();
  
  // ========== HESAPLANAN DEĞERLER ==========
  printSeparator();
  Serial.println(F(">>> HESAPLANAN DEGERLER <<<"));
  printSeparator();
  
  Serial.print(F("Ciy Noktasi         : "));
  Serial.print(packet.dew_point, 2);
  Serial.println(F(" C"));
  
  Serial.print(F("Hissedilen Sicaklik : "));
  Serial.print(packet.heat_index, 2);
  Serial.println(F(" C"));
  
  Serial.print(F("Mutlak Nem          : "));
  Serial.print(packet.absolute_humidity, 2);
  Serial.println(F(" g/m3"));
  
  // Sıcaklık - Çiy noktası farkı (Kuf riski)
  float dewDiff = packet.temperature - packet.dew_point;
  Serial.print(F("Sicak-Ciy Farki     : "));
  Serial.print(dewDiff, 2);
  Serial.print(F(" C  "));
  if (dewDiff < 3.0) {
    Serial.println(F("[UYARI: Kuf riski yuksek!]"));
  } else {
    Serial.println(F("[Normal]"));
  }
  Serial.println();
  
  // ========== KONTROL SİSTEMLERİ ==========
  printSeparator();
  Serial.println(F(">>> SERA KONTROL SISTEMLERI <<<"));
  printSeparator();
  
  // Sera kapağı
  Serial.print(F("Sera Kapagi    : "));
  Serial.print(packet.roof_position);
  Serial.print(F(" %  "));
  if (packet.roof_position == 0) {
    Serial.println(F("[KAPALI]"));
  } else if (packet.roof_position < 25) {
    Serial.println(F("[AZ ACIK]"));
  } else if (packet.roof_position < 50) {
    Serial.println(F("[YARIM ACIK]"));
  } else if (packet.roof_position < 75) {
    Serial.println(F("[COK ACIK]"));
  } else if (packet.roof_position < 100) {
    Serial.println(F("[NEREDEYSE TAM ACIK]"));
  } else {
    Serial.println(F("[TAM ACIK]"));
  }
  
  // Sulama pompası
  Serial.print(F("Sulama Pompasi : "));
  if (packet.pump_state == 1) {
    Serial.print(F("ACIK  [Kalan: "));
    Serial.print(packet.irrigation_duration);
    Serial.println(F(" saniye]"));
  } else {
    Serial.println(F("KAPALI"));
  }
  Serial.println();
  
  // ========== GENEL DEĞERLENDİRME ==========
  printSeparator();
  Serial.println(F(">>> GENEL DEGERLENDIRME <<<"));
  printSeparator();
  
  // Genel sağlık skoru hesapla (basit algoritma)
  int healthScore = 0;
  
  // Sıcaklık (20-26°C ideal)
  if (packet.temperature >= 20 && packet.temperature <= 26) healthScore += 25;
  else if (packet.temperature >= 15 && packet.temperature <= 30) healthScore += 15;
  
  // Nem (50-70% ideal)
  if (packet.humidity >= 50 && packet.humidity <= 70) healthScore += 25;
  else if (packet.humidity >= 40 && packet.humidity <= 80) healthScore += 15;
  
  // CO2 (400-1000 ppm ideal)
  if (packet.mhz14a_ready && packet.co2_ppm >= 400 && packet.co2_ppm <= 1000) healthScore += 25;
  else if (packet.mhz14a_ready && packet.co2_ppm >= 300 && packet.co2_ppm <= 1500) healthScore += 15;
  
  // Toprak nemi (40-70% ideal)
  if (packet.soil_moisture_percent >= 40 && packet.soil_moisture_percent <= 70) healthScore += 25;
  else if (packet.soil_moisture_percent >= 30 && packet.soil_moisture_percent <= 80) healthScore += 15;
  
  Serial.print(F("Sera Saglik Skoru: "));
  Serial.print(healthScore);
  Serial.print(F("/100  ["));
  
  if (healthScore >= 80) {
    Serial.println(F("MUKEMMEL]"));
  } else if (healthScore >= 60) {
    Serial.println(F("IYI]"));
  } else if (healthScore >= 40) {
    Serial.println(F("ORTA]"));
  } else {
    Serial.println(F("ZAYIF - DIKKAT GEREKLI]"));
  }
  
  // Uyarılar
  bool hasWarning = false;
  Serial.println();
  Serial.println(F("Aktif Uyarilar:"));
  
  if (packet.temperature > 32) {
    Serial.println(F("  [!] ASIRI SICAK - Acil havalandirma gerekli"));
    hasWarning = true;
  }
  if (packet.temperature < 10) {
    Serial.println(F("  [!] DONMA RISKI - Sera kapali tutulmali"));
    hasWarning = true;
  }
  if (packet.humidity > 85) {
    Serial.println(F("  [!] YUKSEK NEM - Kuf riski"));
    hasWarning = true;
  }
  if (packet.mhz14a_ready && packet.co2_ppm > 2000) {
    Serial.println(F("  [!] YUKSEK CO2 - Havalandirma gerekli"));
    hasWarning = true;
  }
  if (packet.soil_moisture_percent < 20) {
    Serial.println(F("  [!] TOPRAK COK KURU - Acil sulama gerekli"));
    hasWarning = true;
  }
  if (packet.soil_moisture_percent > 90) {
    Serial.println(F("  [!] ASIRI SULAMA - Sulama durdur"));
    hasWarning = true;
  }
  if (dewDiff < 3.0 && packet.humidity > 80) {
    Serial.println(F("  [!] KUF RISKI YUKSEK"));
    hasWarning = true;
  }
  
  if (!hasWarning) {
    Serial.println(F("  Uyari yok - Tum sistemler normal"));
  }
  
  Serial.println();
}

// İstatistikleri göster
void printStatistics() {
  printSeparator();
  Serial.println(F(">>> ILETISIM ISTATISTIKLERI <<<"));
  printSeparator();
  
  unsigned long sessionTime = (millis() - sessionStartTime) / 1000;
  
  Serial.print(F("Oturum Suresi     : "));
  Serial.print(sessionTime / 60);
  Serial.print(F(" dk "));
  Serial.print(sessionTime % 60);
  Serial.println(F(" sn"));
  
  Serial.print(F("Basarili Paket    : "));
  Serial.println(totalPacketsReceived);
  
  Serial.print(F("Bozuk Paket       : "));
  Serial.println(totalPacketsCorrupted);
  
  unsigned long totalPackets = totalPacketsReceived + totalPacketsCorrupted;
  if (totalPackets > 0) {
    float successRate = (float)totalPacketsReceived / totalPackets * 100.0;
    Serial.print(F("Basari Orani      : "));
    Serial.print(successRate, 1);
    Serial.println(F(" %"));
  }
  
  if (totalPacketsReceived > 0 && sessionTime > 0) {
    float packetsPerMinute = (float)totalPacketsReceived / (sessionTime / 60.0);
    Serial.print(F("Paket Hizi        : "));
    Serial.print(packetsPerMinute, 1);
    Serial.println(F(" paket/dk"));
  }
  
  printSeparator();
  Serial.println();
}

// ========================================
// DURUM DEĞERLENDİRME FONKSİYONLARI
// ========================================

String getAirQualityStatus(float gas_resistance) {
  if (gas_resistance > 50) return "Iyi";
  if (gas_resistance > 20) return "Orta";
  return "Kotu";
}

String getLightStatus(float lux) {
  if (lux < 1) return "Karanlik";
  if (lux < 10) return "Cok Los";
  if (lux < 50) return "Los";
  if (lux < 200) return "Orta";
  if (lux < 1000) return "Parlak";
  if (lux < 10000) return "Cok Parlak";
  return "Gunes Isigi";
}

String getSoilStatus(float moisture) {
  if (moisture < 20) return "Cok Kuru - ACIL!";
  if (moisture < 40) return "Kuru";
  if (moisture < 60) return "Optimal";
  if (moisture < 80) return "Nemli";
  return "Cok Islak - UYARI!";
}

String getTemperatureStatus(float temp) {
  if (temp < 10) return "Cok Soguk - Donma Riski";
  if (temp < 15) return "Soguk";
  if (temp < 20) return "Serin";
  if (temp < 26) return "Ideal";
  if (temp < 30) return "Sicak";
  if (temp < 35) return "Cok Sicak";
  return "ASIRI SICAK - TEHLIKE";
}

String getHumidityStatus(float humidity) {
  if (humidity < 30) return "Cok Kuru";
  if (humidity < 50) return "Kuru";
  if (humidity < 70) return "Ideal";
  if (humidity < 85) return "Nemli";
  return "Cok Nemli - Kuf Riski";
}

String getCO2Status(uint16_t co2) {
  if (co2 < 400) return "Cok Dusuk";
  if (co2 < 800) return "Mukemmel";
  if (co2 < 1000) return "Iyi";
  if (co2 < 1500) return "Orta";
  if (co2 < 2000) return "Kotu";
  return "Cok Kotu - TEHLIKE";
}
