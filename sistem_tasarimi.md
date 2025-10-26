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

### 3. Kablosuz İletişim

#### LoRa E32 Modülü (Verici)
- **Model:** E32-TTL-100
- **İletişim:** UART (Software Serial)
- **Frekans:** 433 MHz (veya 868/915 MHz)
- **Menzil:** 3 km (açık alan)
- **Güç:** 100 mW
- **Pinler (Verici):**
  - RX → D10 (Software Serial)
  - TX → D11 (Software Serial)
  - M0 → D6
  - M1 → D7
  - VCC → 5V
  - GND → GND
- **Özellikler:**
  - Binary paket transferi
  - CRC hata kontrolü
  - Otomatik yeniden gönderim
  - Düşük güç tüketimi

#### LoRa E32 Modülü (Alıcı - Yer İstasyonu)
- **Bağımsız Arduino sistemi**
- **Aynı pin konfigürasyonu**
- **Serial Monitor çıktısı (9600 baud)**

### 4. Aktüatörler

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
ARDUINO MEGA 2560 (VERİCİ SİSTEMİ)
│
├─ I2C Bus (D20/SDA, D21/SCL)
│  ├─ BH1750 (0x23)
│  └─ BME680 (0x76)
│
├─ UART1 (D18/TX1, D19/RX1)
│  └─ MH-Z14A CO2 Sensor
│
├─ Software Serial (D10/RX, D11/TX)
│  └─ LoRa E32 Modülü (Verici)
│     - M0 → D6
│     - M1 → D7
│
├─ Analog Input
│  └─ A0 → MH Water Sensor
│
├─ PWM Output
│  └─ D9 → Servo Motor (Sera Kapağı)
│
└─ Digital Output
   └─ D10 → Röle (Sulama Pompası)

         ↓↓↓ LoRa 433MHz Kablosuz ↓↓↓
         
ARDUINO (ALICI - YER İSTASYONU)
│
└─ Software Serial (D10/RX, D11/TX)
   └─ LoRa E32 Modülü (Alıcı)
      - M0 → D6
      - M1 → D7
      - Serial Monitor → USB (9600 baud)
```

---

## 📊 Veri Akışı

```
┌─────────────────┐
│   SENSÖRLER     │
│  (VERİCİ SİSTEM)│
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
                      ├──> Kontrol Sinyalleri
                      │    ├──> Servo Motor (Kapak)
                      │    └──> Röle (Sulama)
                      │
                      └──> LoRa Veri Paketi (72 byte)
                           │
                           ├─ BME680: Sıcaklık, Nem, Basınç, Gaz
                           ├─ BH1750: Işık (lux)
                           ├─ MH-Z14A: CO2, Sensör Sıcaklık
                           ├─ Soil: Nem %, Ham değer
                           ├─ Kontrol: Kapak %, Pompa, Süre
                           ├─ Hesaplanan: Çiy, Heat Index, Abs. Nem
                           ├─ Sistem: Uptime, Sensör durumu
                           └─ CRC: Veri doğrulama
                           
                           ↓↓↓ 433 MHz Kablosuz ↓↓↓
                           
┌─────────────────────────────────────────┐
│   YER İSTASYONU (ALICI SİSTEM)         │
│                                         │
│ Arduino + LoRa E32 Alıcı               │
│   ↓                                     │
│ CRC Doğrulama                          │
│   ↓                                     │
│ Veri Çözümleme                         │
│   ↓                                     │
│ Serial Monitor (9600 baud)             │
│   • Sistem Bilgileri                   │
│   • Tüm Sensör Verileri                │
│   • Hesaplanan Değerler                │
│   • Kontrol Durumları                  │
│   • Sera Sağlık Skoru (0-100)          │
│   • Akıllı Uyarılar                    │
│   • İletişim İstatistikleri            │
└─────────────────────────────────────────┘
```

---

## 🧠 Yazılım Mimarisi

### 1. Ana Döngü (Loop)
```cpp
loop() {
  readAllSensors()       // Tüm sensörleri oku
  calculateValues()      // Bilimsel hesaplamalar
  controlGreenhouse()    // Sera kapak kontrolü
  controlIrrigation()    // Sulama kontrolü
  sendLoRaData()        // LoRa ile veri gönder
  printData()           // Seri port çıktısı (lokal)
  delay(5000)           // 5 saniye bekle
}
```

### 2. LoRa Veri Paketi Yapısı
```cpp
#pragma pack(push,1)
struct SensorDataPacket {
  // BME680 (16 byte)
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
  
