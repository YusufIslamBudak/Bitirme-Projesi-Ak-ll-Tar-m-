#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>

// I2C Pin tanimlari (Arduino Mega icin D20=SDA, D21=SCL)
#define I2C_SDA 20
#define I2C_SCL 21

// UART Pin tanimlari (Arduino Mega icin D19=RX1, D18=TX1)
#define MHZ14A_RX 19  // Arduino RX -> MH-Z14A TX
#define MHZ14A_TX 18  // Arduino TX -> MH-Z14A RX

// Sensor nesneleri
BH1750 lightMeter;
Adafruit_BME680 bme;

// Deniz seviyesi basinci (hPa) - yukseklik hesaplama icin
#define SEALEVELPRESSURE_HPA (1013.25)

// MH-Z14A komutlari
byte mhz14aCmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte mhz14aResponse[9];

// Global degiskenler
int co2ppm = 0;
int co2Temperature = 0;
unsigned long mhz14aStartTime = 0;  // Baslangic zamani
bool mhz14aReady = false;            // Sensor hazir mi?
#define MHZ14A_WARMUP_TIME 180000    // 3 dakika (180000 ms)

// Fonksiyon prototipleri
void initSensors();
void readBH1750();
void readBME680();
void readMHZ14A();
int getMHZ14ACO2();
void printSensorData();

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
  
  Serial.println(F("=== Multi-Sensor System Test ==="));
  Serial.println(F("BH1750 (Light) + BME680 (Air) + MH-Z14A (CO2)"));
  Serial.println();
  
  // I2C haberlesme baslat
  Wire.begin();
  
  // UART haberlesme baslat (MH-Z14A icin)
  Serial1.begin(9600); // MH-Z14A 9600 baud kullanir
  
  // MH-Z14A baslangic zamanini kaydet
  mhz14aStartTime = millis();
  
  // Sensorleri baslat
  initSensors();
  
  delay(1000);
}

void loop() {
  // MH-Z14A isinma kontrolu
  if (!mhz14aReady) {
    unsigned long elapsedTime = millis() - mhz14aStartTime;
    if (elapsedTime >= MHZ14A_WARMUP_TIME) {
      mhz14aReady = true;
      Serial.println(F("\n*** MH-Z14A warm-up complete! ***\n"));
    }
  }
  
  // Ekrani temizle (ANSI escape code)
  Serial.write(27);       // ESC
  Serial.print(F("[2J")); // Ekrani temizle
  Serial.write(27);       // ESC
  Serial.print(F("[H"));  // Imleci en uste getir
  
  // Her 2 saniyede bir sensorleri oku
  Serial.println(F("\n--- New Reading ---"));
  
  readBH1750();
  readBME680();
  readMHZ14A();
  printSensorData();
  
  delay(2000);
}

// Sensorleri baslatma fonksiyonu
void initSensors() {
  Serial.println(F("Sensors initializing..."));
  
  // BH1750 isik sensoru baslat
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("OK BH1750 started successfully"));
  } else {
    Serial.println(F("ERROR BH1750 failed! Check I2C connection."));
    Serial.println(F("  Default address: 0x23 or 0x5C"));
  }
  
  // BME680 sensoru baslat - Once 0x77, sonra 0x76 dene
  bool bme680Found = false;
  
  Serial.println(F("Trying BME680 at 0x77..."));
  if (bme.begin(0x77, &Wire)) {
    Serial.println(F("OK BME680 found at address 0x77"));
    bme680Found = true;
  } else {
    Serial.println(F("Not found at 0x77, trying 0x76..."));
    delay(100);
    if (bme.begin(0x76, &Wire)) {
      Serial.println(F("OK BME680 found at address 0x76"));
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
    
    Serial.println(F("  BME680 parameters configured"));
    
    // Ilk okuma - sensoru hazirla
    delay(100);
    bme.performReading();
  } else {
    Serial.println(F("ERROR BME680 not found! Check I2C connection."));
    Serial.println(F("  Possible address: 0x76 or 0x77"));
  }
  
  // MH-Z14A CO2 sensoru test
  Serial.println(F("Testing MH-Z14A CO2 sensor..."));
  Serial.println(F("  UART: Serial1 (9600 baud)"));
  Serial.println(F("  Arduino RX1 (D19) -> MH-Z14A TX"));
  Serial.println(F("  Arduino TX1 (D18) -> MH-Z14A RX"));
  Serial.println(F("  *** WARM-UP TIME: 3-5 minutes ***"));
  Serial.println(F("  *** Initial readings may be inaccurate ***"));
  
  // Ilk okuma (sicak acma gecikmesi)
  delay(100);
  int testCO2 = getMHZ14ACO2();
  if (testCO2 > 0 && testCO2 < 5000) {
    Serial.print(F("OK MH-Z14A responding. CO2: "));
    Serial.print(testCO2);
    Serial.println(F(" ppm"));
  } else {
    Serial.println(F("WARNING MH-Z14A not responding properly"));
    Serial.println(F("  Check: UART connection, power, sensor warm-up"));
  }
  
  Serial.println();
}

