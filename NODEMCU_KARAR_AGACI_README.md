# NodeMCU Akıllı Sera Kontrol Sistemi - Karar Ağacı

## 🎯 Genel Bakış

NodeMCU artık Arduino Mega'dan aldığı sensör verilerini kullanarak **otomatik karar ağacı** algoritması ile serayı yönetir. Sistem, beyaz tahtada belirtilen kurallara ve 2209-A projesi tarımsal verilerine göre tasarlanmıştır.

## 📊 Karar Ağacı Yapısı

### Öncelik Sıralaması (En Yüksek → En Düşük)

```
KRİTİK ACİL DURUMLAR (En Yüksek Öncelik)
├── KOD-7: DONMA RİSKİ (Sıcaklık < 10°C veya Çiy Noktası < 5°C)
└── KOD-8: FIRTINA RİSKİ (Basınç < 985 hPa)

YÜKSEK ÖNCELİKLİ DURUMLAR
├── KOD-1: AŞIRI SICAK+NEM (Sıc > 32°C ve Nem > 70%)
├── KOD-2: YÜKSEK SICAK+CO2 (Sıc > 28°C ve CO2 > 800 ppm)
├── KOD-3: YÜKSEK CO2 (CO2 > 1500 ppm)
└── KOD-4: YÜKSEK NEM - Küf Riski (Nem > 85%)

NORMAL OPERASYON KONTROLLARI
├── KOD-6: GECE MODU - Soğuk Koruma (Işık < 50 lux ve Sıc < 18°C)
├── KOD-5: GÜNDÜZ HAVALANDIRMASı (Işık > 10000 lux)
└── KOD-9: OPTIMAL KOŞULLAR (Tüm değerler ideal aralıkta)

SULAMA KONTROL SİSTEMİ
├── SULAMA-1: ACİL SULAMA (Toprak < 20% ve Sıc > 28°C)
├── SULAMA-2: NORMAL SULAMA (Toprak < 40% ve Sıc > 20°C)
├── SULAMA-3: AKŞAM SULAMASI (Toprak < 50% ve Işık < 1000 lux)
├── SULAMA-4: YAĞMUR İPTALİ (Basınç < 990 hPa ve Nem > 85%)
├── SULAMA-5: AŞIRI SULAMA KORUMASI (Toprak > 90%)
└── SULAMA-6: KÜF RİSKİ - Sulama Durdur
```

## 🔧 Teknik Özellikler

### Sensör Değerleri (JSON Parsing)

```cpp
SensorData {
  float temperature;      // Sıcaklık (°C)
  float humidity;         // Nem (%)
  float pressure;         // Basınç (hPa)
  float lux;              // Işık şiddeti (lux)
  int co2;                // CO2 (ppm)
  float soilMoisture;     // Toprak nem (%)
  float dewPoint;         // Çiy noktası (°C)
  float heatIndex;        // Hissedilen sıcaklık (°C)
}
```

### Karar Aralıkları

| Parametre | Kritik Düşük | Düşük | Normal | Yüksek | Kritik Yüksek |
|-----------|--------------|-------|--------|--------|---------------|
| **Sıcaklık** | < 10°C | 10-18°C | 18-30°C | 30-32°C | > 32°C |
| **Nem** | < 30% | 30-50% | 50-70% | 70-85% | > 85% |
| **CO2** | < 400 ppm | 400-600 ppm | 600-1000 ppm | 1000-1500 ppm | > 1500 ppm |
| **Toprak Nem** | < 20% | 20-40% | 40-70% | 70-90% | > 90% |
| **Işık** | < 50 lux | 50-1000 lux | 1000-10000 lux | 10000-20000 lux | > 20000 lux |
| **Basınç** | < 985 hPa | 985-990 hPa | 990-1020 hPa | 1020-1030 hPa | > 1030 hPa |

## 🎮 Kontrol Mantığı

### 1. KOD-7: DONMA RİSKİ ❄️
```
KOŞUL: Sıcaklık < 10°C VEYA Çiy Noktası < 5°C
EYLEM:
  ✓ Kapak + Fan KAPAT (ısı kaybını önle)
  ✓ Sulama KAPAT (donma riski)
ÖNCELİK: KRİTİK - Diğer tüm kontrolleri iptal et
```

### 2. KOD-8: FIRTINA RİSKİ 🌪️
```
KOŞUL: Basınç < 985 hPa
EYLEM:
  ✓ Kapak + Fan KAPAT (rüzgar hasarı önleme)
  ✓ Sulama KAPAT (güvenlik)
ÖNCELİK: KRİTİK
```

