#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include "LoRa_E32.h"
#include <Servo.h>
#include "Communication.h"  // Haberlesme modulu
#include "Sensors.h"        // Sensor modulu (Kalman filtreli)

// I2C Pin tanimlari (Arduino Mega icin D20=SDA, D21=SCL)
#define I2C_SDA 20
#define I2C_SCL 21

// UART Pin tanimlari (Arduino Mega icin D19=RX1, D18=TX1)
#define MHZ14A_RX 19  // Arduino RX -> MH-Z14A TX
#define MHZ14A_TX 18  // Arduino TX -> MH-Z14A RX

// LoRa E32 Pin tanimlari (Arduino Mega Serial2: D17=RX2, D16=TX2)
#define LORA_M0_PIN 6   // LoRa M0 kontrol pini
#define LORA_M1_PIN 8   // LoRa M1 kontrol pini (D7'den D8'e tasindi)
// LoRa Serial2 kullanacak: RX2=D17, TX2=D16 (donanim UART)

// Kontrol Pin Tanimlari
#define SERVO_PIN 9          // MG995 Servo - Sera kapagi (0°=Acik, 95°=Kapali)
#define FAN_RELAY_PIN 30     // Fan rolesi
#define LIGHT_RELAY_PIN 7    // Aydinlatma rolesi
#define PUMP_RELAY_PIN 31    // Sulama pompasi rolesi

// Analog Pin tanimlari
#define SOIL_MOISTURE_PIN A0  // MH Water Sensor (Toprak nem sensoru)

// Sensor nesneleri
BH1750 lightMeter;
Adafruit_BME680 bme;
LoRa_E32 e32ttl100(10, 11);  // LoRa modulu (RX=10, TX=11 - Software Serial)
Servo mg995;  // MG995 Servo motoru

// Haberlesme ve Sensor modulleri
Communication comm(e32ttl100, LORA_M0_PIN, LORA_M1_PIN);
Sensors sensors(lightMeter, bme);  // Kalman filtreli sensor modulu

// Deniz seviyesi basinci (hPa) - yukseklik hesaplama icin
#define SEALEVELPRESSURE_HPA (1013.25)

// MH-Z14A komutlari
byte mhz14aCmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte mhz14aResponse[9];

// Global degiskenler - Sensörler
int co2ppm = 0;
int co2Temperature = 0;
unsigned long mhz14aStartTime = 0;  // Baslangic zamani
bool mhz14aReady = false;            // Sensor hazir mi?
#define MHZ14A_WARMUP_TIME 180000    // 3 dakika (180000 ms)

// Toprak nem sensoru kalibrasyonu
#define SOIL_WET_VALUE 300      // Su icinde (tamamen islak)
#define SOIL_DRY_VALUE 1023     // Havada (tamamen kuru)
int soilMoistureRaw = 0;        // Ham analog deger
float soilMoisturePercent = 0;  // Yuzde cinsinden nem

// Global degiskenler - Sera Kontrol
int currentRoofPosition = 0;         // Mevcut kapak pozisyonu (0=kapali, 100=tamamen acik)
unsigned long lastRoofAction = 0;    // Son kapak hareketi zamani
#define ROOF_ACTION_DELAY 30000      // Kapak hareketleri arasi minimum 30 saniye
String lastRoofReason = "System Start"; // Son kapak hareketi sebebi

// Serial Komut Kontrol Degiskenleri
String serialCommand = "";           // Gelen komut
bool roofOpen = false;                // Kapak durumu
bool fanOn = false;                   // Fan durumu
bool lightOn = false;                 // Isik durumu
bool pumpOn = false;                  // Pompa durumu
int currentServoPosition = 95;        // Mevcut servo pozisyonu (baslangiçta kapali)

// Sulama oncesi durum kayit degiskenleri (geri yukleme icin)
bool savedRoofOpen = false;           // Sulama oncesi kapak durumu
bool savedFanOn = false;              // Sulama oncesi fan durumu
bool savedLightOn = false;            // Sulama oncesi isik durumu
int savedServoPosition = 95;          // Sulama oncesi servo pozisyonu

// Global degiskenler - Sulama Kontrol
bool isPumpOn = false;                      // Pompa durumu
unsigned long pumpStartTime = 0;            // Pompa acilma zamani
unsigned long lastIrrigationCheck = 0;      // Son sulama kontrolu
#define IRRIGATION_CHECK_INTERVAL 10000     // Her 10 saniyede sulama kontrolu
#define IRRIGATION_LOCKOUT_TIME 600000      // Sulama sonrasi 10 dakika bekleme
unsigned long irrigationLockoutUntil = 0;   // Sulama kilidi bitiş zamani
int irrigationDuration = 0;                 // Sulama suresi (saniye)
String lastIrrigationReason = "System Start"; // Son sulama sebebi

// Fonksiyon prototipleri
void initSensors();
void readBH1750();
void readBME680();
void readMHZ14A();
void readSoilMoisture();
int getMHZ14ACO2();
void printSensorData();
void controlGreenhouse();
void controlIrrigation();
void setRoofPosition(int position, String reason);
void setPumpState(bool state, int duration, String reason);
int checkGreenhouseConditions();
bool checkIrrigationNeeded();
void sendLoRaData();
void processSerialCommand();  // Serial komut isleme fonksiyonu

// Bilimsel hesaplama fonksiyonlari
float calculateDewPoint(float temp, float humidity);
float calculateAbsoluteHumidity(float temp, float humidity);
float calculateHeatIndex(float temp, float humidity);
float calculateVaporPressure(float temp, float humidity);
float calculateSeaLevelPressure(float pressure, float altitude, float temp);
float luxToFootCandles(float lux);
float co2PpmToMgPerM3(int ppm, float temp, float pressure);

