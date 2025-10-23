# Akıllı Sera Sistemi - Sistem Tasarımı

## 📋 Proje Özeti

Çoklu sensör entegrasyonu ile otomatik sera kontrol sistemi. Sıcaklık, nem, CO2, ışık ve toprak nemi verilerini kullanarak sera kapağı ve sulama sistemini akıllı bir şekilde kontrol eder.

---

## 🔧 Donanım Bileşenleri

### 1. Mikrocontroller
- **Arduino Mega 2560**
  - 54 dijital I/O pin
  - 16 analog giriş
  - 4 donanımsal UART
  - I2C desteği
  - 256KB Flash bellek

### 2. Sensörler

#### a) BH1750 (GY-30) - Işık Sensörü
- **İletişim:** I2C
- **Adres:** 0x23 veya 0x5C
- **Ölçüm Aralığı:** 1-65535 lux
- **Çözünürlük:** 1 lux
- **Pinler:**
  - SDA → D20
  - SCL → D21
  - VCC → 5V
  - GND → GND

#### b) BME680 - Çevre Sensörü
- **İletişim:** I2C
- **Adres:** 0x76 veya 0x77
- **Ölçümler:**
  - Sıcaklık: -40°C ~ +85°C (±1°C)
  - Nem: 0% ~ 100% (±3%)
  - Basınç: 300 ~ 1100 hPa (±1 hPa)
  - Gaz Direnci: 0 ~ 500 KOhm
- **Pinler:**
  - SDA → D20
  - SCL → D21
  - VCC → 3.3V veya 5V
  - GND → GND

#### c) MH-Z14A - CO2 Sensörü
- **İletişim:** UART (9600 baud)
- **Ölçüm Aralığı:** 0-5000 ppm
- **Doğruluk:** ±50 ppm + 5%
- **Isınma Süresi:** 3 dakika
- **Pinler:**
  - TX → D19 (RX1)
  - RX → D18 (TX1)
  - VCC → 5V (150mA)
  - GND → GND

#### d) MH Water Sensor - Toprak Nem Sensörü
- **İletişim:** Analog
- **Çıkış:** 0-1023 (ADC)
- **Ölçüm:** Kapasitif toprak nemi
- **Pinler:**
  - A0 → A0 (Analog)
  - VCC → 5V
  - GND → GND
- **Kalibrasyon:**
  - Kuru (Hava): 1023
  - Islak (Su): 300

### 3. Aktüatörler

#### a) Servo Motor (Sera Kapağı)
- **Model:** Standart servo (örn: SG90, MG996R)
- **Kontrol:** PWM
- **Açı:** 0-180°
- **Pin:** D9
- **Güç:** Harici 5V (servo tipine göre)

#### b) Röle Modülü (Sulama Pompası)
- **Tip:** 5V Röle
- **Kanal:** 1 kanal (genişletilebilir)
- **Kontrol:** Dijital pin
- **Pin:** D10
- **Yük:** Su pompası / Selenoid vana

---

## 🔌 Bağlantı Şeması

```
ARDUINO MEGA 2560
│
├─ I2C Bus (D20/SDA, D21/SCL)
│  ├─ BH1750 (0x23)
│  └─ BME680 (0x76)
│
├─ UART1 (D18/TX1, D19/RX1)
│  └─ MH-Z14A CO2 Sensor
│
├─ Analog Input
│  └─ A0 → MH Water Sensor
│
├─ PWM Output
│  └─ D9 → Servo Motor (Sera Kapağı)
│
└─ Digital Output
   └─ D10 → Röle (Sulama Pompası)
```

---

## 📊 Veri Akışı

```
┌─────────────────┐
│   SENSÖRLER     │
│                 │
│ • BH1750        │──┐
│ • BME680        │──┤
│ • MH-Z14A       │──┼──> Arduino Mega 2560
│ • Soil Sensor   │──┘
└─────────────────┘
                      │
                      ├──> Veri Okuma
                      │
                      ├──> Bilimsel Hesaplamalar
                      │    • Çiy Noktası
                      │    • Heat Index
                      │    • Mutlak Nem
                      │    • CO2 Konsantrasyonu
                      │
                      ├──> Karar Algoritması
                      │    • 9 Sera Kapak Kodu
                      │    • 8 Sulama Kodu
                      │
                      └──> Kontrol Sinyalleri
                           ├──> Servo Motor (Kapak)
                           └──> Röle (Sulama)
```

---

## 🧠 Yazılım Mimarisi

### 1. Ana Döngü (Loop)
```cpp
loop() {
  readAllSensors()      // Tüm sensörleri oku
  calculateValues()      // Bilimsel hesaplamalar
  controlGreenhouse()    // Sera kapak kontrolü
  controlIrrigation()    // Sulama kontrolü
  printData()           // Seri port çıktısı
  delay(2000)           // 2 saniye bekle
}
```

### 2. Kontrol Sistemi

