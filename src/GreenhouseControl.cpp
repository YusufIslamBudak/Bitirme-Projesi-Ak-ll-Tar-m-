#include "GreenhouseControl.h"
#include <BH1750.h>
#include <Adafruit_BME680.h>

// External global degiskenler
extern BH1750 lightMeter;
extern Adafruit_BME680 bme;
extern int currentRoofPosition;
extern unsigned long lastRoofAction;
extern String lastRoofReason;
extern int co2ppm;
extern bool mhz14aReady;

// External constants
#define ROOF_ACTION_DELAY 30000

// External fonksiyonlar
extern float calculateHeatIndex(float temp, float humidity);
extern float calculateDewPoint(float temp, float humidity);

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