void setup() {
  // Seri haberlesme baslat (Communication modulu ile)
  comm.begin();
  comm.printHeader();
  
  // I2C haberlesme baslat
  Wire.begin();
  
  // UART haberlesme baslat (MH-Z14A icin)
  Serial1.begin(9600); // MH-Z14A 9600 baud kullanir
  
  // MH-Z14A baslangic zamanini kaydet
  mhz14aStartTime = millis();
  
  // Sensorleri baslat
  initSensors();
  
  // LoRa modulu baslat (Communication modulu ile)
  comm.initLoRa();
  
  // Kontrol pinlerini baslat
  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, HIGH);  // Baslangicta fan kapali (HIGH=kapali)
  
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  digitalWrite(LIGHT_RELAY_PIN, HIGH);  // Baslangicta isik kapali (HIGH=kapali)
  
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH);  // Baslangicta pompa kapali (HIGH=kapali)
  
  // Servo motor baslat
  mg995.attach(SERVO_PIN);
  mg995.write(95);  // Baslangicta kapak kapali (95 derece)
  delay(500);       // Servo hareket etsin diye bekle
  mg995.detach();   // PWM sinyalini kes, titreme onleme
  
  comm.printSuccess("Kontrol Sistemleri Baslatildi");
  Serial.println(F("Servo Motor (D9): KAPALI (95 derece)"));
  Serial.println(F("Fan Rolesi (D30): KAPALI"));
  Serial.println(F("Isik Rolesi (D7): KAPALI"));
  Serial.println(F("Pompa Rolesi (D31): KAPALI"));
  comm.printCommandHelp();
  
  delay(1000);
}

void loop() {
  // Serial komutlari isle (her zaman kontrol et)
  processSerialCommand();
  
  // MH-Z14A isinma kontrolu
  if (!mhz14aReady) {
    unsigned long elapsedTime = millis() - mhz14aStartTime;
    if (elapsedTime >= MHZ14A_WARMUP_TIME) {
      mhz14aReady = true;
      Serial.println(F("\n*** MH-Z14A isinma tamamlandi! ***\n"));
    }
  }
  
  // Her 5 saniyede bir sensorleri oku ve otomatik kontrol yap
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead >= 5000) {
    lastSensorRead = millis();
    
    // Ekrani temizle (Communication modulu ile)
    comm.clearScreen();
    
    Serial.println(F("\n--- Yeni Okuma ---"));
    
    // Sensors modulu ile tum sensor okumalarini yap (Kalman filtreli)
    sensors.readAllSensors();
    sensors.updateMHZ14AWarmup();
    
    // Sera kontrol (sadece otomatik modda)
    // Manuel kontrolde servo karismamasi icin devre disi
    // controlGreenhouse();
    
    // Sulama kontrol
    controlIrrigation();
    
    // LoRa ile veri gonder (Communication modulu ile)
    sendLoRaData();
  }
}

// Sensorleri baslatma fonksiyonu
void initSensors() {
  // Sensors modulu ile sensor baslatma
  sensors.begin();
  sensors.initSensors();
  sensors.setAltitude(100.0);  // Rakimi ayarla (metre)
  sensors.setSoilCalibration(SOIL_WET_VALUE, SOIL_DRY_VALUE);
  
  Serial.println(F("*** Kalman Filtresi AKTIF ***"));
  Serial.println(F("*** Her sensor hem RAW hem FILTRELENMİŞ deger verecek ***\n"));
}

// BH1750 isik sensorunu okuma fonksiyonu
void readBH1750() {
  float lux = lightMeter.readLightLevel();
  
  Serial.println(F("--- GY-30 (BH1750) Isik Sensoru ---"));
  
  if (lux < 0) {
    Serial.println(F("Hata: BH1750 okunamadi!"));
  } else {
    Serial.print(F("Isik Siddeti: "));
    Serial.print(lux);
    Serial.println(F(" lux"));
    
    // Foot-candles donusumu
    float fc = luxToFootCandles(lux);
    Serial.print(F("Isik Siddeti: "));
    Serial.print(fc, 2);
    Serial.println(F(" fc (foot-candles)"));
    
    // Isik seviyesi yorumlama
    if (lux < 1) {
      Serial.println(F("  -> Karanlik"));
    } else if (lux < 10) {
      Serial.println(F("  -> Cok Los"));
    } else if (lux < 50) {
      Serial.println(F("  -> Los"));
    } else if (lux < 200) {
      Serial.println(F("  -> Orta"));
    } else if (lux < 1000) {
      Serial.println(F("  -> Parlak"));
    } else {
      Serial.println(F("  -> Cok Parlak"));
    }
  }
  Serial.println();
}

