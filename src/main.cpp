#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include "LoRa_E32.h"
// #include <Servo.h>  // Servo motor icin - servo baglaninca yorum satirini kaldir

// I2C Pin tanimlari (Arduino Mega icin D20=SDA, D21=SCL)
#define I2C_SDA 20
#define I2C_SCL 21

// UART Pin tanimlari (Arduino Mega icin D19=RX1, D18=TX1)
#define MHZ14A_RX 19  // Arduino RX -> MH-Z14A TX
#define MHZ14A_TX 18  // Arduino TX -> MH-Z14A RX

// LoRa E32 Pin tanimlari (Arduino Mega Serial2: D17=RX2, D16=TX2)
#define LORA_M0_PIN 6   // LoRa M0 kontrol pini
#define LORA_M1_PIN 7   // LoRa M1 kontrol pini
// LoRa Serial2 kullanacak: RX2=D17, TX2=D16 (donanim UART)

// Servo motor pin tanimlamasi
#define SERVO_PIN 9   // Kapak servo motoru D9'a bagli

// Role (Sulama pompasi) pin tanimlamasi
#define PUMP_RELAY_PIN 10  // Sulama pompasi role D10'a bagli

// Analog Pin tanimlari
#define SOIL_MOISTURE_PIN A0  // MH Water Sensor (Toprak nem sensoru)

// Sensor nesneleri
BH1750 lightMeter;
Adafruit_BME680 bme;
LoRa_E32 e32ttl100(10, 11);  // LoRa modulu (RX=10, TX=11 - Software Serial)
// Servo roofServo;  // Sera kapak servo motoru - servo baglaninca yorum satirini kaldir

// Deniz seviyesi basinci (hPa) - yukseklik hesaplama icin
#define SEALEVELPRESSURE_HPA (1013.25)

// LoRa veri paketi yapisi - Tum sensor verileri
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
void initLoRa();
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
uint16_t calcCRC(const SensorDataPacket& packet);
void hexDump(const uint8_t *data, size_t len);

// Bilimsel hesaplama fonksiyonlari
float calculateDewPoint(float temp, float humidity);
float calculateAbsoluteHumidity(float temp, float humidity);
float calculateHeatIndex(float temp, float humidity);
float calculateVaporPressure(float temp, float humidity);
float calculateSeaLevelPressure(float pressure, float altitude, float temp);
float luxToFootCandles(float lux);
float co2PpmToMgPerM3(int ppm, float temp, float pressure);

void setup() {
  // Seri haberlesme baslat (Debug icin)
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  Serial.println(F("=== Akilli Tarim Sistemi ==="));
  Serial.println(F("BH1750 (Isik) + BME680 (Hava) + MH-Z14A (CO2) + Toprak Nem"));
  Serial.println();
  
  // I2C haberlesme baslat
  Wire.begin();
  
  // UART haberlesme baslat (MH-Z14A icin)
  Serial1.begin(9600); // MH-Z14A 9600 baud kullanir
  
  // MH-Z14A baslangic zamanini kaydet
  mhz14aStartTime = millis();
  
  // Sensorleri baslat
  initSensors();
  
  // LoRa modulu baslat
  initLoRa();
  
  // Role (sulama pompasi) pin ayari
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW);  // Baslangicta pompa kapali
  Serial.println(F("Sulama pompasi rolesi baslatildi"));
  Serial.println(F("Pompa durumu: KAPALI"));
  
  // Servo motor baslat (servo baglaninca yorum satirini kaldir)
  // roofServo.attach(SERVO_PIN);
  // roofServo.write(0);  // Baslangicta kapak kapali
  Serial.println(F("Servo motor initialized (simulated)"));
  Serial.println(F("Roof position: CLOSED (0%)"));
  
  delay(1000);
}

void loop() {
  // MH-Z14A isinma kontrolu
  if (!mhz14aReady) {
    unsigned long elapsedTime = millis() - mhz14aStartTime;
    if (elapsedTime >= MHZ14A_WARMUP_TIME) {
      mhz14aReady = true;
      Serial.println(F("\n*** MH-Z14A isinma tamamlandi! ***\n"));
    }
  }
  
  // Ekrani temizle (ANSI escape code)
  Serial.write(27);       // ESC
  Serial.print(F("[2J")); // Ekrani temizle
  Serial.write(27);       // ESC
  Serial.print(F("[H"));  // Imleci en uste getir
  
  // Her 2 saniyede bir sensorleri oku
  Serial.println(F("\n--- Yeni Okuma ---"));
  
  readBH1750();
  readBME680();
  readMHZ14A();
  readSoilMoisture();
  
  // Sera kontrol
  controlGreenhouse();
  
  // Sulama kontrol
  controlIrrigation();
  
  // LoRa ile veri gonder
  sendLoRaData();
  
  printSensorData();
  
  delay(5000);  // 5 saniye bekle
}