#### a) Sera Kapak Kontrolü
- **Girdi:** Sıcaklık, Nem, CO2, Işık, Basınç
- **Çıktı:** Kapak pozisyonu (0-100%)
- **Frekans:** 2 saniye
- **Histerezis:** 30 saniye (titreme önleme)

#### b) Sulama Kontrolü
- **Girdi:** Toprak Nemi, Sıcaklık, Hava Nemi, Işık
- **Çıktı:** Pompa AÇIK/KAPALI
- **Frekans:** 2 saniye
- **Minimum Bekleme:** 10 dakika

---

## 📈 Bilimsel Hesaplamalar

### 1. Çiy Noktası (Dew Point)
**Formül:** Magnus-Tetens
```
Td = (b × α) / (a - α)
α = (a×T)/(b+T) + ln(RH/100)
```
**Kullanım:** Küf riski tespiti

### 2. Hissedilen Sıcaklık (Heat Index)
**Formül:** Rothfusz (NOAA)
```
HI = -42.379 + 2.049T + 10.143RH - 0.225T×RH + ...
```
**Kullanım:** Bitki stres tespiti

### 3. Mutlak Nem (Absolute Humidity)
**Formül:** Termodinamik
```
AH = (e × 2.1674) / (T + 273.15)
```
**Kullanım:** Buharlaşma hesabı

### 4. CO2 Konsantrasyonu
**Formül:** İdeal Gaz Yasası
```
C(mg/m³) = (ppm × M × P) / (R × T)
```
**Kullanım:** Havalandırma hesabı

### 5. Deniz Seviyesi Basıncı
**Formül:** Barometrik
```
P0 = P × exp((g × M × h) / (R × T))
```
**Kullanım:** Hava durumu tahmini

---

## 🎯 Kontrol Algoritmaları

### Sera Kapak Kontrol Kodları

| Kod | Öncelik | Koşul | Kapak | Açıklama |
|-----|---------|-------|-------|----------|
| KOD-7 | 1 | Sıcaklık < 10°C | 0% | Donma riski |
| KOD-1 | 2 | Sıcaklık > 32°C + Nem > 70% | 100% | Aşırı sıcak |
| KOD-8 | 3 | Basınç < 985 hPa | 0% | Fırtına |
| KOD-2 | 4 | Sıcaklık > 28°C + CO2 > 800 | 75% | Yüksek sıcaklık |
| KOD-3 | 5 | CO2 > 1500 ppm | 50% | Yüksek CO2 |
| KOD-4 | 6 | Nem > 85% | 40% | Küf riski |
| KOD-6 | 7 | Gece + Sıcaklık < 18°C | 0% | Gece koruma |
| KOD-5 | 8 | Gündüz + Normal sıcaklık | 25% | Havalandırma |
| KOD-9 | 9 | İdeal koşullar | 0% | Stabil sistem |

### Sulama Kontrol Kodları

| Kod | Öncelik | Koşul | Pompa | Süre | Açıklama |
|-----|---------|-------|-------|------|----------|
| SULAMA-5 | 1 | Toprak > 90% | KAPAT | 24h | Aşırı sulama |
| SULAMA-7 | 2 | Gece + Soğuk | KAPAT | - | Gece yasağı |
| SULAMA-4 | 3 | Yağmur | KAPAT | 30dk | Doğal yağış |
| SULAMA-6 | 4 | Nem yüksek | KAPAT | - | Küf riski |
| SULAMA-1 | 5 | Toprak < 20% + Sıcak | AÇIK | 30s | Acil |
| SULAMA-3 | 6 | Akşam + Kuru | AÇIK | 25s | Optimal |
| SULAMA-2 | 7 | Gündüz + Kuru | AÇIK | 20s | Normal |
| SULAMA-8 | 8 | Toprak 50-70% | KAPAT | - | İdeal |

---

## 📡 Seri İletişim

### Çıktı Formatı
```
--- New Reading ---

--- GY-30 (BH1750) Light Sensor ---
Light Level: 475.00 lux
Light Level: 44.13 fc
  -> Bright

--- BME680 Air Quality Sensor ---
Temperature: 25.16 C
Pressure: 994.75 hPa
Humidity: 59.38 %
Gas Resistance: 165.22 KOhm
Dew Point: 16.67 C
Absolute Humidity: 8.14 g/m3
Heat Index: 25.09 C
...

--- MH Water Sensor ---
Soil Moisture: 45.5 %
  -> Dry - Irrigation recommended

>>> GREENHOUSE ROOF CONTROL <<<
Position: 25% - CODE-5: Normal ventilation
>>> END ROOF CONTROL <<<

>>> IRRIGATION CONTROL <<<
Action: PUMP ON (20 seconds)
Reason: SULAMA-2: Normal irrigation
>>> END IRRIGATION CONTROL <<<

Uptime: 334 seconds
```

---

## 🔋 Güç Tüketimi

