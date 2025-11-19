#include "Sensors.h"

// Constructor
Sensors::Sensors(BH1750& light, Adafruit_BME680& bme)
  : _light(light), _bme(bme), _mhz14aStartTime(0), _mhz14aReady(false),
    _soilWetValue(300), _soilDryValue(1023), _altitude(100.0),
    _kalman_temperature(0.001, 0.5, 1.0, 25.0),    // Sıcaklık: yavaş değişim, orta güven
    _kalman_humidity(0.001, 1.0, 1.0, 50.0),       // Nem: yavaş değişim, düşük güven
    _kalman_pressure(0.0001, 0.1, 1.0, 1013.0),    // Basınç: çok yavaş, yüksek güven
    _kalman_gas(0.01, 5.0, 1.0, 100.0),            // Gaz: hızlı değişim, düşük güven
    _kalman_lux(0.01, 2.0, 1.0, 100.0),            // Işık: orta hız, orta güven
    _kalman_co2(0.01, 10.0, 1.0, 400.0),           // CO2: orta hız, düşük güven
    _kalman_soil(0.001, 2.0, 1.0, 50.0),           // Toprak: yavaş, orta güven
    _kalmanEnabled(true) {
  memset(&_readings, 0, sizeof(_readings));
}

// Initialize all sensors
void Sensors::begin() {
  _mhz14aStartTime = millis();
}

// Initialize sensors
void Sensors::initSensors() {
  Serial.println(F("Sensorler baslatiliyor..."));
  
  // BH1750 isik sensoru baslat
  if (_light.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("OK BH1750 basariyla baslatildi"));
  } else {
    Serial.println(F("HATA BH1750 baslamadi! I2C baglantisini kontrol edin."));
    Serial.println(F("  Varsayilan adres: 0x23 veya 0x5C"));
  }
  
  // BME680 sensoru baslat
  bool bme680Found = false;
  Serial.println(F("BME680 0x77 adresinde deneniyor..."));
  if (_bme.begin(0x77, &Wire)) {
    Serial.println(F("OK BME680 0x77 adresinde bulundu"));
    bme680Found = true;
  } else {
    Serial.println(F("0x77'de bulunamadi, 0x76 deneniyor..."));
    delay(100);
    if (_bme.begin(0x76, &Wire)) {
      Serial.println(F("OK BME680 0x76 adresinde bulundu"));
      bme680Found = true;
    }
  }
  
  if (bme680Found) {
    _bme.setTemperatureOversampling(BME680_OS_8X);
    _bme.setHumidityOversampling(BME680_OS_2X);
    _bme.setPressureOversampling(BME680_OS_4X);
    _bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    _bme.setGasHeater(320, 150);
    Serial.println(F("  BME680 parametreleri ayarlandi"));
    delay(100);
    _bme.performReading();
  } else {
    Serial.println(F("HATA BME680 bulunamadi!"));
  }
  
  // MH-Z14A CO2 sensoru test
  Serial.println(F("MH-Z14A CO2 sensoru test ediliyor..."));
  Serial.println(F("  UART: Serial1 (9600 baud)"));
  Serial.println(F("  Arduino RX1 (D19) -> MH-Z14A TX"));
  Serial.println(F("  Arduino TX1 (D18) -> MH-Z14A RX"));
  Serial.println(F("  *** ISINMA SURESI: 3-5 dakika ***"));
  Serial.println(F("  *** Ilk okumalar yanlis olabilir ***"));
  
  delay(100);
  int testCO2 = getMHZ14ACO2();
  if (testCO2 > 0 && testCO2 < 5000) {
    Serial.print(F("OK MH-Z14A yanit veriyor. CO2: "));
    Serial.print(testCO2);
    Serial.println(F(" ppm"));
  } else {
    Serial.println(F("UYARI MH-Z14A dogru yanit vermiyor"));
  }
  
  Serial.println();
}

