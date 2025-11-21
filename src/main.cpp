#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include "LoRa_E32.h"
#include <Servo.h>
#include "Communication.h"     // Haberlesme modulu
#include "Sensors.h"           // Sensor modulu (Kalman filtreli)
#include "JSONFormatter.h"     // JSON formatter modulu
#include "SerialCommands.h"    // Serial komut isleme modulu
#include "GreenhouseControl.h" // Sera kontrol modulu
#include "IrrigationControl.h" // Sulama kontrol modulu

// I2C Pin tanimlari (Arduino Mega icin D20=SDA, D21=SCL)
#define I2C_SDA 20
#define I2C_SCL 21

// UART Pin tanimlari (Arduino Mega icin D19=RX1, D18=TX1)
#define MHZ14A_RX 19  // Arduino RX -> MH-Z14A TX
#define MHZ14A_TX 18  // Arduino TX -> MH-Z14A RX

// LoRa E32 Pin tanimlari (Software Serial: D10=RX, D11=TX)
#define LORA_M0_PIN 6   // LoRa M0 kontrol pini
#define LORA_M1_PIN 8   // LoRa M1 kontrol pini
// LoRa Software Serial: RX=D10, TX=D11 (e32ttl100 nesnesinde tanimli)

// Kontrol Pin Tanimlari
#define SERVO_PIN 9          // MG995 Servo - Sera kapagi (0°=Acik, 95°=Kapali)
#define LIGHT_RELAY_PIN 29   // Aydinlatma rolesi (D7'den D29'a tasindi - LoRa cakismasi onlendi)
#define FAN_RELAY_PIN 30     // Fan rolesi
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
JSONFormatter jsonFormatter;        // JSON formatter modulu

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
void sendLoRaData();

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
  
  // NodeMCU haberlesme baslat (Serial2 - JSON format)
  comm.initNodeMCU();
  
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
  Serial.println(F("Isik Rolesi (D29): KAPALI"));
  Serial.println(F("Fan Rolesi (D30): KAPALI"));
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
  // Sensör okumalarını al (Kalman filtreli değerler)
  SensorReadings& readings = sensors.getReadings();
  
  // JSON formatinda veri hazirlama
  // LoRa icin COMPACT JSON kullan (filtrelenmis veriler, max 200 byte)
  const char* loraJson = jsonFormatter.createCompactJSON(
    readings.temperature_filtered,
    readings.humidity_filtered,
    readings.pressure_filtered,
    readings.gas_resistance_filtered,
    readings.lux_filtered,
    (int)readings.co2_ppm_filtered,
    readings.soil_moisture_percent_filtered,
    currentRoofPosition,
    fanOn,
    lightOn,
    isPumpOn,
    millis() / 1000
  );
  
  // LoRa ile JSON gonder
  comm.sendLoRaJSON(loraJson);
  
  // NodeMCU icin FIREBASE JSON kullan (Firebase & SD kart icin optimized)
  const char* firebaseJson = jsonFormatter.createFirebaseJSON(
    readings.temperature_filtered,
    readings.humidity_filtered,
    readings.pressure_filtered,
    readings.gas_resistance_filtered,
    readings.lux_filtered,
    (int)readings.co2_ppm_filtered,
    readings.soil_moisture_percent_filtered,
    calculateDewPoint(bme.temperature, bme.humidity),
    calculateHeatIndex(bme.temperature, bme.humidity),
    currentRoofPosition,
    fanOn,
    lightOn,
    isPumpOn,
    millis() / 1000
  );
  
  // NodeMCU'ya Serial2 ile JSON gonder (Firebase & SD kart icin)
  comm.sendToNodeMCU(firebaseJson);
}