  // BH1750 (4 byte)
  float lux;
  
  // MH-Z14A (3 byte)
  uint16_t co2_ppm;
  int8_t co2_temperature;
  
  // Toprak Nem (6 byte)
  float soil_moisture_percent;
  uint16_t soil_moisture_raw;
  
  // Kontrol Durumları (5 byte)
  uint8_t roof_position;        // 0-100%
  uint8_t pump_state;           // 0/1
  uint16_t irrigation_duration; // saniye
  
  // Hesaplanan Değerler (12 byte)
  float dew_point;
  float heat_index;
  float absolute_humidity;
  
  // Sistem (5 byte)
  uint32_t uptime;              // saniye
  uint8_t mhz14a_ready;         // 0/1
  
  // Veri Bütünlüğü (2 byte)
  uint16_t crc;
};
#pragma pack(pop)
// TOPLAM: 72 byte
```

### 3. Kontrol Sistemi

#### a) Sera Kapak Kontrolü
- **Girdi:** Sıcaklık, Nem, CO2, Işık, Basınç
- **Çıktı:** Kapak pozisyonu (0-100%)
- **Frekans:** 5 saniye
- **Histerezis:** 30 saniye (titreme önleme)

#### b) Sulama Kontrolü
- **Girdi:** Toprak Nemi, Sıcaklık, Hava Nemi, Işık
- **Çıktı:** Pompa AÇIK/KAPALI
- **Frekans:** 5 saniye
- **Minimum Bekleme:** 10 dakika

#### c) LoRa Haberleşme
- **Protokol:** Binary paket transferi
- **Paket Boyutu:** 72 byte
- **Gönderim Frekansı:** 5 saniye
- **Hata Kontrolü:** CRC-16
- **Mod:** Normal (M0=LOW, M1=LOW)
- **Menzil:** 3 km (açık alan)
- **Başarı Oranı:** >95% (ideal koşullar)

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

### Verici Sistemi (115200 baud)
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
...

>>> LORA VERI GONDERIMI <<<
Paket Boyutu: 72 byte
[DEBUG] Gonderilecek Paket Ozeti:
  Sicaklik: 25.2 C
  Nem: 59.4 %
  CO2: 450 ppm
  Toprak Nem: 45.5 %
  Sera Kapak: 25 %
  Sulama: KAPALI
  CRC: 0x1A2B
[LORA] *** PAKET BASARIYLA GONDERILDI ***
>>> LORA GONDERIM BITTI <<<
```

### Alıcı Sistemi - Yer İstasyonu (9600 baud)
```
=====================================================
        AKILLI TARIM SISTEMI - CANLI VERI           
=====================================================

*** PAKET BASARIYLA ALINDI ***

-----------------------------------------------------
>>> SISTEM BILGILERI <<<
-----------------------------------------------------
Sistem Calisma Suresi: 0s 5dk 30sn
MH-Z14A CO2 Sensor: HAZIR
CRC Kontrolu: 0x1A2B

-----------------------------------------------------
>>> HAVA KALITESI (BME680) <<<
-----------------------------------------------------
Sicaklik       : 25.16 C  [Ideal]
Nem            : 59.38 %  [Ideal]
Basinc         : 994.75 hPa
Gaz Direnci    : 165.22 KOhm  [Iyi]

-----------------------------------------------------
>>> ISIK SEVIYESI (BH1750) <<<
-----------------------------------------------------
Isik Siddeti   : 475.0 lux  [Parlak]
               = 44.13 fc (foot-candles)

-----------------------------------------------------
>>> CO2 SEVIYESI (MH-Z14A) <<<
-----------------------------------------------------
CO2 Konsant.   : 450 ppm  [Mukemmel]
Sensor Sicak.  : 24 C
Sensor Durum   : Stabil

-----------------------------------------------------
>>> TOPRAK NEM SENSORU <<<
-----------------------------------------------------
Toprak Nemi    : 45.5 %  [Optimal]
Ham Deger      : 512

-----------------------------------------------------
>>> HESAPLANAN DEGERLER <<<
-----------------------------------------------------
Ciy Noktasi         : 16.67 C
Hissedilen Sicaklik : 25.09 C
Mutlak Nem          : 8.14 g/m3
Sicak-Ciy Farki     : 8.49 C  [Normal]

-----------------------------------------------------
>>> SERA KONTROL SISTEMLERI <<<
-----------------------------------------------------
Sera Kapagi    : 25 %  [AZ ACIK]
Sulama Pompasi : KAPALI

-----------------------------------------------------
>>> GENEL DEGERLENDIRME <<<
-----------------------------------------------------
Sera Saglik Skoru: 85/100  [MUKEMMEL]

Aktif Uyarilar:
  Uyari yok - Tum sistemler normal

-----------------------------------------------------
>>> ILETISIM ISTATISTIKLERI <<<
-----------------------------------------------------
Oturum Suresi     : 5 dk 30 sn
Basarili Paket    : 66
Bozuk Paket       : 2
Basari Orani      : 97.1 %
Paket Hizi        : 12.0 paket/dk
-----------------------------------------------------
```