// BME680 sensorunu okuma fonksiyonu
void readBME680() {
  Serial.println(F("--- BME680 Hava Kalitesi Sensoru ---"));
  
  if (!bme.performReading()) {
    Serial.println(F("Hata: BME680 okunamadi!"));
    Serial.println(F("Kontrol: sensor baglantisi, I2C adresi, guc"));
    return;
  }
  
  float temp = bme.temperature;
  float pressure = bme.pressure / 100.0;
  float humidity = bme.humidity;
  float gasResistance = bme.gas_resistance / 1000.0;
  
  // Ham veriler
  Serial.println(F("--- Ham Olcumler ---"));
  Serial.print(F("Sicaklik: "));
  Serial.print(temp, 2);
  Serial.println(F(" C"));
  
  Serial.print(F("Basinc: "));
  Serial.print(pressure, 2);
  Serial.println(F(" hPa"));
  
  Serial.print(F("Nem: "));
  Serial.print(humidity, 2);
  Serial.println(F(" %"));
  
  Serial.print(F("Gaz Direnci: "));
  Serial.print(gasResistance, 2);
  Serial.println(F(" KOhm"));
  
  // Bilimsel hesaplamalar
  Serial.println(F("--- Hesaplanan Degerler ---"));
  
  // Ciy noktasi
  float dewPoint = calculateDewPoint(temp, humidity);
  Serial.print(F("Ciy Noktasi: "));
  Serial.print(dewPoint, 2);
  Serial.println(F(" C"));
  
  // Mutlak nem
  float absHumidity = calculateAbsoluteHumidity(temp, humidity);
  Serial.print(F("Mutlak Nem: "));
  Serial.print(absHumidity, 2);
  Serial.println(F(" g/m3"));
  
  // Hissedilen sicaklik
  float heatIndex = calculateHeatIndex(temp, humidity);
  Serial.print(F("Hissedilen Sicaklik: "));
  Serial.print(heatIndex, 2);
  Serial.println(F(" C"));
  
  // Buhar basinci
  float vaporPressure = calculateVaporPressure(temp, humidity);
  Serial.print(F("Buhar Basinci: "));
  Serial.print(vaporPressure, 3);
  Serial.println(F(" kPa"));
  
  // Deniz seviyesi basinci
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float seaLevelPressure = calculateSeaLevelPressure(pressure, altitude, temp);
  Serial.print(F("Deniz Seviyesi Basinci: "));
  Serial.print(seaLevelPressure, 2);
  Serial.println(F(" hPa"));
  
  Serial.print(F("Yukseklik: "));
  Serial.print(altitude, 2);
  Serial.println(F(" m"));
  
  // Hava kalitesi yorumu (gaz direncine gore)
  Serial.print(F("Hava Kalitesi: "));
  if (gasResistance > 50) {
    Serial.println(F("Iyi"));
  } else if (gasResistance > 20) {
    Serial.println(F("Orta"));
  } else {
    Serial.println(F("Kotu"));
  }
  
  Serial.println();
}

// MH-Z14A CO2 sensorunu okuma fonksiyonu
void readMHZ14A() {
  Serial.println(F("--- MH-Z14A CO2 Sensoru ---"));
  
  // Isinma durumu kontrolu
  if (!mhz14aReady) {
    unsigned long elapsedTime = millis() - mhz14aStartTime;
    unsigned long remainingTime = (MHZ14A_WARMUP_TIME - elapsedTime) / 1000;
    
    Serial.print(F("Durum: ISINMA SURECI ("));
    Serial.print(remainingTime);
    Serial.println(F(" saniye kaldi)"));
    Serial.println(F("Okumalar yanlis olabilir..."));
  }
  
  co2ppm = getMHZ14ACO2();
  
  if (co2ppm > 0) {
    Serial.println(F("--- Ham Olcumler ---"));
    Serial.print(F("CO2 Seviyesi: "));
    Serial.print(co2ppm);
    Serial.println(F(" ppm"));
    
    // 5000 ppm hata kontrolu
    if (co2ppm == 5000) {
      Serial.println(F("  -> SENSOR HAZIR DEGIL (isinma devam ediyor)"));
    } else if (co2ppm > 5000) {
      Serial.println(F("  -> HATA: Gecersiz okuma"));
    } else {
      Serial.print(F("Sensor Sicakligi: "));
      Serial.print(co2Temperature);
      Serial.println(F(" C"));
      
      // Bilimsel hesaplamalar - BME680 verilerini kullan
      if (bme.temperature > 0) {
        Serial.println(F("--- Hesaplanan Degerler ---"));
        
        float temp = bme.temperature;
        float pressure = bme.pressure / 100.0;
        
        // CO2 konsantrasyonu mg/m3
        float co2MgPerM3 = co2PpmToMgPerM3(co2ppm, temp, pressure);
        Serial.print(F("CO2 Yogunlugu: "));
        Serial.print(co2MgPerM3, 2);
        Serial.println(F(" mg/m3"));
        
        // Havalandirma onerisi (ASHRAE 62.1 standardina gore)
        // 1000 ppm'in ustunde kisi basina 10 L/s havalandirma
        if (co2ppm > 1000) {
          float ventilationRate = (co2ppm - 400) / 60.0; // L/s/person yaklasik
          Serial.print(F("Onerilen Havalandirma: "));
          Serial.print(ventilationRate, 1);
          Serial.println(F(" L/s kisi basina"));
        }
      }
      
      // CO2 seviyesi yorumlama - sadece sensor hazirsa
      if (mhz14aReady) {
        Serial.print(F("Hava Kalitesi: "));
        if (co2ppm < 400) {
          Serial.println(F("HATA - Cok Dusuk (Sensoru kontrol edin)"));
        } else if (co2ppm < 800) {
          Serial.println(F("Mukemmel"));
        } else if (co2ppm < 1000) {
          Serial.println(F("Iyi"));
        } else if (co2ppm < 1500) {
          Serial.println(F("Orta"));
        } else if (co2ppm < 2000) {
          Serial.println(F("Kotu"));
        } else {
          Serial.println(F("Cok Kotu - Havalandirma Gerekli!"));
        }
      } else {
        Serial.println(F("Hava Kalitesi: ISINMA - Kararli okuma bekleniyor"));
      }
    }
  } else {
    Serial.println(F("Hata: MH-Z14A okunamadi!"));
    Serial.println(F("Kontrol: UART baglantisi, sensor gucu"));
  }
  
  Serial.println();
}

