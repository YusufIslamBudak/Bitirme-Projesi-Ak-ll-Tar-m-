#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <Arduino.h>

/**
 * @brief Bilimsel hesaplamalar için yardımcı sınıf
 * 
 * Tüm meteorolojik ve çevresel hesaplamaları içerir.
 * Statik metodlar kullanır, nesne oluşturmaya gerek yoktur.
 */
class Calculations {
public:
  /**
   * @brief Çiy noktası hesaplar (Magnus formülü)
   * @param temp Sıcaklık (°C)
   * @param humidity Bağıl nem (%)
   * @return Çiy noktası (°C)
   */
  static float calculateDewPoint(float temp, float humidity);
  
  /**
   * @brief Mutlak nem hesaplar (havadaki su buharı yoğunluğu)
   * @param temp Sıcaklık (°C)
   * @param humidity Bağıl nem (%)
   * @return Mutlak nem (g/m³)
   */
  static float calculateAbsoluteHumidity(float temp, float humidity);
  
  /**
   * @brief Hissedilen sıcaklık hesaplar (Heat Index)
   * @param temp Sıcaklık (°C)
   * @param humidity Bağıl nem (%)
   * @return Hissedilen sıcaklık (°C)
   */
  static float calculateHeatIndex(float temp, float humidity);
  
  /**
   * @brief Buhar basıncı hesaplar
   * @param temp Sıcaklık (°C)
   * @param humidity Bağıl nem (%)
   * @return Buhar basıncı (kPa)
   */
  static float calculateVaporPressure(float temp, float humidity);
  
  /**
   * @brief Deniz seviyesi basıncını hesaplar
   * @param pressure Yerel basınç (hPa)
   * @param altitude Rakım (m)
   * @param temp Sıcaklık (°C)
   * @return Deniz seviyesi basıncı (hPa)
   */
  static float calculateSeaLevelPressure(float pressure, float altitude, float temp);
  
  /**
   * @brief Lux değerini foot-candles'a çevirir
   * @param lux Işık şiddeti (lux)
   * @return Işık şiddeti (foot-candles)
   */
  static float luxToFootCandles(float lux);
  
  /**
   * @brief CO2 ppm değerini mg/m³'e çevirir
   * @param ppm CO2 konsantrasyonu (ppm)
   * @param temp Sıcaklık (°C)
   * @param pressure Basınç (hPa)
   * @return CO2 yoğunluğu (mg/m³)
   */
  static float co2PpmToMgPerM3(int ppm, float temp, float pressure);
  
  /**
   * @brief Önerilen havalandırma oranını hesaplar
   * @param co2_ppm CO2 konsantrasyonu (ppm)
   * @return Havalandırma oranı (L/s kişi başına)
   */
  static float calculateVentilationRate(int co2_ppm);
};

#endif // CALCULATIONS_H
