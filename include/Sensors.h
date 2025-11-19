#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <BH1750.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include "KalmanFilter.h"
#include "Calculations.h"

// Sensör okuma verilerini tutan struct
struct SensorReadings {
  // BH1750 - Işık (RAW ve FİLTRELİ)
  float lux_raw;
  float lux_filtered;
  float footCandles_raw;
  float footCandles_filtered;
  
  // BME680 - Hava kalitesi (RAW ve FİLTRELİ)
  float temperature_raw;
  float temperature_filtered;
  float humidity_raw;
  float humidity_filtered;
  float pressure_raw;
  float pressure_filtered;
  float gas_resistance_raw;
  float gas_resistance_filtered;
  
  // MH-Z14A - CO2 (RAW ve FİLTRELİ)
  int co2_ppm_raw;
  float co2_ppm_filtered;
  int co2_temperature;
  bool mhz14a_ready;
  
  // Toprak nem (RAW ve FİLTRELİ)
  int soil_moisture_raw;
  float soil_moisture_raw_analog;
  float soil_moisture_percent_raw;
  float soil_moisture_percent_filtered;
  
  // Hesaplanan değerler (filtrelenmiş değerlerden)
  float dew_point;
  float heat_index;
  float absolute_humidity;
  float vapor_pressure;
  float sea_level_pressure;
  float altitude;
  float co2_density;
  float recommended_ventilation;
};

class Sensors {
public:
  // Constructor
  Sensors(BH1750& light, Adafruit_BME680& bme);
  
  // Initialization
  void begin();
  void initSensors();
  
  // Sensor reading
  void readBH1750();
  void readBME680();
  void readMHZ14A();
  void readSoilMoisture();
  void readAllSensors();
  
  // MH-Z14A specific
  int getMHZ14ACO2();
  bool isMHZ14AReady();
  void updateMHZ14AWarmup();
  
  // Data access
  SensorReadings& getReadings() { return _readings; }
  
  // Kalman filter control
  void enableKalmanFilter(bool enable);
  bool isKalmanFilterEnabled() const { return _kalmanEnabled; }
  void resetKalmanFilters();
  
  // Configuration
  void setSoilCalibration(int wetValue, int dryValue);
  void setAltitude(float altitude);
  
private:
  BH1750& _light;
  Adafruit_BME680& _bme;
  SensorReadings _readings;
  
  // Kalman Filters (her sensör için ayrı)
  KalmanFilter _kalman_temperature;
  KalmanFilter _kalman_humidity;
  KalmanFilter _kalman_pressure;
  KalmanFilter _kalman_gas;
  KalmanFilter _kalman_lux;
  KalmanFilter _kalman_co2;
  KalmanFilter _kalman_soil;
  bool _kalmanEnabled;
  
  // MH-Z14A
  unsigned long _mhz14aStartTime;
  bool _mhz14aReady;
  static const unsigned long MHZ14A_WARMUP_TIME = 180000; // 3 dakika
  
  // Soil calibration
  int _soilWetValue;
  int _soilDryValue;
  
  // Altitude
  float _altitude;
  
  // Constants
  static constexpr float SEALEVELPRESSURE_HPA = 1013.25;
};

#endif // SENSORS_H
