#include "IrrigationControl.h"
#include "Sensors.h"

// External global degiskenler
extern Sensors sensors;
extern bool isPumpOn;
extern unsigned long pumpStartTime;
extern unsigned long lastIrrigationCheck;
extern unsigned long irrigationLockoutUntil;
extern int irrigationDuration;
extern String lastIrrigationReason;
extern int currentRoofPosition;

// External constants
#define IRRIGATION_CHECK_INTERVAL 10000
#define IRRIGATION_LOCKOUT_TIME 600000
#define PUMP_RELAY_PIN 31

// External fonksiyonlar
extern void setRoofPosition(int position, String reason);

// ========================================
// SULAMA KONTROL FONKSIYONLARI - OTOMATIK YONETIM DEVRE DISI
// ========================================

// NOT: Bu fonksiyonlar artik kullanilmiyor.
// Sulama sadece UART komutlari ile manuel olarak kontrol ediliyor.

// Sulama sistemini kontrol et
/*
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
*/

// Sulama gereksinimi kontrolu
/*
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
*/

// Pompa durumunu ayarla
/*
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
*/