// Toprak nem sensorunu okuma fonksiyonu
void readSoilMoisture() {
  Serial.println(F("--- MH Water Sensor (Soil Moisture) ---"));
  
  // Analog deger oku (0-1023)
  soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
  
  // Yuzdeye donustur (0-100%)
  // Not: Deger ters orantili (dusuk deger = yuksek nem)
  soilMoisturePercent = map(soilMoistureRaw, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  
  // Sinir kontrolu
  if (soilMoisturePercent < 0) soilMoisturePercent = 0;
  if (soilMoisturePercent > 100) soilMoisturePercent = 100;
  
  // Ham deger
  Serial.print(F("Raw Analog Value: "));
  Serial.println(soilMoistureRaw);
  
  // Yuzde
  Serial.print(F("Soil Moisture: "));
  Serial.print(soilMoisturePercent, 1);
  Serial.println(F(" %"));
  
  // Durum yorumlama
  Serial.print(F("Soil Condition: "));
  if (soilMoisturePercent < 20) {
    Serial.println(F("Very Dry - Urgent irrigation needed!"));
  } else if (soilMoisturePercent < 40) {
    Serial.println(F("Dry - Irrigation recommended"));
  } else if (soilMoisturePercent < 60) {
    Serial.println(F("Optimal - Good moisture level"));
  } else if (soilMoisturePercent < 80) {
    Serial.println(F("Moist - Adequate moisture"));
  } else {
    Serial.println(F("Very Wet - Risk of overwatering!"));
  }
  
  Serial.println();
}

// MH-Z14A CO2 degerini alma fonksiyonu
int getMHZ14ACO2() {
  // Buffer temizle
  while (Serial1.available() > 0) {
    Serial1.read();
  }
  
  // Okuma komutunu gonder
  Serial1.write(mhz14aCmd, 9);
  
  // Yanit icin bekle
  unsigned long timeout = millis();
  int i = 0;
  
  while ((millis() - timeout) < 1000) {
    if (Serial1.available() > 0) {
      mhz14aResponse[i] = Serial1.read();
      i++;
      if (i >= 9) break;
    }
  }
  
  // Yanit kontrolu
  if (i != 9) {
    return -1; // Yanit alinamadi
  }
  
  // Checksum kontrolu
  byte checksum = 0;
  for (int j = 1; j < 8; j++) {
    checksum += mhz14aResponse[j];
  }
  checksum = 0xFF - checksum;
  checksum += 1;
  
  if (mhz14aResponse[8] != checksum) {
    return -1; // Checksum hatasi
  }
  
  // CO2 degerini hesapla
  int co2Value = (int)mhz14aResponse[2] * 256 + (int)mhz14aResponse[3];
  
  // Sicaklik degerini al (opsiyonel)
  co2Temperature = (int)mhz14aResponse[4] - 40;
  
  return co2Value;
}

// Sensor verilerini ozetleyen fonksiyon
void printSensorData() {
  Serial.println(F("================================="));
  Serial.print(F("Uptime: "));
  Serial.print(millis() / 1000);
  Serial.println(F(" saniye"));
  
  // Sera durumu
  Serial.print(F("Sera Kapagi: "));
  Serial.print(currentRoofPosition);
  Serial.print(F("% - "));
  Serial.println(lastRoofReason);
  
  // Sulama durumu
  Serial.print(F("Sulama Pompasi: "));
  if (isPumpOn) {
    unsigned long elapsedTime = (millis() - pumpStartTime) / 1000;
    Serial.print(F("ACIK ("));
    Serial.print(elapsedTime);
    Serial.print(F("/"));
    Serial.print(irrigationDuration);
    Serial.println(F(" sn)"));
  } else {
    Serial.print(F("KAPALI - "));
    Serial.println(lastIrrigationReason);
  }
  
  Serial.println(F("================================="));
}

// ========================================
// SULAMA KONTROL FONKSIYONLARI
// ========================================

// Sulama sistemini kontrol et
void controlIrrigation() {
  // Pompa aciksa, sure kontrolu yap
  if (isPumpOn) {
    unsigned long elapsedTime = millis() - pumpStartTime;
    if (elapsedTime >= (irrigationDuration * 1000)) {
      // Sulama suresi doldu, pompayı kapat
      setPumpState(false, 0, "Irrigation duration completed");
      irrigationLockoutUntil = millis() + IRRIGATION_LOCKOUT_TIME;
    }
    return; // Pompa acikken baska kontrol yapma
  }
  
  // Sulama kilidi aktifse kontrol yapma
  if (millis() < irrigationLockoutUntil) {
    return;
  }
  
  // Yeterli zaman gecmis mi?
  if (millis() - lastIrrigationCheck < IRRIGATION_CHECK_INTERVAL) {
    return;
  }
  
  lastIrrigationCheck = millis();
  
  // Sulama gerekli mi kontrol et
  if (checkIrrigationNeeded()) {
    // checkIrrigationNeeded() icerisinde setPumpState() cagrildi
    irrigationLockoutUntil = millis() + IRRIGATION_LOCKOUT_TIME;
  }
}

// Sulama gereksinimi kontrolu
bool checkIrrigationNeeded() {
  // Sensors modulunden filtrelenmis degerleri al
  SensorReadings& readings = sensors.getReadings();
  float temp = readings.temperature_filtered;
  float humidity = readings.humidity_filtered;
  float pressure = readings.pressure_filtered;
  float lux = readings.lux_filtered;
  float soilMoisturePercent = readings.soil_moisture_percent_filtered;
  
  // ============================================
  // SULAMA KODU 5: ASIRI SULAMA KORUMASI
  // ============================================
  if (soilMoisturePercent > 90.0) {
    lastIrrigationReason = "SULAMA-5: ASIRI SULAMA KORUMASI! 24 saat kilitledi";
    irrigationLockoutUntil = millis() + 86400000; // 24 saat
    // Ek aksiyon: Sera kapagini ac (kuruma icin)
    if (currentRoofPosition < 75) {
      setRoofPosition(75, "Toprak cok islak - kurutma gerekli");
    }
    return false;
  }
  
  // ============================================
  // SULAMA KODU 7: GECE SULAMA YASAGI
  // ============================================
  if (lux < 50.0 && temp < 12.0) {
    lastIrrigationReason = "SULAMA-7: GECE YASAGI. Soguk koruma";
    return false;
  }
  
  // ============================================
  // SULAMA KODU 4: YAGMUR IPTALI
  // ============================================
  if ((pressure < 990.0 && humidity > 85.0) || soilMoisturePercent > 80.0) {
    lastIrrigationReason = "SULAMA-4: YAGMUR IPTALI. Dogal nem";
    return false;
  }
  
  // ============================================
  // SULAMA KODU 6: KUF RISKI
  // ============================================
  if (soilMoisturePercent > 80.0 && humidity > 85.0 && temp < 22.0) {
    lastIrrigationReason = "SULAMA-6: KUF RISKI. Sulama durdur";
    // Ek aksiyon: Havalandirma
    if (currentRoofPosition < 40) {
      setRoofPosition(40, "Kuf riski - havalandirma");
    }
    return false;
  }
  
  // ============================================
  // SULAMA KODU 1: ACIL SULAMA
  // ============================================
  if (soilMoisturePercent < 20.0 && temp > 28.0) {
    setPumpState(true, 30, "SULAMA-1: ACIL! Kuru toprak + Yuksek sicaklik");
    // Ek aksiyon: Sera kapagini ac (buharlaşma kontrolu)
    if (currentRoofPosition < 50) {
      setRoofPosition(50, "Sulama aktif - buharlasma kontrolu");
    }
    return true;
  }
  
  // ============================================
  // SULAMA KODU 3: AKSAM SULAMA (OPTIMAL)
  // ============================================
  if (soilMoisturePercent < 50.0 && lux < 1000.0 && temp > 15.0) {
    setPumpState(true, 25, "SULAMA-3: AKSAM SULAMA. Optimal zaman");
    return true;
  }
  
  // ============================================
  // SULAMA KODU 2: NORMAL SULAMA
  // ============================================
  if (soilMoisturePercent < 40.0 && temp > 20.0 && lux > 1000.0) {
    setPumpState(true, 20, "SULAMA-2: NORMAL SULAMA. Gunduz");
    return true;
  }
  
  // ============================================
  // SULAMA KODU 8: IDEAL DURUM
  // ============================================
  if (soilMoisturePercent >= 50.0 && soilMoisturePercent <= 70.0) {
    lastIrrigationReason = "SULAMA-8: OPTIMAL NEM. Sulama gerekmiyor";
    return false;
  }
  
  // Varsayilan: Sulama yapma
  return false;
}

// Pompa durumunu ayarla
void setPumpState(bool state, int duration, String reason) {
  // Role kontrolu (role baglaninca yorum satirini kaldir)
  // digitalWrite(PUMP_RELAY_PIN, state ? HIGH : LOW);
  
  // Simule edilmis cikti (test icin)
  Serial.println(F("\n>>> SULAMA KONTROLU <<<"));
  Serial.print(F("Onceki Durum: "));
  Serial.println(isPumpOn ? F("ACIK") : F("KAPALI"));
  Serial.print(F("Yeni Durum: "));
  Serial.println(state ? F("ACIK") : F("KAPALI"));
  
  if (state) {
    Serial.print(F("Sure: "));
    Serial.print(duration);
    Serial.println(F(" saniye"));
  }
  
  Serial.print(F("Sebep: "));
  Serial.println(reason);
  
  if (state && !isPumpOn) {
    Serial.println(F("Islem: POMPA ACILIYOR..."));
    Serial.print(F("Role Pin D"));
    Serial.print(PUMP_RELAY_PIN);
    Serial.println(F(": HIGH"));
  } else if (!state && isPumpOn) {
    Serial.println(F("Islem: POMPA KAPATILIYOR..."));
    Serial.print(F("Role Pin D"));
    Serial.print(PUMP_RELAY_PIN);
    Serial.println(F(": LOW"));
  } else {
    Serial.println(F("Islem: DEGISIKLIK YOK"));
  }
  
  Serial.println(F(">>> SULAMA KONTROLU BITTI <<<\n"));
  
  // Global degiskenleri guncelle
  isPumpOn = state;
  if (state) {
    pumpStartTime = millis();
    irrigationDuration = duration;
  }
  lastIrrigationReason = reason;
}

// ========================================
// SERA KONTROL FONKSIYONLARI
// ========================================

// Sera kosullarini kontrol et ve karar ver
void controlGreenhouse() {
  // Son hareketten beri yeterli zaman gecmis mi? (Titreme onleme)
  if (millis() - lastRoofAction < ROOF_ACTION_DELAY) {
    return; // Henuz erken, bekle
  }
  
  // Koşulları kontrol et ve gerekli pozisyonu hesapla
  int requiredPosition = checkGreenhouseConditions();
  
  // Pozisyon değişmesi gerekiyorsa hareket et
  if (requiredPosition != currentRoofPosition) {
    setRoofPosition(requiredPosition, lastRoofReason);
  }
}

// Sera kosullarini degerlendirip gerekli kapak pozisyonunu dondur
int checkGreenhouseConditions() {
  float temp = bme.temperature;
  float humidity = bme.humidity;
  float pressure = bme.pressure / 100.0;
  float lux = lightMeter.readLightLevel();
  float heatIndex = calculateHeatIndex(temp, humidity);
  float dewPoint = calculateDewPoint(temp, humidity);
  
  // ============================================
  // KOD 7: DONMA RISKI (EN YUKSEK ONCELIK)
  // ============================================
  if (temp < 10.0 || dewPoint < 5.0) {
    lastRoofReason = "KOD-7: DONMA RISKI! Acil kapama";
    return 0; // %0 - Tamamen kapali
  }
  
  // ============================================
  // KOD 1: ASIRI SICAK + NEM (ACIL)
  // ============================================
  if (temp > 32.0 && humidity > 70.0 && heatIndex > 35.0) {
    lastRoofReason = "KOD-1: ASIRI SICAK+NEM! Tam havalandirma";
    return 100; // %100 - Tamamen acik
  }
  
  // ============================================
  // KOD 8: FIRTINA RISKI (BASINC DUSUK)
  // ============================================
  if (pressure < 985.0) {
    lastRoofReason = "KOD-8: DUSUK BASINC! Firtina koruması";
    return 0; // %0 - Tamamen kapali
  }
  
  // ============================================
  // KOD 2: YUKSEK SICAKLIK
  // ============================================
  if (temp > 28.0 && co2ppm > 800) {
    lastRoofReason = "KOD-2: YUKSEK SICAK+CO2. Havalandirma aktif";
    return 75; // %75 - Cok acik
  }
  
  // ============================================
  // KOD 3: YUKSEK CO2
  // ============================================
  if (co2ppm > 1500 && temp > 20.0 && mhz14aReady) {
    lastRoofReason = "KOD-3: YUKSEK CO2. Hava degisimi gerekli";
    return 50; // %50 - Yarim acik
  }
  
  // ============================================
  // KOD 4: YUKSEK NEM (KUF RISKI)
  // ============================================
  if (humidity > 85.0 && temp < 25.0 && (temp - dewPoint) < 3.0) {
    lastRoofReason = "KOD-4: YUKSEK NEM. Kuf riski onleme";
    return 40; // %40 - Orta aciklik
  }
  
  // ============================================
  // KOD 6: GECE SOGUK KORUMA
  // ============================================
  if (lux < 50.0 && temp < 18.0) {
    lastRoofReason = "KOD-6: GECE MODU. Soguk koruma";
    return 0; // %0 - Tamamen kapali
  }
  
  // ============================================
  // KOD 5: GUNDUZ HAVALANDIRMA
  // ============================================
  if (lux > 10000.0 && temp > 22.0 && temp < 28.0 && co2ppm < 1000) {
    lastRoofReason = "KOD-5: GUNDUZ HAVALANDIRMA. Normal hava akisi";
    return 25; // %25 - Parsiyel havalandirma
  }
  
  // ============================================
  // KOD 9: IDEAL DURUM
  // ============================================
  if (temp >= 20.0 && temp <= 26.0 && humidity >= 50.0 && humidity <= 70.0 
      && co2ppm >= 400 && co2ppm <= 1000) {
    lastRoofReason = "KOD-9: OPTIMAL KOSULLAR. Sistem stabil";
    return 0; // %0 - Enerji tasarrufu, kapali tut
  }
  
  // Varsayilan: Mevcut konumu koru
  return currentRoofPosition;
}

// Sera kapagini belirtilen pozisyona getir
void setRoofPosition(int position, String reason) {
  // Pozisyon sinirlari (0-100%)
  if (position < 0) position = 0;
  if (position > 100) position = 100;
  
  // Servo motor kontrolu (servo baglaninca yorum satirini kaldir)
  // int servoAngle = map(position, 0, 100, 0, 180); // 0-100% -> 0-180 derece
  // roofServo.write(servoAngle);
  
  // Simule edilmis cikti (test icin)
  Serial.println(F("\n>>> SERA KAPAGI KONTROLU <<<"));
  Serial.print(F("Onceki Pozisyon: "));
  Serial.print(currentRoofPosition);
  Serial.println(F("%"));
  Serial.print(F("Yeni Pozisyon: "));
  Serial.print(position);
  Serial.println(F("%"));
  Serial.print(F("Sebep: "));
  Serial.println(reason);
  
  if (position > currentRoofPosition) {
    Serial.println(F("Action: OPENING roof..."));
  } else if (position < currentRoofPosition) {
    Serial.println(F("Action: CLOSING roof..."));
  } else {
    Serial.println(F("Action: NO CHANGE"));
  }
  
  // Servo acisi hesapla ve goster (servo baglaninca aktif olacak)
  int servoAngle = map(position, 0, 100, 0, 180);
  Serial.print(F("Servo Angle: "));
  Serial.print(servoAngle);
  Serial.println(F(" degrees"));
  Serial.println(F(">>> END ROOF CONTROL <<<\n"));
  
  // Global degiskenleri guncelle
  currentRoofPosition = position;
  lastRoofAction = millis();
  lastRoofReason = reason;
}

// ========================================
// BILIMSEL HESAPLAMA FONKSIYONLARI
// ========================================

// Ciy noktasi hesaplama (Magnus-Tetens formulu)
// Kaynak: Alduchov & Eskridge (1996)
float calculateDewPoint(float temp, float humidity) {
  float a = 17.27;
  float b = 237.7;
  float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0);
  float dewPoint = (b * alpha) / (a - alpha);
  return dewPoint;
}

