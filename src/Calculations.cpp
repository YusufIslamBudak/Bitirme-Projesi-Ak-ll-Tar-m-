#include "Calculations.h"
#include <math.h>

// Çiy noktası hesabı (Magnus formülü)
float Calculations::calculateDewPoint(float temp, float humidity) {
  float a = 17.27;
  float b = 237.7;
  float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0);
  return (b * alpha) / (a - alpha);
}

// Mutlak nem hesabı (g/m³)
float Calculations::calculateAbsoluteHumidity(float temp, float humidity) {
  // Formül: AH = (6.112 × e^((17.67 × T)/(T+243.5)) × RH × 2.1674) / (273.15 + T)
  return (6.112 * exp((17.67 * temp) / (temp + 243.5)) * humidity * 2.1674) / (273.15 + temp);
}

// Hissedilen sıcaklık (Heat Index)
float Calculations::calculateHeatIndex(float temp, float humidity) {
  // 27°C altında hissedilen sıcaklık = gerçek sıcaklık
  if (temp < 27.0) {
    return temp;
  }
  
  // Rothfusz regresyon denklemi (NOAA)
  float c1 = -8.78469475556;
  float c2 = 1.61139411;
  float c3 = 2.33854883889;
  float c4 = -0.14611605;
  float c5 = -0.012308094;
  float c6 = -0.0164248277778;
  float c7 = 0.002211732;
  float c8 = 0.00072546;
  float c9 = -0.000003582;
  
  float T = temp;
  float RH = humidity;
  
  return c1 + c2*T + c3*RH + c4*T*RH + c5*T*T + c6*RH*RH + c7*T*T*RH + c8*T*RH*RH + c9*T*T*RH*RH;
}

// Buhar basıncı hesabı (kPa)
float Calculations::calculateVaporPressure(float temp, float humidity) {
  // Doymuş buhar basıncı (kPa)
  float es = 0.6108 * exp((17.27 * temp) / (temp + 237.3));
  // Gerçek buhar basıncı
  return es * (humidity / 100.0);
}

// Deniz seviyesi basıncı (hPa)
float Calculations::calculateSeaLevelPressure(float pressure, float altitude, float temp) {
  // Uluslararası standart atmosfer formülü
  // P0 = P / (1 - h/44330)^5.255
  return pressure / pow(1.0 - (altitude / 44330.0), 5.255);
}

// Lux → Foot-Candles dönüşümü
float Calculations::luxToFootCandles(float lux) {
  // 1 foot-candle = 10.764 lux
  return lux * 0.0929;
}

// CO2 ppm → mg/m³ dönüşümü
float Calculations::co2PpmToMgPerM3(int ppm, float temp, float pressure) {
  // İdeal gaz yasası: ρ = (ppm × M × P) / (R × T × 1000)
  // M = CO2 molar kütlesi (44.01 g/mol)
  // R = Gaz sabiti (8.314 J/(mol·K))
  // P = Basınç (Pa)
  // T = Sıcaklık (K)
  
  float molarMass = 44.01;  // g/mol
  float R = 8.314;          // J/(mol·K)
  float tempK = temp + 273.15;
  float pressurePa = pressure * 100.0;  // hPa → Pa
  
  return (ppm * molarMass * pressurePa) / (R * tempK * 1000.0);
}

// Önerilen havalandırma oranı (L/s kişi başına)
float Calculations::calculateVentilationRate(int co2_ppm) {
  // ASHRAE 62.1 standardına göre basitleştirilmiş hesaplama
  // Yaklaşık formül: VentRate = CO2_ppm / 77.8
  return co2_ppm / 77.8;
}
