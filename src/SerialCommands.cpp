#include "SerialCommands.h"

// External global degiskenler (main.cpp'den)
extern Servo mg995;
extern String serialCommand;
extern bool roofOpen;
extern bool fanOn;
extern bool lightOn;
extern bool pumpOn;
extern int currentServoPosition;
extern bool savedRoofOpen;
extern bool savedFanOn;
extern bool savedLightOn;
extern int savedServoPosition;

// External pin tanimlari (main.cpp'den)
#define SERVO_PIN 9
#define FAN_RELAY_PIN 30
#define LIGHT_RELAY_PIN 29
#define PUMP_RELAY_PIN 31

// Forward declaration
void executeCommand(String& cmd);

// ========================================
// SERIAL KOMUT ISLEME FONKSIYONU
// ========================================
void processSerialCommand() {
  // Serial'den (USB) veri gelirse oku
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Enter veya bosluk karakterinde komutu isle
    if (c == '\n' || c == '\r' || c == ' ') {
      if (serialCommand.length() > 0) {
        executeCommand(serialCommand);
      }
    } else if (c >= 32 && c <= 126) {
      // Sadece yazdırılabilir karakterleri ekle (ASCII 32-126)
      serialCommand += c;
    }
  }
  
  // Serial2'den (NodeMCU) veri gelirse oku
  while (Serial2.available() > 0) {
    char c = Serial2.read();
    
    // Enter veya bosluk karakterinde komutu isle
    if (c == '\n' || c == '\r' || c == ' ') {
      if (serialCommand.length() > 0) {
        Serial.print(F("[NodeMCU KOMUT] "));
        Serial.println(serialCommand);
        executeCommand(serialCommand);
      }
    } else if (c >= 32 && c <= 126) {
      // Sadece yazdırılabilir karakterleri ekle (ASCII 32-126)
      serialCommand += c;
    }
  }
}

// ========================================
// KOMUT YURUT FONKSIYONU (USB + NodeMCU icin ortak)
// ========================================
void executeCommand(String& cmd) {
  // Stringi trim et ve kucuk harfe cevir
  cmd.trim();
  cmd.toLowerCase();
  
  Serial.print(F("[DEBUG] Alinan komut: "));
  Serial.println(cmd);
  
  // Komut isleme - Sozel komutlar
  if (cmd == "havaac") {
    // Kapak AC + Fan AC
    Serial.println(F("\n>>> KOMUT: HAVA AC (Kapak + Fan) <<<"));
    if (currentServoPosition != 0) {
      mg995.attach(SERVO_PIN);    // Servo'yu aktif et
      mg995.write(0);              // Kapak acik (0 derece)
      delay(1000);                 // Servo hareket etsin (1 saniye)
      mg995.detach();              // PWM sinyalini kes
      currentServoPosition = 0;
      Serial.println(F("[OK] Kapak acildi (0 derece)"));
    } else {
      Serial.println(F("[INFO] Kapak zaten acik"));
    }
    digitalWrite(FAN_RELAY_PIN, LOW);  // Fan ac (LOW=acik)
    roofOpen = true;
    fanOn = true;
    Serial.println(F("[OK] Fan acildi"));
    cmd = "";  // Komutu temizle
    
  } else if (cmd == "havakapa") {
    // Kapak KAPAT + Fan KAPAT
    Serial.println(F("\n>>> KOMUT: HAVA KAPA (Kapak + Fan) <<<"));
    if (currentServoPosition != 95) {
      mg995.attach(SERVO_PIN);     // Servo'yu aktif et
      mg995.write(95);             // Kapak kapali (95 derece)
      delay(1000);                 // Servo hareket etsin (1 saniye)
      mg995.detach();              // PWM sinyalini kes
      currentServoPosition = 95;
      Serial.println(F("[OK] Kapak kapatildi (95 derece)"));
    } else {
      Serial.println(F("[INFO] Kapak zaten kapali"));
    }
    digitalWrite(FAN_RELAY_PIN, HIGH);  // Fan kapat (HIGH=kapali)
    roofOpen = false;
    fanOn = false;
    Serial.println(F("[OK] Fan kapatildi"));
    cmd = "";  // Komutu temizle
    
  } else if (cmd == "isikac") {
    // Isik AC
    Serial.println(F("\n>>> KOMUT: ISIK AC <<<"));
    digitalWrite(LIGHT_RELAY_PIN, LOW);  // Isik ac (LOW=acik)
    lightOn = true;
    Serial.println(F("[OK] Isik acildi"));
    cmd = "";  // Komutu temizle
    
  } else if (cmd == "isikkapa") {
    // Isik KAPAT
    Serial.println(F("\n>>> KOMUT: ISIK KAPA <<<"));
    digitalWrite(LIGHT_RELAY_PIN, HIGH);  // Isik kapat (HIGH=kapali)
    lightOn = false;
    Serial.println(F("[OK] Isik kapatildi"));
    cmd = "";  // Komutu temizle
    
  } else if (cmd == "sulaac") {
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
    cmd = "";  // Komutu temizle
    
  } else if (cmd == "sulakapa") {
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
    cmd = "";  // Komutu temizle
    
  } else {
    // Gecersiz komut - sadece uyari ver, hicbir sey yapma
    Serial.print(F("\n[UYARI] Bilinmeyen komut: '"));
    Serial.print(cmd);
    Serial.println(F("'"));
    Serial.println(F("Gecerli komutlar: havaac, havakapa, isikac, isikkapa, sulaac, sulakapa"));
    cmd = "";  // Komutu temizle
  }
}