// Mutlak nem hesaplama (g/m3)
// Kaynak: Termodinamik denklemler
float calculateAbsoluteHumidity(float temp, float humidity) {
  // Doymus buhar basinci (hPa)
  float es = 6.112 * exp((17.67 * temp) / (temp + 243.5));
  // Gercek buhar basinci
  float e = es * (humidity / 100.0);
  // Mutlak nem
  float absHumidity = (e * 2.1674) / (temp + 273.15);
  return absHumidity;
}

// Hissedilen sicaklik (Heat Index - NOAA formulu)
// Kaynak: Rothfusz & NSR (1990)
float calculateHeatIndex(float temp, float humidity) {
  // Heat Index sadece yuksek sicaklik ve nemde gecerli
  if (temp < 27.0) {
    return temp; // Dusuk sicaklikta hissedilen = gercek sicaklik
  }
  
  // Fahrenheit'a donustur (formul F cinsindendir)
  float T = temp * 9.0 / 5.0 + 32.0;
  float RH = humidity;
  
  // Rothfusz regresyon denklemi
  float HI = -42.379 + 2.04901523 * T + 10.14333127 * RH 
             - 0.22475541 * T * RH - 0.00683783 * T * T 
             - 0.05481717 * RH * RH + 0.00122874 * T * T * RH 
             + 0.00085282 * T * RH * RH - 0.00000199 * T * T * RH * RH;
  
  // Celsius'a geri donustur
  float heatIndexC = (HI - 32.0) * 5.0 / 9.0;
  
  return heatIndexC;
}