// Sensorleri baslatma fonksiyonu
void initSensors() {
  Serial.println(F("Sensorler baslatiliyor..."));
  
  // BH1750 isik sensoru baslat
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("OK BH1750 basariyla baslatildi"));
  } else {
    Serial.println(F("HATA BH1750 baslamadi! I2C baglantisini kontrol edin."));
    Serial.println(F("  Varsayilan adres: 0x23 veya 0x5C"));
  }
  
  // BME680 sensoru baslat - Once 0x77, sonra 0x76 dene
  bool bme680Found = false;
  
  Serial.println(F("BME680 0x77 adresinde deneniyor..."));
  if (bme.begin(0x77, &Wire)) {
    Serial.println(F("OK BME680 0x77 adresinde bulundu"));
    bme680Found = true;
  } else {
    Serial.println(F("0x77'de bulunamadi, 0x76 deneniyor..."));
    delay(100);
    if (bme.begin(0x76, &Wire)) {
      Serial.println(F("OK BME680 0x76 adresinde bulundu"));
      bme680Found = true;
    }
  }
  
  if (bme680Found) {
    // BME680 ayarlarini yapilandir
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C, 150 ms
    
    Serial.println(F("  BME680 parametreleri ayarlandi"));
    
    // Ilk okuma - sensoru hazirla
    delay(100);
    bme.performReading();
  } else {
    Serial.println(F("HATA BME680 bulunamadi! I2C baglantisini kontrol edin."));
    Serial.println(F("  Olasi adres: 0x76 veya 0x77"));
  }
  
  // MH-Z14A CO2 sensoru test
  Serial.println(F("MH-Z14A CO2 sensoru test ediliyor..."));
  Serial.println(F("  UART: Serial1 (9600 baud)"));
  Serial.println(F("  Arduino RX1 (D19) -> MH-Z14A TX"));
  Serial.println(F("  Arduino TX1 (D18) -> MH-Z14A RX"));
  Serial.println(F("  *** ISINMA SURESI: 3-5 dakika ***"));
  Serial.println(F("  *** Ilk okumalar yanlis olabilir ***"));
  
  // Ilk okuma (sicak acma gecikmesi)
  delay(100);
  int testCO2 = getMHZ14ACO2();
  if (testCO2 > 0 && testCO2 < 5000) {
    Serial.print(F("OK MH-Z14A yanit veriyor. CO2: "));
    Serial.print(testCO2);
    Serial.println(F(" ppm"));
  } else {
    Serial.println(F("UYARI MH-Z14A dogru yanit vermiyor"));
    Serial.println(F("  Kontrol: UART baglantisi, guc, sensor isinmasi"));
  }
  
  Serial.println();
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
  float temp = bme.temperature;
  float humidity = bme.humidity;
  float pressure = bme.pressure / 100.0;
  float lux = lightMeter.readLightLevel();
  
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
void initLoRa() {
  Serial.println(F("\n--- LoRa E32 Modulu Baslatiliyor ---"));
  
  // M0 ve M1 pinlerini ayarla (Normal mod: LOW, LOW)
  pinMode(LORA_M0_PIN, OUTPUT);
  pinMode(LORA_M1_PIN, OUTPUT);
  digitalWrite(LORA_M0_PIN, LOW);  // NORMAL MODE
  digitalWrite(LORA_M1_PIN, LOW);
  delay(50);
  
  // LoRa modulu baslat
  e32ttl100.begin();
  
  Serial.print(F("LoRa VERICI hazir. Paket boyutu: "));
  Serial.println((int)sizeof(SensorDataPacket));
  Serial.print(F("LoRa RX Pin: 10, TX Pin: 11"));
  Serial.println(F("\nLoRa M0=LOW, M1=LOW (Normal Mode)"));
  Serial.println();
}

// CRC hesaplama fonksiyonu
uint16_t calcCRC(const SensorDataPacket& packet) {
  uint16_t crc = 0;
  const uint8_t* ptr = (const uint8_t*)&packet;
  // CRC alanı hariç tüm baytları topla
  for (size_t i = 0; i < sizeof(SensorDataPacket) - sizeof(packet.crc); i++) {
    crc += ptr[i];
  }
  return crc;
}

// HEX dump fonksiyonu (debug icin)
void hexDump(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 16 == 0) Serial.println();
  }
  Serial.println();
}

// LoRa ile veri gonderme fonksiyonu
void sendLoRaData() {
  // Veri paketini olustur
  SensorDataPacket packet;
  
  // BME680 verileri
  packet.temperature = bme.temperature;
  packet.humidity = bme.humidity;
  packet.pressure = bme.pressure / 100.0;  // Pa -> hPa
  packet.gas_resistance = bme.gas_resistance / 1000.0;  // ohm -> Kohm
  
  // BH1750 verileri
  packet.lux = lightMeter.readLightLevel();
  
  // MH-Z14A verileri
  packet.co2_ppm = (uint16_t)co2ppm;
  packet.co2_temperature = (int8_t)co2Temperature;
  
  // Toprak nem verileri
  packet.soil_moisture_percent = soilMoisturePercent;
  packet.soil_moisture_raw = (uint16_t)soilMoistureRaw;
  
  // Kontrol durumlari
  packet.roof_position = (uint8_t)currentRoofPosition;
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
  
  // CRC hesapla
  packet.crc = calcCRC(packet);
  
  // Debug bilgileri
  Serial.println(F("\n>>> LORA VERI GONDERIMI <<<"));
  Serial.print(F("Paket Boyutu: "));
  Serial.print((int)sizeof(SensorDataPacket));
  Serial.println(F(" byte"));
  
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
  Serial.print(F("  Sulama: "));
  Serial.println(packet.pump_state ? F("ACIK") : F("KAPALI"));
  Serial.print(F("  CRC: 0x"));
  Serial.println(packet.crc, HEX);
  
  Serial.println(F("\n[DEBUG] HEX Dump:"));
  hexDump((uint8_t*)&packet, sizeof(packet));
  
  // LoRa ile gonder
  ResponseStatus rs = e32ttl100.sendMessage((uint8_t*)&packet, sizeof(packet));
  
  Serial.print(F("[LORA] Gonderim Sonucu: "));
  Serial.println(rs.getResponseDescription());
  
  if (rs.code == 1) {
    Serial.println(F("[LORA] *** PAKET BASARIYLA GONDERILDI ***"));
  } else {
    Serial.println(F("[LORA] !!! GONDERIM HATASI !!!"));
  }
  
  Serial.println(F(">>> LORA GONDERIM BITTI <<<\n"));
}