// BH1750 isik sensorunu okuma fonksiyonu
void readBH1750() {
  float lux = lightMeter.readLightLevel();
  
  Serial.println(F("--- GY-30 (BH1750) Light Sensor ---"));
  
  if (lux < 0) {
    Serial.println(F("Error: BH1750 read failed!"));
  } else {
    Serial.print(F("Light Level: "));
    Serial.print(lux);
    Serial.println(F(" lux"));
    
    // Foot-candles donusumu
    float fc = luxToFootCandles(lux);
    Serial.print(F("Light Level: "));
    Serial.print(fc, 2);
    Serial.println(F(" fc (foot-candles)"));
    
    // Isik seviyesi yorumlama
    if (lux < 1) {
      Serial.println(F("  -> Dark"));
    } else if (lux < 10) {
      Serial.println(F("  -> Very Dim"));
    } else if (lux < 50) {
      Serial.println(F("  -> Dim"));
    } else if (lux < 200) {
      Serial.println(F("  -> Medium"));
    } else if (lux < 1000) {
      Serial.println(F("  -> Bright"));
    } else {
      Serial.println(F("  -> Very Bright"));
    }
  }
  Serial.println();
}

// BME680 sensorunu okuma fonksiyonu
void readBME680() {
  Serial.println(F("--- BME680 Air Quality Sensor ---"));
  
  if (!bme.performReading()) {
    Serial.println(F("Error: BME680 reading failed!"));
    Serial.println(F("Check: sensor connection, I2C address, power"));
    return;
  }
  
  float temp = bme.temperature;
  float pressure = bme.pressure / 100.0;
  float humidity = bme.humidity;
  float gasResistance = bme.gas_resistance / 1000.0;
  
  // Ham veriler
  Serial.println(F("--- Raw Measurements ---"));
  Serial.print(F("Temperature: "));
  Serial.print(temp, 2);
  Serial.println(F(" C"));
  
  Serial.print(F("Pressure: "));
  Serial.print(pressure, 2);
  Serial.println(F(" hPa"));
  
  Serial.print(F("Humidity: "));
  Serial.print(humidity, 2);
  Serial.println(F(" %"));
  
  Serial.print(F("Gas Resistance: "));
  Serial.print(gasResistance, 2);
  Serial.println(F(" KOhm"));
  
  // Bilimsel hesaplamalar
  Serial.println(F("--- Calculated Values ---"));
  
  // Ciy noktasi
  float dewPoint = calculateDewPoint(temp, humidity);
  Serial.print(F("Dew Point: "));
  Serial.print(dewPoint, 2);
  Serial.println(F(" C"));
  
  // Mutlak nem
  float absHumidity = calculateAbsoluteHumidity(temp, humidity);
  Serial.print(F("Absolute Humidity: "));
  Serial.print(absHumidity, 2);
  Serial.println(F(" g/m3"));
  
  // Hissedilen sicaklik
  float heatIndex = calculateHeatIndex(temp, humidity);
  Serial.print(F("Heat Index (Feels Like): "));
  Serial.print(heatIndex, 2);
  Serial.println(F(" C"));
  
  // Buhar basinci
  float vaporPressure = calculateVaporPressure(temp, humidity);
  Serial.print(F("Vapor Pressure: "));
  Serial.print(vaporPressure, 3);
  Serial.println(F(" kPa"));
  
  // Deniz seviyesi basinci
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float seaLevelPressure = calculateSeaLevelPressure(pressure, altitude, temp);
  Serial.print(F("Sea Level Pressure: "));
  Serial.print(seaLevelPressure, 2);
  Serial.println(F(" hPa"));
  
  Serial.print(F("Altitude: "));
  Serial.print(altitude, 2);
  Serial.println(F(" m"));
  
  // Hava kalitesi yorumu (gaz direncine gore)
  Serial.print(F("Air Quality: "));
  if (gasResistance > 50) {
    Serial.println(F("Good"));
  } else if (gasResistance > 20) {
    Serial.println(F("Moderate"));
  } else {
    Serial.println(F("Poor"));
  }
  
  Serial.println();
}