// Buhar basinci hesaplama (kPa)
// Kaynak: Buck denklemi (1981)
float calculateVaporPressure(float temp, float humidity) {
  // Doymus buhar basinci (kPa) - Buck denklemi
  float es = 0.61121 * exp((18.678 - temp / 234.5) * (temp / (257.14 + temp)));
  // Gercek buhar basinci
  float e = es * (humidity / 100.0);
  return e;
}

// Deniz seviyesi basinci hesaplama
// Kaynak: Barometrik formul (ISA - International Standard Atmosphere)
float calculateSeaLevelPressure(float pressure, float altitude, float temp) {
  // P0 = P * exp((g * M * h) / (R * T))
  // g = 9.80665 m/s2, M = 0.0289644 kg/mol, R = 8.31432 J/(mol·K)
  float tempK = temp + 273.15;
  float exponent = (9.80665 * 0.0289644 * altitude) / (8.31432 * tempK);
  float P0 = pressure * exp(exponent);
  return P0;
}

// Lux -> Foot-candles donusumu
// 1 foot-candle = 10.764 lux (Kesin donusum)
float luxToFootCandles(float lux) {
  return lux / 10.764;
}

// CO2 ppm -> mg/m3 donusumu
// Kaynak: Ideal gaz yasasi (PV = nRT)
// CO2 molar kutlesi = 44.01 g/mol
float co2PpmToMgPerM3(int ppm, float temp, float pressure) {
  // Ideal gaz yasasi: C(mg/m3) = (ppm * M * P) / (R * T)
  // M = 44.01 g/mol (CO2 molar kutlesi)
  // R = 8.314 J/(mol·K)
  // P = Pascal cinsinden basinc
  // T = Kelvin cinsinden sicaklik
  
  float tempK = temp + 273.15;
  float pressurePa = pressure * 100.0; // hPa -> Pa
  
  // mg/m3 = (ppm * M * P) / (R * T)
  float concentration = (ppm * 44.01 * pressurePa) / (8.314 * tempK * 1000.0);
  
  return concentration;
}