---

## 🔋 Güç Tüketimi

### Verici Sistem
| Bileşen | Akım | Güç |
|---------|------|-----|
| Arduino Mega | ~50mA | 0.25W |
| BH1750 | ~0.2mA | 0.001W |
| BME680 | ~3.7mA | 0.018W |
| MH-Z14A | ~150mA | 0.75W |
| Soil Sensor | ~20mA | 0.1W |
| LoRa E32 TX | ~120mA | 0.6W |
| Servo (SG90) | ~100-500mA | 0.5-2.5W |
| Röle + Pompa | ~50mA + Pompa | 0.25W + Pompa |
| **TOPLAM** | **~500mA** | **~2.5-5W** |

### Alıcı Sistem (Yer İstasyonu)
| Bileşen | Akım | Güç |
|---------|------|-----|
| Arduino | ~50mA | 0.25W |
| LoRa E32 RX | ~20mA | 0.1W |
| **TOPLAM** | **~70mA** | **~0.35W** |

*Not: Pompa gücü modele göre değişir (genelde 5-12W)*

**Önerilen Güç Kaynakları:**
- Verici: 5V 3A adaptör
- Alıcı: 5V 1A adaptör veya USB

---

## 📁 Dosya Yapısı

```
Tarhun Bitirme Projesi/
│
├── platformio.ini              # PlatformIO konfigürasyonu
├── README.md                   # Proje açıklaması
├── kosullar.md                 # Kontrol koşulları
├── sistem_tasarimi.md          # Sistem tasarım dokümantasyonu
├── YerIstasyonu_Alici.ino     # Alıcı kodu (Arduino IDE)
│
├── src/
│   └── main.cpp                # Ana verici program kodu
│
├── include/
│   └── README                  # Header dosyaları
│
└── lib/
    └── README                  # Kütüphaneler
```

---

## 📚 Kullanılan Kütüphaneler

### Verici Sistem (PlatformIO)
```ini
lib_deps = 
    claws/BH1750@^1.3.0
    adafruit/Adafruit BME680 Library@^2.0.4
    adafruit/Adafruit Unified Sensor@^1.1.14
    xreef/EByte LoRa E32 library@^1.5.10
```