// MH-Z14A CO2 sensorunu okuma fonksiyonu
void readMHZ14A() {
  Serial.println(F("--- MH-Z14A CO2 Sensor ---"));
  
  // Isinma durumu kontrolu
  if (!mhz14aReady) {
    unsigned long elapsedTime = millis() - mhz14aStartTime;
    unsigned long remainingTime = (MHZ14A_WARMUP_TIME - elapsedTime) / 1000;
    
    Serial.print(F("Status: WARMING UP ("));
    Serial.print(remainingTime);
    Serial.println(F(" seconds remaining)"));
    Serial.println(F("Readings may be inaccurate..."));
  }
  
  co2ppm = getMHZ14ACO2();
  
  if (co2ppm > 0) {
    Serial.println(F("--- Raw Measurements ---"));
    Serial.print(F("CO2 Level: "));
    Serial.print(co2ppm);
    Serial.println(F(" ppm"));
    
    // 5000 ppm hata kontrolu
    if (co2ppm == 5000) {
      Serial.println(F("  -> SENSOR NOT READY (warm-up in progress)"));
    } else if (co2ppm > 5000) {
      Serial.println(F("  -> ERROR: Invalid reading"));
    } else {
      Serial.print(F("Sensor Temp: "));
      Serial.print(co2Temperature);
      Serial.println(F(" C"));
      
      // Bilimsel hesaplamalar - BME680 verilerini kullan
      if (bme.temperature > 0) {
        Serial.println(F("--- Calculated Values ---"));
        
        float temp = bme.temperature;
        float pressure = bme.pressure / 100.0;
        
        // CO2 konsantrasyonu mg/m3
        float co2MgPerM3 = co2PpmToMgPerM3(co2ppm, temp, pressure);
        Serial.print(F("CO2 Concentration: "));
        Serial.print(co2MgPerM3, 2);
        Serial.println(F(" mg/m3"));
        
        // Havalandirma onerisi (ASHRAE 62.1 standardina gore)
        // 1000 ppm'in ustunde kisi basina 10 L/s havalandirma
        if (co2ppm > 1000) {
          float ventilationRate = (co2ppm - 400) / 60.0; // L/s/person yaklasik
          Serial.print(F("Recommended Ventilation: "));
          Serial.print(ventilationRate, 1);
          Serial.println(F(" L/s per person"));
        }
      }
      
      // CO2 seviyesi yorumlama - sadece sensor hazirsa
      if (mhz14aReady) {
        Serial.print(F("Air Quality: "));
        if (co2ppm < 400) {
          Serial.println(F("ERROR - Too Low (Check sensor)"));
        } else if (co2ppm < 800) {
          Serial.println(F("Excellent"));
        } else if (co2ppm < 1000) {
          Serial.println(F("Good"));
        } else if (co2ppm < 1500) {
          Serial.println(F("Moderate"));
        } else if (co2ppm < 2000) {
          Serial.println(F("Poor"));
        } else {
          Serial.println(F("Very Poor - Ventilation Needed!"));
        }
      } else {
        Serial.println(F("Air Quality: WARMING UP - wait for stable reading"));
      }
    }
  } else {
    Serial.println(F("Error: MH-Z14A read failed!"));
    Serial.println(F("Check: UART connection, sensor power"));
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
  Serial.println(F(" seconds"));
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