// ========================================
// LORA HABERLESME FONKSIYONLARI
// ========================================

// LoRa modulu baslatma
// LoRa ile veri gonderme fonksiyonu
void sendLoRaData() {
  // Veri paketini olustur
  SensorDataPacket packet;
  
  // Sensör okumalarını al (Kalman filtreli değerler)
  SensorReadings& readings = sensors.getReadings();
  
  // BME680 verileri (FİLTRELİ değerler)
  packet.temperature = readings.temperature_filtered;
  packet.humidity = readings.humidity_filtered;
  packet.pressure = readings.pressure_filtered;
  packet.gas_resistance = readings.gas_resistance_filtered;
  
  // BH1750 verileri (FİLTRELİ)
  packet.lux = readings.lux_filtered;
  
  // MH-Z14A verileri (FİLTRELİ)
  packet.co2_ppm = (uint16_t)readings.co2_ppm_filtered;
  packet.co2_temperature = (int8_t)readings.co2_temperature;
  
  // Toprak nem verileri (FİLTRELİ)
  packet.soil_moisture_percent = readings.soil_moisture_percent_filtered;
  packet.soil_moisture_raw = (uint16_t)readings.soil_moisture_raw;
  
  // Kontrol durumlari
  packet.roof_position = (uint8_t)currentRoofPosition;
  packet.fan_state = fanOn ? 1 : 0;
  packet.light_state = lightOn ? 1 : 0;
  packet.pump_state = isPumpOn ? 1 : 0;
  if (isPumpOn) {
    unsigned long elapsedTime = (millis() - pumpStartTime) / 1000;
    packet.irrigation_duration = (uint16_t)(irrigationDuration - elapsedTime);
  } else {
    packet.irrigation_duration = 0;
  }
  
  // Hesaplanan degerler
  packet.dew_point = calculateDewPoint(bme.temperature, bme.humidity);
  packet.heat_index = calculateHeatIndex(bme.temperature, bme.humidity);
  packet.absolute_humidity = calculateAbsoluteHumidity(bme.temperature, bme.humidity);
  
  // Sistem durumu
  packet.uptime = millis() / 1000;  // saniye cinsinden
  packet.mhz14a_ready = mhz14aReady ? 1 : 0;
  
  // CRC hesapla (Communication modulu ile)
  packet.crc = comm.calcCRC(packet);
  
  // LoRa ile gonder (Communication modulu ile)
  comm.sendLoRaPacket(packet);
}