### 3. KOD-1: AŞIRI SICAK+NEM 🔥💧
```
KOŞUL: Sıcaklık > 32°C VE Nem > 70%
EYLEM:
  ✓ Kapak + Fan AÇ (maksimum havalandırma)
  ✓ Sulama KAPAT (buharlaşma fazla)
ÖNCELİK: YÜKSEK
```

### 4. KOD-2: YÜKSEK SICAK+CO2 🌡️
```
KOŞUL: Sıcaklık > 28°C VE CO2 > 800 ppm
EYLEM:
  ✓ Kapak + Fan AÇ (havalandırma)
ÖNCELİK: YÜKSEK
```

### 5. KOD-3: YÜKSEK CO2 💨
```
KOŞUL: CO2 > 1500 ppm VE Sıcaklık > 20°C
EYLEM:
  ✓ Kapak + Fan AÇ (hava değişimi)
ÖNCELİK: ORTA
```

### 6. KOD-4: KÜF RİSKİ 🍄
```
KOŞUL: Nem > 85% VE Sıcaklık < 25°C VE (Sıc - ÇiyNoktası) < 3°C
EYLEM:
  ✓ Kapak + Fan AÇ (nem azaltma)
ÖNCELİK: ORTA
```

### 7. KOD-6: GECE MODU 🌙
```
KOŞUL: Işık < 50 lux VE Sıcaklık < 18°C
EYLEM:
  ✓ Kapak + Fan KAPAT (ısı koruma)
  ✓ Işık AÇ (fotosent desteği)
ÖNCELİK: ORTA
```

### 8. KOD-5: GÜNDÜZ HAVALANDIRMASı ☀️
```
KOŞUL: Işık > 10000 lux VE 22°C < Sıc < 28°C VE CO2 < 1000 ppm
EYLEM:
  ✓ Kapak + Fan AÇ (doğal havalandırma)
  ✓ Işık KAPAT (güneş yeterli)
ÖNCELİK: DÜŞÜK
```

### 9. KOD-9: OPTIMAL KOŞULLAR ✅
```
KOŞUL: 
  - 20°C ≤ Sıcaklık ≤ 26°C
  - 50% ≤ Nem ≤ 70%
  - 400 ppm ≤ CO2 ≤ 1000 ppm
  - 50% ≤ Toprak Nem ≤ 70%
EYLEM:
  ✓ Enerji tasarrufu (gereksiz sistemleri kapat)
  ✓ Sistem stabil - Minimal müdahale
ÖNCELİK: DÜŞÜK
```

## 💧 Sulama Kontrol Sistemi

### SULAMA-1: ACİL SULAMA 🚨
```
KOŞUL: Toprak Nem < 20% VE Sıcaklık > 28°C
EYLEM: Sulama AÇ (30 saniye)
ÖNCELİK: KRİTİK
```

### SULAMA-2: NORMAL SULAMA 💦
```
KOŞUL: Toprak Nem < 40% VE Sıcaklık > 20°C VE Işık > 1000 lux
EYLEM: Sulama AÇ (20 saniye)
ÖNCELİK: ORTA
```

### SULAMA-3: AKŞAM SULAMASI (Optimal) 🌅
```
KOŞUL: Toprak Nem < 50% VE Işık < 1000 lux VE Sıcaklık > 15°C
EYLEM: Sulama AÇ (25 saniye)
ÖNCELİK: DÜŞÜK
NOT: Buharlaşma minimum, en ideal sulama zamanı
```

### SULAMA-4: YAĞMUR İPTALİ ☔
```
KOŞUL: Basınç < 990 hPa VE Nem > 85% VE Pompa AÇIK
EYLEM: Sulama KAPAT
ÖNCELİK: ORTA
SEBEP: Doğal yağış bekleniyor
```

### SULAMA-5: AŞIRI SULAMA KORUMASI ⚠️
```
KOŞUL: Toprak Nem > 90%
EYLEM: 
  ✓ Sulama KAPAT
  ✓ Kapak + Fan AÇ (kurutma)
ÖNCELİK: YÜKSEK
SEBEP: Kök çürümesi riski
```

### SULAMA-6: KÜF RİSKİ 🍄
```
KOŞUL: Toprak Nem > 80% VE Nem > 85% VE Sıcaklık < 22°C
EYLEM:
  ✓ Sulama KAPAT
  ✓ Kapak + Fan AÇ (havalandırma)
ÖNCELİK: ORTA
```

## ⚙️ Teknik Detaylar

### Karar Aralığı
- **Karar Frekansı**: 10 saniyede bir
- **Komut Cooldown**: 30 saniye (aynı komut tekrar önleme)
- **JSON Parsing**: Manuel string işleme (ArduinoJson kullanmadan)