| Bileşen | Akım | Güç |
|---------|------|-----|
| Arduino Mega | ~50mA | 0.25W |
| BH1750 | ~0.2mA | 0.001W |
| BME680 | ~3.7mA | 0.018W |
| MH-Z14A | ~150mA | 0.75W |
| Soil Sensor | ~20mA | 0.1W |
| Servo (SG90) | ~100-500mA | 0.5-2.5W |
| Röle + Pompa | ~50mA + Pompa | 0.25W + Pompa |
| **TOPLAM** | **~400mA** | **~2-4W** |

*Not: Pompa gücü modele göre değişir (genelde 5-12W)*

**Önerilen Güç Kaynağı:** 5V 3A adaptör

---

## 📁 Dosya Yapısı

```
Tarhun Bitirme Projesi/
│
├── platformio.ini          # PlatformIO konfigürasyonu
├── README.md              # Proje açıklaması
├── kosullar.md            # Kontrol koşulları
├── sistem_tasarimi.md     # Bu dosya
│
├── src/
│   └── main.cpp           # Ana program kodu
│
├── include/
│   └── README             # Header dosyaları
│
└── lib/
    └── README             # Kütüphaneler
```

---

## 📚 Kullanılan Kütüphaneler

```ini
lib_deps = 
    claws/BH1750@^1.3.0
    adafruit/Adafruit BME680 Library@^2.0.4
    adafruit/Adafruit Unified Sensor@^1.1.14
```

---

## 🚀 Kurulum ve Kullanım

### 1. Donanım Montajı
1. Tüm sensörleri Arduino'ya bağlayın
2. Servo motoru D9'a bağlayın (harici güç)
3. Röle modülünü D10'a bağlayın
4. Güç kaynağını bağlayın

### 2. Yazılım Yükleme
```bash
# PlatformIO ile
pio run --target upload

# Arduino IDE ile
# main.cpp dosyasını .ino uzantılı olarak kaydet ve yükle
```

### 3. İlk Çalıştırma
1. Seri monitörü açın (115200 baud)
2. MH-Z14A sensörünün 3 dakika ısınmasını bekleyin
3. Sensör değerlerini gözlemleyin
4. Sistem otomatik kontrole başlayacak

### 4. Kalibrasyon
- **Toprak Nem Sensörü:**
  - Kuru değer: Sensörü havada tutun, değeri kaydedin
  - Islak değer: Sensörü suya batırın, değeri kaydedin
  - `main.cpp` içinde `SOIL_DRY_VALUE` ve `SOIL_WET_VALUE` güncelleyin

---

## 🔍 Test ve Doğrulama

### Sensör Testleri
1. **BH1750:** El feneri ile ışık değişimi gözleyin
2. **BME680:** Çakmak ile sıcaklık/gaz direnci test edin
3. **MH-Z14A:** Nefes vererek CO2 artışını test edin
4. **Soil Sensor:** Kuru/ıslak toprakta test edin

### Kontrol Testleri
1. **Sera Kapak:** Sıcaklık değişimlerinde kapak hareketini gözleyin
2. **Sulama:** Toprak nem seviyesini değiştirerek pompayı test edin

---

## 🛡️ Güvenlik Özellikleri

1. **Histerezis:** 30 saniye minimum hareket aralığı (titreme önleme)
2. **Timeout:** MH-Z14A 3 dakika ısınma süresi
3. **Sınır Kontrolü:** Tüm değerler min/max kontrollü
4. **Aşırı Sulama Koruması:** 90% üstü nemde sulama kilidi
5. **Donma Koruması:** 10°C altında kapak otomatik kapanır

---

## 📊 Performans Metrikleri

- **Veri Okuma Frekansı:** 2 saniye
- **Karar Alma Süresi:** <100ms
- **Servo Yanıt Süresi:** ~500ms
- **Röle Yanıt Süresi:** <50ms
- **Sensör Doğruluğu:**
  - Sıcaklık: ±1°C
  - Nem: ±3%
  - CO2: ±50ppm
  - Işık: ±20%
  - Toprak Nem: ±5%

---

## 🔮 Gelecek Geliştirmeler

1. **WiFi/Bluetooth Modülü** - Uzaktan izleme
2. **SD Kart** - Veri kaydetme
3. **LCD Ekran** - Yerel veri görüntüleme
4. **Mobil Uygulama** - Akıllı telefon kontrolü
5. **Yapay Zeka** - Makine öğrenmesi ile optimizasyon
6. **Güneş Paneli** - Enerji bağımsızlığı
7. **Çoklu Bölge** - Farklı bitki türleri için bölgesel kontrol

---

## 📞 Destek ve Katkı

**Geliştirici:** Yusuf Islam Budak  
**Proje:** Bitirme Tezi - Akıllı Tarım Sistemi  
**GitHub:** https://github.com/YusufIslamBudak/Bitirme-Projesi-Ak-ll-Tar-m-  
**Tarih:** Ekim 2025

---

## 📄 Lisans

Bu proje bir bitirme tezi çalışmasıdır.

---

**Son Güncelleme:** 23 Ekim 2025