// Read BH1750 light sensor
void Sensors::readBH1750() {
  float raw_lux = _light.readLightLevel();
  
  // Raw değerleri kaydet
  _readings.lux_raw = raw_lux;
  _readings.footCandles_raw = Calculations::luxToFootCandles(raw_lux);
  
  // Kalman filtresi uygula
  if (_kalmanEnabled) {
    _readings.lux_filtered = _kalman_lux.update(raw_lux);
    _readings.footCandles_filtered = Calculations::luxToFootCandles(_readings.lux_filtered);
  } else {
    _readings.lux_filtered = raw_lux;
    _readings.footCandles_filtered = _readings.footCandles_raw;
  }
  
  Serial.println(F("--- GY-30 (BH1750) Isik Sensoru ---"));
  if (raw_lux < 0) {
    Serial.println(F("Hata: BH1750 okunamadi!"));
  } else {
    Serial.print(F("RAW Isik: "));
    Serial.print(_readings.lux_raw);
    Serial.print(F(" lux | FILTRELI: "));
    Serial.print(_readings.lux_filtered);
    Serial.println(F(" lux"));
    
    Serial.print(F("RAW: "));
    Serial.print(_readings.footCandles_raw, 2);
    Serial.print(F(" fc | FILTRELI: "));
    Serial.print(_readings.footCandles_filtered, 2);
    Serial.println(F(" fc"));
    
    float lux = _readings.lux_filtered;
    if (lux < 1) Serial.println(F("  -> Karanlik"));
    else if (lux < 10) Serial.println(F("  -> Cok Los"));
    else if (lux < 50) Serial.println(F("  -> Los"));
    else if (lux < 200) Serial.println(F("  -> Orta"));
    else if (lux < 1000) Serial.println(F("  -> Parlak"));
    else Serial.println(F("  -> Cok Parlak"));
  }
  Serial.println();
}

// Read BME680 sensor
void Sensors::readBME680() {
  if (!_bme.performReading()) {
    Serial.println(F("HATA BME680 okuma basarisiz!"));
    return;
  }
  
  // Raw değerleri oku
  _readings.temperature_raw = _bme.temperature;
  _readings.humidity_raw = _bme.humidity;
  _readings.pressure_raw = _bme.pressure / 100.0;
  _readings.gas_resistance_raw = _bme.gas_resistance / 1000.0;
  
  // Kalman filtreleri uygula
  if (_kalmanEnabled) {
    _readings.temperature_filtered = _kalman_temperature.update(_readings.temperature_raw);
    _readings.humidity_filtered = _kalman_humidity.update(_readings.humidity_raw);
    _readings.pressure_filtered = _kalman_pressure.update(_readings.pressure_raw);
    _readings.gas_resistance_filtered = _kalman_gas.update(_readings.gas_resistance_raw);
  } else {
    _readings.temperature_filtered = _readings.temperature_raw;
    _readings.humidity_filtered = _readings.humidity_raw;
    _readings.pressure_filtered = _readings.pressure_raw;
    _readings.gas_resistance_filtered = _readings.gas_resistance_raw;
  }
  
  // Hesaplanan değerler (FİLTRELİ değerlerden)
  _readings.dew_point = Calculations::calculateDewPoint(_readings.temperature_filtered, _readings.humidity_filtered);
  _readings.absolute_humidity = Calculations::calculateAbsoluteHumidity(_readings.temperature_filtered, _readings.humidity_filtered);
  _readings.heat_index = Calculations::calculateHeatIndex(_readings.temperature_filtered, _readings.humidity_filtered);
  _readings.vapor_pressure = Calculations::calculateVaporPressure(_readings.temperature_filtered, _readings.humidity_filtered);
  _readings.sea_level_pressure = Calculations::calculateSeaLevelPressure(_readings.pressure_filtered, _altitude, _readings.temperature_filtered);
  _readings.altitude = _altitude;
  
  Serial.println(F("--- BME680 Hava Kalitesi Sensoru ---"));
  Serial.println(F("--- Ham Olcumler (RAW | FILTRELI) ---"));
  Serial.print(F("Sicaklik: ")); 
  Serial.print(_readings.temperature_raw); Serial.print(F(" | ")); 
  Serial.print(_readings.temperature_filtered); Serial.println(F(" C"));
  
  Serial.print(F("Basinc: ")); 
  Serial.print(_readings.pressure_raw); Serial.print(F(" | ")); 
  Serial.print(_readings.pressure_filtered); Serial.println(F(" hPa"));
  
  Serial.print(F("Nem: ")); 
  Serial.print(_readings.humidity_raw); Serial.print(F(" | ")); 
  Serial.print(_readings.humidity_filtered); Serial.println(F(" %"));
  
  Serial.print(F("Gaz Direnci: ")); 
  Serial.print(_readings.gas_resistance_raw); Serial.print(F(" | ")); 
  Serial.print(_readings.gas_resistance_filtered); Serial.println(F(" KOhm"));
  
  Serial.println(F("--- Hesaplanan Degerler (Filtreliden) ---"));
  Serial.print(F("Ciy Noktasi: ")); Serial.print(_readings.dew_point); Serial.println(F(" C"));
  Serial.print(F("Mutlak Nem: ")); Serial.print(_readings.absolute_humidity); Serial.println(F(" g/m3"));
  Serial.print(F("Hissedilen Sicaklik: ")); Serial.print(_readings.heat_index); Serial.println(F(" C"));
  Serial.print(F("Buhar Basinci: ")); Serial.print(_readings.vapor_pressure); Serial.println(F(" kPa"));
  Serial.print(F("Deniz Seviyesi Basinci: ")); Serial.print(_readings.sea_level_pressure); Serial.println(F(" hPa"));
  Serial.print(F("Yukseklik: ")); Serial.print(_readings.altitude); Serial.println(F(" m"));
  
  float gas = _readings.gas_resistance_filtered;
  if (gas > 150.0) Serial.println(F("Hava Kalitesi: Iyi"));
  else if (gas > 100.0) Serial.println(F("Hava Kalitesi: Orta"));
  else Serial.println(F("Hava Kalitesi: Zayif"));
  Serial.println();
}