### Komut Güvenliği
```cpp
void sendCommandSafe(String command, String& lastCmd, unsigned long& lastTime) {
  // Aynı komut 30 saniye içinde tekrar gönderilmez
  if (command == lastCmd && (millis() - lastTime) < 30000) {
    return; // Komut atla
  }
  sendCommandToArduino(command);
}
```

### Web Kontrol Paneli
- **Ana Sayfa**: `http://<NodeMCU-IP>/`
- **Komut Gönder**: `http://<NodeMCU-IP>/command?cmd=havaac`
- **Durum Sorgula**: `http://<NodeMCU-IP>/status`
- **Otomatik Kontrol**: Web arayüzünden aç/kapa butonu

### Otomatik Kontrol
```cpp
bool autoControlEnabled = true;  // Web arayüzünden değiştirilebilir

// Loop içinde
if (autoControlEnabled && (millis() - lastDecisionTime >= 10000)) {
  makeDecision();  // Karar ağacını çalıştır
}
```

## 📝 Kullanım

### 1. Otomatik Mod (Varsayılan)
- NodeMCU sensör verilerini okur
- Her 10 saniyede karar ağacını çalıştırır
- Gerekli komutları Arduino'ya gönderir

### 2. Manuel Mod
- Web arayüzünden "Otomatik Kontrol" butonuna tıkla
- Manuel komutlar gönderilebilir
- Karar ağacı devre dışı kalır

### 3. Hibrit Mod
- Otomatik kontrol açıkken manuel komut gönderme
- Acil müdahale için kullanılabilir
- 30 saniye sonra otomatik kontrol devam eder

## 🔍 Örnek Senaryo

### Senaryo: Sıcak Yaz Günü
```
SENSÖR VERİLERİ:
- Sıcaklık: 33°C
- Nem: 75%
- CO2: 950 ppm
- Toprak: 35%
- Işık: 65000 lux
- Basınç: 1013 hPa

KARAR AĞACI ÇIKTISI:
✓ KOD-1 TETİKLENDİ: AŞIRI SICAK+NEM
  → Komut: "havaac" (Kapak + Fan açıldı)
✓ SULAMA-2 TETİKLENDİ: NORMAL SULAMA
  → Komut: "sulaac" (Sulama başladı)

SONUÇ:
- Sera maksimum havalandırıldı
- Toprak nemlendirildi
- 20 saniye sonra sulama otomatik kapandı
```

## 🎯 Gelişmiş Özellikler

### 1. Çoklu Koşul Kontrolü
Her karar noktası birden fazla sensör verisini değerlendirir

### 2. Öncelik Sıralaması
Kritik durumlar diğer tüm kontrolleri iptal eder

### 3. Enerji Tasarrufu
Optimal koşullarda gereksiz sistemler kapatılır

### 4. Tekrar Önleme
Aynı komut 30 saniye içinde tekrar gönderilmez

### 5. SD Kart Loglama
Tüm kararlar ve sensör verileri zaman damgalı kaydedilir

## 🛠️ Gerekli Kütüphaneler

```cpp
// ESP8266 için
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// SD Kart için
#include <SPI.h>
#include <SD.h>

// Diğer
#include <SoftwareSerial.h>
#include <time.h>
```

## 📊 Performans

- **RAM Kullanımı**: ~15KB
- **Flash Kullanımı**: ~350KB
- **Karar Süresi**: <100ms
- **WiFi Latency**: ~50ms
- **SD Yazma**: ~200ms

## 🎓 2209-A Proje Uyumluluğu

Bu sistem, TÜBİTAK 2209-A Üniversite Öğrencileri Araştırma Projeleri Destekleme Programı kapsamında geliştirilmiş tarımsal verilere uygun olarak tasarlanmıştır.

### Referans Değerler:
- **Sıcaklık Aralığı**: 18-30°C (Gece: 12-18°C)
- **Nem Aralığı**: 50-80% (Sabah: 60-70%)
- **CO2 Optimal**: 400-1000 ppm (Min: 600 ppm)
- **Toprak Nem**: 40-60% (Optimal)
- **Basınç Min**: 1000 hPa (Fırtına: <990 hPa)

## 📞 Destek

Sorularınız için: [GitHub Issues](https://github.com/YusufIslamBudak/Bitirme-Projesi-Ak-ll-Tar-m-)

---

**Geliştirici**: Yusuf İslam Budak  
**Proje**: Akıllı Tarım - Sera Otomasyon Sistemi  
**Tarih**: Kasım 2025