### Alıcı Sistem (Arduino IDE)
- **EByte LoRa E32 library** (Library Manager'dan kurulur)

---

## 🚀 Kurulum ve Kullanım

### 1. Donanım Montajı

#### Verici Sistem (Sera İçi)
1. Tüm sensörleri Arduino Mega'ya bağlayın
2. LoRa E32 modülünü D10, D11, D6, D7 pinlerine bağlayın
3. Servo motoru D9'a bağlayın (harici güç)
4. Röle modülünü D10'a bağlayın
5. Güç kaynağını bağlayın (5V 3A)

#### Alıcı Sistem (Yer İstasyonu)
1. Arduino'ya LoRa E32 modülünü bağlayın (aynı pin konfigürasyonu)
2. USB ile bilgisayara bağlayın
3. Serial Monitor açın (9600 baud)

### 2. Yazılım Yükleme

#### Verici Sistem
```bash
# PlatformIO ile
cd "I:\Drive'ım\Bitirme Tezi\Tarhun Bitirme Projesi"
pio run --target upload
```

#### Alıcı Sistem
1. Arduino IDE'yi açın
2. `YerIstasyonu_Alici.ino` dosyasını açın
3. Library Manager'dan "EByte LoRa E32" kütüphanesini kurun
4. Board ve Port seçin
5. Upload butonuna tıklayın

### 3. İlk Çalıştırma

#### Verici
1. Seri monitörü açın (115200 baud)
2. MH-Z14A sensörünün 3 dakika ısınmasını bekleyin
3. Sensör değerlerini ve LoRa gönderimlerini gözlemleyin
4. Sistem otomatik kontrole başlayacak

#### Alıcı
1. Serial Monitor açın (9600 baud)
2. LoRa paketlerinin geldiğini gözlemleyin
3. Detaylı sensör verilerini ve analizleri görün
4. İstatistikleri takip edin

### 4. Kalibrasyon
- **Toprak Nem Sensörü:**
  - Kuru değer: Sensörü havada tutun, değeri kaydedin
  - Islak değer: Sensörü suya batırın, değeri kaydedin
  - `main.cpp` içinde `SOIL_DRY_VALUE` ve `SOIL_WET_VALUE` güncelleyin

- **LoRa Menzil:**
  - İlk testlerde 10-20 metre mesafede deneyin
  - Sinyal kalitesini CRC başarı oranından takip edin
  - İdeal koşullarda 3 km'ye kadar çıkabilir

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

### LoRa İletişim Testleri
1. **Yakın Mesafe (1-5m):**
   - CRC başarı oranı: >99%
   - Her paket ulaşmalı
   
2. **Orta Mesafe (10-50m):**
   - CRC başarı oranı: >95%
   - Ara sıra paket kayıpları normal
   
3. **Uzak Mesafe (100-500m):**
   - CRC başarı oranı: >90%
   - Engellere dikkat
   
4. **Hata Kontrolü:**
   - Bozuk paketler CRC ile otomatik tespit edilir
   - Yer istasyonunda istatistikler takip edilir
   
5. **Sinyal Kalitesi İyileştirme:**
   - Antenleri dik konumda tutun
   - Metal engellerden uzak durun
   - Yüksekliği artırın
   - Açık alan kullanın

---

## 🛡️ Güvenlik Özellikleri

1. **Histerezis:** 30 saniye minimum hareket aralığı (titreme önleme)
2. **Timeout:** MH-Z14A 3 dakika ısınma süresi
3. **Sınır Kontrolü:** Tüm değerler min/max kontrollü
4. **Aşırı Sulama Koruması:** 90% üstü nemde sulama kilidi
5. **Donma Koruması:** 10°C altında kapak otomatik kapanır

---

## 📊 Performans Metrikleri

### Verici Sistem
- **Veri Okuma Frekansı:** 5 saniye
- **Karar Alma Süresi:** <100ms
- **Servo Yanıt Süresi:** ~500ms
- **Röle Yanıt Süresi:** <50ms
- **LoRa Gönderim Süresi:** ~100ms
- **Sensör Doğruluğu:**
  - Sıcaklık: ±1°C
  - Nem: ±3%
  - CO2: ±50ppm
  - Işık: ±20%
  - Toprak Nem: ±5%

### Alıcı Sistem
- **Paket Alma Süresi:** <50ms
- **CRC Doğrulama:** <10ms
- **Veri İşleme:** <100ms
- **Serial Çıktı:** <500ms
- **Başarı Oranı:** >95% (ideal koşullar)

### LoRa İletişim
- **Bant Genişliği:** 125 kHz
- **Paket Boyutu:** 72 byte
- **Hava Süresi:** ~200ms/paket
- **Maksimum Veri Hızı:** ~5 paket/saniye
- **Gerçek Kullanım:** 0.2 paket/saniye (5s aralık)
- **Enerji Verimliliği:** Yüksek (duty cycle %4)

---

## 🔮 Gelecek Geliştirmeler

1. **GSM/4G Modülü** - İnternet üzerinden uzaktan izleme
2. **SD Kart** - Veri kaydetme ve log tutma
3. **LCD Ekran** - Yerel veri görüntüleme (verici tarafta)
4. **Web Dashboard** - Grafiksel arayüz ve tarihsel veri analizi
5. **Mobil Uygulama** - Akıllı telefon kontrolü ve bildirimler
6. **Yapay Zeka** - Makine öğrenmesi ile optimizasyon ve tahminleme
7. **Güneş Paneli** - Enerji bağımsızlığı
8. **Çoklu Bölge** - Farklı bitki türleri için bölgesel kontrol
9. **Hava Durumu API** - Dış hava durumu ile entegrasyon
10. **LoRaWAN Gateway** - The Things Network entegrasyonu
11. **Kamera Modülü** - Bitki sağlığı görsel analizi
12. **Çoklu Alıcı** - Birden fazla yer istasyonu desteği

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

**Son Güncelleme:** 27 Ekim 2025

**Versiyon:** 2.0 - LoRa Kablosuz İletişim Entegrasyonu