// Read MH-Z14A CO2 sensor
void Sensors::readMHZ14A() {
  int raw_co2 = getMHZ14ACO2();
  
  // Raw değerleri kaydet
  _readings.co2_ppm_raw = raw_co2;
  _readings.mhz14a_ready = _mhz14aReady;
  
  // Kalman filtresi uygula
  if (_kalmanEnabled) {
    _readings.co2_ppm_filtered = _kalman_co2.update((float)raw_co2);
  } else {
    _readings.co2_ppm_filtered = (float)raw_co2;
  }
  
  _readings.co2_density = Calculations::co2PpmToMgPerM3((int)_readings.co2_ppm_filtered, 
                                                         _readings.temperature_filtered, 
                                                         _readings.pressure_filtered);
  _readings.recommended_ventilation = Calculations::calculateVentilationRate((int)_readings.co2_ppm_filtered);
  
  Serial.println(F("--- MH-Z14A CO2 Sensoru ---"));
  
  if (!_mhz14aReady) {
    unsigned long remaining = (MHZ14A_WARMUP_TIME - (millis() - _mhz14aStartTime)) / 1000;
    Serial.print(F("Durum: ISINMA SURECI ("));
    Serial.print(remaining);
    Serial.println(F(" saniye kaldi)"));
    Serial.println(F("Okumalar yanlis olabilir..."));
  }
  
  Serial.println(F("--- Ham Olcumler (RAW | FILTRELI) ---"));
  Serial.print(F("CO2: ")); 
  Serial.print(_readings.co2_ppm_raw); Serial.print(F(" | ")); 
  Serial.print((int)_readings.co2_ppm_filtered); Serial.println(F(" ppm"));
  
  Serial.print(F("Sensor Sicakligi: ")); Serial.print(_readings.co2_temperature); Serial.println(F(" C"));
  
  Serial.println(F("--- Hesaplanan Degerler (Filtreliden) ---"));
  Serial.print(F("CO2 Yogunlugu: ")); Serial.print(_readings.co2_density); Serial.println(F(" mg/m3"));
  Serial.print(F("Onerilen Havalandirma: ")); Serial.print(_readings.recommended_ventilation); Serial.println(F(" L/s kisi basina"));
  
  if (!_mhz14aReady) {
    Serial.println(F("Hava Kalitesi: ISINMA - Kararli okuma bekleniyor"));
  } else {
    int co2 = (int)_readings.co2_ppm_filtered;
    if (co2 < 800) Serial.println(F("Hava Kalitesi: Cok Iyi"));
    else if (co2 < 1000) Serial.println(F("Hava Kalitesi: Iyi"));
    else if (co2 < 1500) Serial.println(F("Hava Kalitesi: Orta"));
    else Serial.println(F("Hava Kalitesi: Zayif - Havalandirma gerekli"));
  }
  Serial.println();
}