// ========================================
// SERIAL KOMUT ISLEME FONKSIYONU
// ========================================
void processSerialCommand() {
  // Serial'den veri gelirse oku
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Enter veya bosluk karakterinde komutu isle
    if (c == '\n' || c == '\r' || c == ' ') {
      if (serialCommand.length() > 0) {
        // Stringi trim et ve kucuk harfe cevir
        serialCommand.trim();
        serialCommand.toLowerCase();
        
        Serial.print(F("[DEBUG] Alinan komut: "));
        Serial.println(serialCommand);
        
        // Komut isleme - Sozel komutlar
        if (serialCommand == "havaac") {
          // Kapak AC + Fan AC
          Serial.println(F("\n>>> KOMUT: HAVA AC (Kapak + Fan) <<<"));
          if (currentServoPosition != 0) {
            mg995.attach(SERVO_PIN);    // Servo'yu aktif et
            mg995.write(0);              // Kapak acik (0 derece)
            delay(1000);                 // Servo hareket etsin (1 saniye)
            mg995.detach();              // PWM sinyalini kes
            currentServoPosition = 0;
            currentRoofPosition = 100;   // Otomatik kontrol ile senkronize et
            Serial.println(F("[OK] Kapak acildi (0 derece)"));
          } else {
            Serial.println(F("[INFO] Kapak zaten acik"));
          }
          digitalWrite(FAN_RELAY_PIN, LOW);  // Fan ac (LOW=acik)
          roofOpen = true;
          fanOn = true;
          Serial.println(F("[OK] Fan acildi"));
          serialCommand = "";  // Komutu temizle
          
        } else if (serialCommand == "havakapa") {
          // Kapak KAPAT + Fan KAPAT
          Serial.println(F("\n>>> KOMUT: HAVA KAPA (Kapak + Fan) <<<"));
          if (currentServoPosition != 95) {
            mg995.attach(SERVO_PIN);     // Servo'yu aktif et
            mg995.write(95);             // Kapak kapali (95 derece)
            delay(1000);                 // Servo hareket etsin (1 saniye)
            mg995.detach();              // PWM sinyalini kes
            currentServoPosition = 95;
            currentRoofPosition = 0;     // Otomatik kontrol ile senkronize et
            Serial.println(F("[OK] Kapak kapatildi (95 derece)"));
          } else {
            Serial.println(F("[INFO] Kapak zaten kapali"));
          }
          digitalWrite(FAN_RELAY_PIN, HIGH);  // Fan kapat (HIGH=kapali)
          roofOpen = false;
          fanOn = false;
          Serial.println(F("[OK] Fan kapatildi"));
          serialCommand = "";  // Komutu temizle
          
        } else if (serialCommand == "isikac") {
          // Isik AC
          Serial.println(F("\n>>> KOMUT: ISIK AC <<<"));
          digitalWrite(LIGHT_RELAY_PIN, LOW);  // Isik ac (LOW=acik)
          lightOn = true;
          Serial.println(F("[OK] Isik acildi"));
          serialCommand = "";  // Komutu temizle
          
        } else if (serialCommand == "isikkapa") {
          // Isik KAPAT
          Serial.println(F("\n>>> KOMUT: ISIK KAPA <<<"));
          digitalWrite(LIGHT_RELAY_PIN, HIGH);  // Isik kapat (HIGH=kapali)
          lightOn = false;
          Serial.println(F("[OK] Isik kapatildi"));
          serialCommand = "";  // Komutu temizle
          
        } else if (serialCommand == "sulaac") {
          // Sulama AC - Once mevcut durumlari kaydet
          Serial.println(F("\n>>> KOMUT: SULAMA AC <<<"));
          
          // Mevcut durumlari kaydet
          savedRoofOpen = roofOpen;
          savedFanOn = fanOn;
          savedLightOn = lightOn;
          savedServoPosition = currentServoPosition;
          
          Serial.println(F("[INFO] Mevcut sistem durumlari kaydedildi"));
          
          // Tum sistemleri kapat (guvenlik icin)
          // 1. Kapak kapat + Fan kapat
          if (roofOpen || fanOn) {
            mg995.attach(SERVO_PIN);
            mg995.write(95);  // Kapak kapat
            delay(1000);
            mg995.detach();
            currentServoPosition = 95;
            digitalWrite(FAN_RELAY_PIN, HIGH);  // Fan kapat
            roofOpen = false;
            fanOn = false;
            Serial.println(F("[GUVENLIK] Kapak ve fan kapatildi"));
          }
          
          // 2. Isik kapat
          if (lightOn) {
            digitalWrite(LIGHT_RELAY_PIN, HIGH);
            lightOn = false;
            Serial.println(F("[GUVENLIK] Isik kapatildi"));
          }
          
          // 3. Sulama ac
          digitalWrite(PUMP_RELAY_PIN, LOW);  // Pompa ac (LOW=acik)
          pumpOn = true;
          Serial.println(F("[OK] Sulama baslatildi"));
          Serial.println(F("[BILGI] Diger tum sistemler geri yuklenmeyi bekliyor..."));
          serialCommand = "";  // Komutu temizle
          
        } else if (serialCommand == "sulakapa") {
          // Sulama KAPAT - Onceki durumlari geri yukle
          Serial.println(F("\n>>> KOMUT: SULAMA KAPA <<<"));
          
          // Sulama pompasini kapat
          digitalWrite(PUMP_RELAY_PIN, HIGH);  // Pompa kapat (HIGH=kapali)
          pumpOn = false;
          Serial.println(F("[OK] Sulama durduruldu"));
          
          delay(500);  // Sistemin stabilize olmasi icin kisa bekleme
          
          Serial.println(F("[INFO] Onceki sistem durumlari geri yukleniyor..."));
          
          // 1. Kapak ve Fan durumunu geri yukle
          if (savedRoofOpen || savedFanOn) {
            mg995.attach(SERVO_PIN);
            mg995.write(savedServoPosition);
            delay(1000);
            mg995.detach();
            currentServoPosition = savedServoPosition;
            
            if (savedFanOn) {
              digitalWrite(FAN_RELAY_PIN, LOW);  // Fan ac
              fanOn = true;
              Serial.println(F("[GERI YUKLEME] Fan acildi"));
            }
            
            if (savedRoofOpen) {
              roofOpen = true;
              Serial.println(F("[GERI YUKLEME] Kapak acildi"));
            }
          }
          
          // 2. Isik durumunu geri yukle
          if (savedLightOn) {
            digitalWrite(LIGHT_RELAY_PIN, LOW);  // Isik ac
            lightOn = true;
            Serial.println(F("[GERI YUKLEME] Isik acildi"));
          }
          
          Serial.println(F("[OK] Tum sistemler onceki durumuna geri yuklendi"));
          serialCommand = "";  // Komutu temizle
          
        } else {
          // Gecersiz komut - sadece uyari ver, hicbir sey yapma
          Serial.print(F("\n[UYARI] Bilinmeyen komut: '"));
          Serial.print(serialCommand);
          Serial.println(F("'"));
          Serial.println(F("Gecerli komutlar: havaac, havakapa, isikac, isikkapa, sulaac, sulakapa"));
          serialCommand = "";  // Komutu temizle
        }
      }
    } else if (c >= 32 && c <= 126) {
      // Sadece yazdırılabilir karakterleri ekle (ASCII 32-126)
      serialCommand += c;
    }
  }
}