// Read soil moisture sensor
void Sensors::readSoilMoisture() {
  int raw_analog = analogRead(A0);
  float raw_percent = map(raw_analog, _soilDryValue, _soilWetValue, 0, 100);
  raw_percent = constrain(raw_percent, 0, 100);
  
  // Raw değerleri kaydet
  _readings.soil_moisture_raw = raw_analog;
  _readings.soil_moisture_raw_analog = raw_analog;
  _readings.soil_moisture_percent_raw = raw_percent;
  
  // Kalman filtresi uygula
  if (_kalmanEnabled) {
    _readings.soil_moisture_percent_filtered = _kalman_soil.update(raw_percent);
  } else {
    _readings.soil_moisture_percent_filtered = raw_percent;
  }
  
  Serial.println(F("--- MH Water Sensor (Soil Moisture) ---"));
  Serial.print(F("Raw Analog: ")); Serial.println(raw_analog);
  Serial.print(F("RAW: ")); 
  Serial.print(_readings.soil_moisture_percent_raw); 
  Serial.print(F(" % | FILTRELI: ")); 
  Serial.print(_readings.soil_moisture_percent_filtered); 
  Serial.println(F(" %"));
  
  float soil = _readings.soil_moisture_percent_filtered;
  if (soil < 30) Serial.println(F("Durum: Kuru - Sulama gerekli"));
  else if (soil < 60) Serial.println(F("Durum: Nemli - Yeterli"));
  else if (soil < 80) Serial.println(F("Durum: Islak - Iyi"));
  else Serial.println(F("Durum: Cok Islak - Asiri sulama riski!"));
  Serial.println();
}

// Read all sensors
void Sensors::readAllSensors() {
  readBH1750();
  readBME680();
  readMHZ14A();
  readSoilMoisture();
}

// Get MH-Z14A CO2 reading
int Sensors::getMHZ14ACO2() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte response[9];
  
  Serial1.write(cmd, 9);
  
  unsigned long timeout = millis();
  int i = 0;
  
  while ((millis() - timeout) < 1000) {
    if (Serial1.available() > 0) {
      response[i] = Serial1.read();
      i++;
      if (i >= 9) break;
    }
  }
  
  if (i != 9) return -1;
  
  byte checksum = 0;
  for (int j = 1; j < 8; j++) {
    checksum += response[j];
  }
  checksum = 0xFF - checksum;
  checksum += 1;
  
  if (response[8] != checksum) return -1;
  
  int co2Value = (int)response[2] * 256 + (int)response[3];
  _readings.co2_temperature = (int)response[4] - 40;
  
  return co2Value;
}

// Check if MH-Z14A is ready
bool Sensors::isMHZ14AReady() {
  return _mhz14aReady;
}

// Update MH-Z14A warmup status
void Sensors::updateMHZ14AWarmup() {
  if (!_mhz14aReady) {
    unsigned long elapsedTime = millis() - _mhz14aStartTime;
    if (elapsedTime >= MHZ14A_WARMUP_TIME) {
      _mhz14aReady = true;
      Serial.println(F("\n*** MH-Z14A isinma tamamlandi! ***\n"));
    }
  }
}

// Kalman filter control
void Sensors::enableKalmanFilter(bool enable) {
  _kalmanEnabled = enable;
  if (!enable) {
    // Kalman kapatıldığında raw değerleri kullan
    Serial.println(F("Kalman filtresi DEVRE DISI"));
  } else {
    Serial.println(F("Kalman filtresi AKTIF"));
  }
}

void Sensors::resetKalmanFilters() {
  _kalman_temperature.reset();
  _kalman_humidity.reset();
  _kalman_pressure.reset();
  _kalman_gas.reset();
  _kalman_lux.reset();
  _kalman_co2.reset();
  _kalman_soil.reset();
  Serial.println(F("Tum Kalman filtreleri sifirlandi"));
}

// Configuration
void Sensors::setSoilCalibration(int wetValue, int dryValue) {
  _soilWetValue = wetValue;
  _soilDryValue = dryValue;
}

void Sensors::setAltitude(float altitude) {
  _altitude = altitude;
}
