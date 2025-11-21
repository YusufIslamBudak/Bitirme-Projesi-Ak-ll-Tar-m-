# Akıllı Sera Sistemi - Sistem Tasarımı

## 📋 Proje Özeti

Çoklu sensör entegrasyonu ile otomatik/manuel sera kontrol sistemi. Sıcaklık, nem, CO2, ışık ve toprak nemi verilerini kullanarak sera kapağı, havalandırma, aydınlatma ve sulama sistemini akıllı bir şekilde kontrol eder.

**Özellikler:**
- ✅ Otomatik mod: Sensör verilerine göre akıllı kontrol
- ✅ Manuel mod: Serial komutlarla anlık kontrol
- ✅ LoRa kablosuz veri iletimi (3 km menzil)
- ✅ 4 kontrol sistemi (Kapak+Fan, Sulama, Aydınlatma)

---

## 🎮 Kontrol Modları

### 1. OTOMATIK MOD (Varsayılan)
Sistem sensör verilerine göre 9 sera kodu ve 8 sulama kodu ile otomatik kararlar verir.

### 2. MANUEL MOD
Serial port üzerinden komutlarla anlık kontrol:

**Serial Komutlar (115200 baud):**
```
havaac    → Sera kapağını aç (0°) + Fan açık
havakapa  → Sera kapağını kapat (95°) + Fan kapalı
isikac    → Aydınlatmayı aç (D29)
isikkapa  → Aydınlatmayı kapat (D29)
sulaac    → Sulamayı aç (D31)
sulakapa  → Sulamayı kapat (D31)
```

**⚠️ Sulama Güvenlik Özelliği:**
- `sulaac` komutu verildiğinde sistem otomatik olarak:
  1. Mevcut tüm sistem durumlarını kaydeder
  2. Sera kapağını kapatır (95°)
  3. Fan'ı kapatır
  4. Işığı kapatır
  5. Sulamayı başlatır
  
- `sulakapa` komutu verildiğinde:
  1. Sulama durdurulur
  2. Tüm sistemler önceki kayıtlı durumuna otomatik geri döner

**Kullanım:** Serial Monitor'da komutu yazıp Enter'a basın.

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
- **Model:** MG995 (Metal dişlili, yüksek tork)
- **Kontrol:** PWM
- **Açı:** 0° (Tam Açık) ~ 95° (Tam Kapalı)
- **Pin:** D9
- **Güç:** 4.8-7.2V, 2.5A (yük altında)
- **Tork:** 10 kg-cm
- **Özellikler:**
  - Metal dişliler (dayanıklı)
  - Çift rulman (hassas)
  - Su geçirmez koruma
  
#### b) Havalandırma Fanı Rölesi
- **Pin:** D30
- **Kontrol:** Dijital (LOW=Açık, HIGH=Kapalı)
- **Özellikler:**
  - Sera kapağı >30% açıkken otomatik aktif
  - Manuel kontrol ile bağımsız çalıştırılabilir
  
#### c) Aydınlatma Rölesi
- **Pin:** D29 (Active LOW)
- **Kontrol:** Dijital (LOW=Açık, HIGH=Kapalı)
- **Kullanım:**
  - Otomatik: Işık < 200 lux → Açık
  - Manuel: Komut isikac/isikkapa ile kontrol
- **Not:** D7'den D29'a taşındı (LoRa M1 pin çakışması önlendi)
  
#### d) Sulama Pompası Rölesi
- **Pin:** D31 (D10'dan taşındı - LoRa çakışması çözüldü)
- **Kontrol:** Dijital (LOW=Açık, HIGH=Kapalı)
- **Kullanım:**
  - Otomatik: Toprak nemi < 40% → 20-30 saniye
  - Manuel: Komut 3/-3 ile kontrol

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
├─ UART2 (D16/TX2, D17/RX2)
│  └─ NodeMCU ESP8266 (9600 baud, JSON)
│     - SoftwareSerial (NodeMCU D1=GPIO5)
│     - Firebase Realtime Database
│     - SD Kart veri kaydetme
│
├─ Software Serial (D10/RX, D11/TX)
│  └─ LoRa E32 Modülü (Verici)
│     - M0 → D6
│     - M1 → D8 (LoRa kontrol pini)
│
├─ Analog Input
│  └─ A0 → MH Water Sensor
│
├─ PWM Output
│  └─ D9 → MG995 Servo Motor (Sera Kapağı)
│
└─ Digital Outputs (Aktuatörler)
   ├─ D29 → Röle (Işık) - Active LOW
   ├─ D30 → Röle (Fan) - Active LOW
   └─ D31 → Röle (Sulama Pompası) - Active LOW

         ↓↓↓ LoRa 433MHz Kablosuz ↓↓↓
         
ARDUINO (ALICI - YER İSTASYONU)
│
└─ Software Serial (D10/RX, D11/TX)
   └─ LoRa E32 Modülü (Alıcı)
      - M0 → D6
      - M1 → D8
      - Serial Monitor → USB (9600 baud)

         ↓↓↓ WiFi / İnternet ↓↓↓

FIREBASE & SD KART SİSTEMİ
│
└─ NodeMCU ESP8266
   ├─ Firebase Realtime Database (Bulut)
   └─ SD Kart Modülü (Yerel)
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
                      │    ├──> Servo Motor (Sera Kapağı)
                      │    ├──> Röle (Fan)
                      │    ├──> Röle (Işık)
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

### 1. Modüler Yapı (Modern C++ Tasarımı)

Proje modüler ve bakımı kolay bir mimari ile tasarlanmıştır. Her modül kendi sorumluluğunu yerine getirir:

#### a) Communication Modülü
- **Konum:** `lib/Communication/` ve `include/Communication.h`
- **Sorumluluk:** Seri port iletişimi, LoRa veri gönderimi
- **İçerik:**
  - `printSeparator()` - Görsel ayırıcı çizgiler
  - `sendLoRaPacket()` - Binary paket gönderimi
  - `calculateCRC16()` - Veri doğrulama
  - `printHexDump()` - Debug için HEX çıktısı

#### b) Sensors Modülü
- **Konum:** `src/Sensors.cpp` ve `include/Sensors.h`
- **Sorumluluk:** Tüm sensör okuma işlemleri + Kalman filtreleme
- **İçerik:**
  - `SensorReadings` struct (RAW ve FILTERED değerler)
  - `readBH1750()` - Işık sensörü
  - `readBME680()` - Hava kalitesi sensörü
  - `readMHZ14A()` - CO2 sensörü
  - `readSoilMoisture()` - Toprak nem sensörü
  - `readAllSensors()` - Tüm sensörleri tek seferde oku
  - 7 adet KalmanFilter nesnesi (her sensör için)

#### c) KalmanFilter Modülü
- **Konum:** `src/KalmanFilter.cpp` ve `include/KalmanFilter.h`
- **Sorumluluk:** 1D Kalman filtresi ile sensör gürültüsü azaltma
- **İçerik:**
  - `update(measurement)` - Yeni ölçüm ile filtreleme
  - `reset()` - Filtreyi sıfırla
  - `getValue()` - Filtrelenmiş değeri al
  - `setProcessNoise(q)` - Süreç gürültüsü ayarı
  - `setMeasurementNoise(r)` - Ölçüm gürültüsü ayarı
  - `getKalmanGain()` - Kazanç faktörünü görüntüle

#### d) Calculations Modülü
- **Konum:** `src/Calculations.cpp` ve `include/Calculations.h`
- **Sorumluluk:** Bilimsel hesaplamalar
- **İçerik:**
  - `calculateDewPoint()` - Çiy noktası (Magnus formülü)
  - `calculateAbsoluteHumidity()` - Mutlak nem
  - `calculateHeatIndex()` - Hissedilen sıcaklık (Rothfusz)
  - `calculateVaporPressure()` - Buhar basıncı
  - `calculateSeaLevelPressure()` - Deniz seviyesi basıncı
  - `luxToFootCandles()` - Işık birimi dönüşümü
  - `co2PpmToMgPerM3()` - CO2 yoğunluğu
  - `calculateVentilationRate()` - Havalandırma oranı

### 2. Ana Döngü (Loop)
```cpp
loop() {
  // Seri port komutlarını kontrol et (Manuel Mod)
  processSerialCommand()
  
  // Her 5 saniyede bir sensör okuma ve otomatik kontrol
  if (millis() - lastSensorRead >= 5000) {
    sensors.readAllSensors()  // Modüler sensör okuma (Kalman filtreli)
    // Filtrelenmiş değerler readings değişkeninde:
    // readings.temperature_filtered, readings.humidity_filtered vb.
    
    calculateValues()      // Calculations modülü kullanılarak hesaplamalar
    controlGreenhouse()    // Sera kapak kontrolü (otomatik mod)
    controlIrrigation()    // Sulama kontrolü (otomatik mod)
    sendLoRaData()        // LoRa ile veri gönder (filtered değerler)
    printData()           // Seri port çıktısı (RAW ve FILTERED karşılaştırma)
    lastSensorRead = millis()
  }
}
```

### 3. LoRa Veri Paketi Yapısı
```cpp
#pragma pack(push,1)
struct SensorDataPacket {
  // BME680 (16 byte) - FILTERED değerler
  float temperature;        // Kalman filtreli sıcaklık
  float humidity;           // Kalman filtreli nem
  float pressure;           // Kalman filtreli basınç
  float gas_resistance;     // Kalman filtreli gaz direnci
  
  // BH1750 (4 byte) - FILTERED değer
  float lux;                // Kalman filtreli ışık
  
  // MH-Z14A (3 byte) - FILTERED değerler
  uint16_t co2_ppm;         // Kalman filtreli CO2
  int8_t co2_temperature;   // CO2 sensör sıcaklığı
  
  // Toprak Nem (6 byte) - FILTERED değer
  float soil_moisture_percent;  // Kalman filtreli toprak nemi
  uint16_t soil_moisture_raw;   // Ham ADC değeri
  
  // Kontrol Durumları (6 byte)
  uint8_t roof_position;        // 0-100%
  uint8_t fan_state;            // 0/1
  uint8_t light_state;          // 0/1
  uint8_t pump_state;           // 0/1
  uint16_t irrigation_duration; // saniye
  
  // Hesaplanan Değerler (12 byte)
  float dew_point;              // Filtrelenmiş verilerden hesaplanır
  float heat_index;             // Filtrelenmiş verilerden hesaplanır
  float absolute_humidity;      // Filtrelenmiş verilerden hesaplanır
  
  // Sistem (5 byte)
  uint32_t uptime;              // saniye
  uint8_t mhz14a_ready;         // 0/1
  
  // Veri Bütünlüğü (2 byte)
  uint16_t crc;
};
#pragma pack(pop)
// TOPLAM: 54 byte
// Not: Kalman filtreli değerler gönderilir, böylece alıcı tarafta 
// temiz ve kararlı veriler elde edilir.
```

### 4. Kalman Filtresi Sistemi

#### Kalman Filtresi Nedir?
Kalman filtresi, gürültülü sensör ölçümlerinden optimal tahminler üreten matematiksel bir algoritmadır. İki aşamadan oluşur:

1. **Tahmin (Prediction):** Sistemin bir sonraki durumunu tahmin et
2. **Güncelleme (Update):** Yeni ölçüm ile tahmini düzelt

#### Kalman Filtresi Parametreleri

Her sensör için optimize edilmiş parametreler:

| Sensör | Process Noise (q) | Measurement Noise (r) | Açıklama |
|--------|-------------------|----------------------|----------|
| Sıcaklık | 0.001 | 0.5 | Yavaş değişir, orta güven |
| Nem | 0.001 | 1.0 | Yavaş değişir, düşük güven |
| Basınç | 0.0001 | 0.1 | Çok yavaş, yüksek güven |
| Gaz | 0.01 | 5.0 | Hızlı değişir, düşük güven |
| Işık | 0.01 | 2.0 | Orta hız, orta güven |
| CO2 | 0.01 | 10.0 | Orta hız, düşük güven |
| Toprak Nem | 0.001 | 2.0 | Yavaş değişir, orta güven |

**q (Process Noise):** Sistem dinamiklerindeki belirsizlik
- Düşük q → Sistem durağan kabul edilir
- Yüksek q → Sistem hızlı değişebilir

**r (Measurement Noise):** Ölçüm gürültüsü
- Düşük r → Sensöre yüksek güven
- Yüksek r → Sensöre düşük güven

#### Kalman Filtresi Algoritması

```cpp
class KalmanFilter {
private:
    float _x;  // Durum tahmini (estimated state)
    float _p;  // Tahmin hatası (estimation error)
    float _q;  // Süreç gürültüsü (process noise)
    float _r;  // Ölçüm gürültüsü (measurement noise)
    
public:
    float update(float measurement) {
        // TAHMİN AŞAMASI (Prediction)
        _p = _p + _q;  // Tahmin hatası artar
        
        // GÜNCELLEME AŞAMASI (Update)
        float k = _p / (_p + _r);  // Kalman kazancı
        _x = _x + k * (measurement - _x);  // Durum güncellemesi
        _p = (1 - k) * _p;  // Hata güncellemesi
        
        return _x;  // Filtrelenmiş değer
    }
};
```

#### Kalman Filtresi Avantajları

✅ **Gürültü Azaltma:** Sensör titremeleri düzeltilir  
✅ **Gerçek Değişimleri Koruma:** Ani sıcaklık artışları korunur  
✅ **Düşük Hesaplama Maliyeti:** Arduino'da hızlı çalışır  
✅ **Otomatik Adaptasyon:** Kalman kazancı kendini ayarlar  
✅ **Kararlı Kontrol:** Röle ve servo daha az tetiklenir  

#### Örnek: Kalman Filtresi Etkisi

**Toprak Nem Sensörü (Gerçek Test Verisi):**

| Zaman | RAW Değer | FILTERED Değer | Fark |
|-------|-----------|----------------|------|
| 0s | 65.0% | 65.00% | 0% |
| 5s | 100.0% | 76.67% | -23.3% (ani sıçrama filtrelendi) |
| 10s | 99.5% | 82.52% | -17.0% |
| 15s | 98.0% | 86.02% | -12.0% |
| 20s | 97.5% | 88.36% | -9.1% |

**Sonuç:** Ham sensör 100%'e sıçradı (muhtemelen gürültü), ancak Kalman filtresi gerçek değişimi kademeli takip etti.

#### Serial Çıktı Formatı (RAW vs FILTERED)

```
--- GY-30 (BH1750) Light Sensor ---
Light Level: 447.50 lux (RAW) | 447.77 lux (FILTERED)
  -> Bright

--- BME680 Air Quality Sensor ---
Temperature: 23.14 C (RAW) | 22.93 C (FILTERED)
Humidity: 58.76 % (RAW) | 58.75 % (FILTERED)
Pressure: 994.50 hPa (RAW) | 994.48 hPa (FILTERED)
Gas Resistance: 166.02 KOhm (RAW) | 168.74 KOhm (FILTERED)

--- MH-Z14A CO2 Sensor ---
CO2 Level: 450 ppm (RAW) | 450 ppm (FILTERED)

--- MH Water Soil Moisture Sensor ---
Soil Moisture: 100.00 % (RAW) | 88.36 % (FILTERED)
  -> ISLAK TOPRAK (Sulamaya gerek yok)
```

**Önemli Not:** LoRa ile gönderilen paketlerde **sadece FILTERED değerler** kullanılır. Bu sayede alıcı tarafta temiz ve kararlı veriler işlenir.

### 5. Kontrol Sistemi

#### a) Sera Kapak ve Havalandırma Kontrolü
- **Girdi:** Sıcaklık (FILTERED), Nem (FILTERED), CO2 (FILTERED), Işık (FILTERED), Basınç (FILTERED)
- **Çıktı:** Kapak pozisyonu (0-100%) ve Fan durumu (AÇIK/KAPALI)
- **Mod:**
  - **Otomatik:** Kalman filtreli sensör verilerine göre karar algoritması
  - **Manuel:** Seri port komutları (havaac/havakapa)
- **Frekans:** 5 saniye
- **Histerezis:** 30 saniye (titreme önleme)
- **Avantaj:** Kalman filtresi sayesinde kapak gereksiz açılıp kapanmaz

#### b) Aydınlatma Kontrolü
- **Çıktı:** LED/Lamba AÇIK/KAPALI
- **Mod:**
  - **Otomatik:** Kalman filtreli ışık sensörü verilerine göre (gelecekte eklenebilir)
  - **Manuel:** Seri port komutları (isikac/isikkapa)
- **Pin:** D29 (Active LOW) - D7'den taşındı

#### c) Sulama Kontrolü
- **Girdi:** Toprak Nemi (FILTERED), Sıcaklık (FILTERED), Hava Nemi (FILTERED), Işık (FILTERED)
- **Çıktı:** Pompa AÇIK/KAPALI
- **Mod:**
  - **Otomatik:** Kalman filtreli toprak nem sensörü verilerine göre
  - **Manuel:** Seri port komutları (sulaac/sulakapa)
- **Frekans:** 5 saniye
- **Minimum Bekleme:** 10 dakika
- **Avantaj:** Kalman filtresi toprak nem gürültüsünü azaltarak gereksiz sulama önler
- **⚠️ Güvenlik Özelliği:** 
  - Sulama başladığında diğer tüm sistemler otomatik kapatılır
  - Sulama bittiğinde sistemler önceki durumuna geri döner
  - Bu özellik elektriksel güvenlik ve su-elektrik teması riskini önler

#### d) Manuel Kontrol (Seri Port)
- **Baud Rate:** 115200
- **Komutlar:**
  - `havaac`: Kapak aç + Fan aç
  - `havakapa`: Kapak kapat + Fan kapat
  - `isikac`: Işık aç
  - `isikkapa`: Işık kapat
  - `sulaac`: Sulama aç (diğer sistemleri kapat)
  - `sulakapa`: Sulama kapat (diğer sistemleri geri yükle)
- **Özellikler:** 
  - Servo titreme önleme (attach/detach pattern)
  - Durum kaydetme ve geri yükleme (sulama güvenliği)
  - Büyük/küçük harf duyarsız komut işleme

#### e) LoRa Haberleşme
- **Protokol:** Binary paket transferi
- **Paket Boyutu:** 54 byte (Kalman filtreli veriler)
- **Gönderim Frekansı:** 5 saniye
- **Hata Kontrolü:** CRC-16
- **Mod:** Normal (M0=LOW, M1=LOW)
- **Menzil:** 3 km (açık alan)
- **Başarı Oranı:** >95% (ideal koşullar)
- **Veri Kalitesi:** Yüksek (Kalman filtreli temiz veriler gönderilir)

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
Light Level: 475.00 lux (RAW) | 474.85 lux (FILTERED)
Light Level: 44.13 fc
  -> Bright

--- BME680 Air Quality Sensor ---
Temperature: 25.16 C (RAW) | 25.14 C (FILTERED)
Pressure: 994.75 hPa (RAW) | 994.74 hPa (FILTERED)
Humidity: 59.38 % (RAW) | 59.36 % (FILTERED)
Gas Resistance: 165.22 KOhm (RAW) | 165.89 KOhm (FILTERED)
...

--- MH-Z14A CO2 Sensor ---
CO2 Level: 450 ppm (RAW) | 450 ppm (FILTERED)
Sensor Temperature: 24 C
...

--- MH Water Soil Moisture Sensor ---
Soil Moisture: 45.50 % (RAW) | 45.48 % (FILTERED)
  -> NORMAL (Sulama gerekli)
...

>>> LORA VERI GONDERIMI <<<
Paket Boyutu: 54 byte (Kalman filtreli veriler)
[DEBUG] Gonderilecek Paket Ozeti:
  Sicaklik: 25.14 C (FILTERED)
  Nem: 59.36 % (FILTERED)
  CO2: 450 ppm (FILTERED)
  Toprak Nem: 45.48 % (FILTERED)
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
  
Kalman Filter Durumu:
  ✓ Gurultu azaltma aktif
  ✓ Kararli veri akisi
  ✓ Rele ve servo gereksiz tetiklenmeleri onlendi

-----------------------------------------------------
>>> ILETISIM ISTATISTIKLERI <<<
-----------------------------------------------------
Oturum Suresi     : 5 dk 30 sn
Basarili Paket    : 66
Bozuk Paket       : 2
Basari Orani      : 97.1 %
Paket Hizi        : 12.0 paket/dk
Paket Boyutu      : 54 byte (Kalman filtreli)
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

## 📁 Dosya Yapısı (Modüler Mimari)

```
Tarhun Bitirme Projesi/
│
├── platformio.ini              # PlatformIO konfigürasyonu
├── README.md                   # Proje açıklaması
├── kosullar.md                 # Kontrol koşulları (Sera + Sulama kodları)
├── sistem_tasarimi.md          # Sistem tasarım dokümantasyonu (Kalman filtresi)
├── YerIstasyonu_Alici.ino     # Alıcı kodu (Arduino IDE)
│
├── src/                        # Kaynak kodlar (Implementation)
│   ├── main.cpp                # Ana verici program kodu
│   ├── Sensors.cpp             # Sensör okuma + Kalman filtreleme
│   ├── Calculations.cpp        # Bilimsel hesaplamalar
│   ├── KalmanFilter.cpp        # 1D Kalman filtresi algoritması
│   └── Communication.cpp       # Seri port + LoRa iletişimi
│
├── include/                    # Header dosyaları (Interface)
│   ├── Sensors.h               # Sensör modülü arayüzü
│   ├── Calculations.h          # Hesaplamalar arayüzü
│   ├── KalmanFilter.h          # Kalman filtresi arayüzü
│   ├── Communication.h         # İletişim arayüzü
│   └── README                  # Header dosyaları açıklaması
│
└── lib/                        # Kütüphaneler
    ├── Communication/          # Communication modülü alternatif konumu
    │   ├── Communication.h
    │   └── Communication.cpp
    └── README                  # Kütüphane açıklaması
```

### Modül Detayları

#### 1. Sensors Modülü (371 satır)
- **Amaç:** Tüm sensör okuma işlemlerini merkezileştirir
- **Özellikler:**
  - Her sensör için ayrı okuma fonksiyonu
  - 7 adet KalmanFilter nesnesi (temperature, humidity, pressure, gas, lux, co2, soil)
  - RAW ve FILTERED değerleri aynı anda tutar
  - Serial çıktısında karşılaştırmalı gösterim
- **Kullanım:**
  ```cpp
  Sensors sensors;
  sensors.begin();
  sensors.readAllSensors();  // Tüm sensörleri oku ve filtrele
  float temp = readings.temperature_filtered;  // Filtrelenmiş sıcaklık
  float temp_raw = readings.temperature_raw;   // Ham sıcaklık
  ```

#### 2. KalmanFilter Modülü (60 satır)
- **Amaç:** 1D Kalman filtresi ile sensör gürültüsü azaltma
- **Özellikler:**
  - Hafif ve hızlı algoritma (Arduino için optimize)
  - Her sensör için ayrı parametre ayarı
  - Otomatik başlatma (ilk ölçüm ile)
- **Kullanım:**
  ```cpp
  KalmanFilter kf(0.001, 0.5);  // q=0.001, r=0.5
  float filtered = kf.update(raw_measurement);
  ```

#### 3. Calculations Modülü (84 satır)
- **Amaç:** Bilimsel hesaplamaları kodun geri kalanından ayırır
- **Özellikler:**
  - Tüm fonksiyonlar static (nesne gerektirmez)
  - Doğrulanmış formüller (Magnus, Rothfusz, İdeal Gaz)
  - SI ve imperial birim dönüşümleri
- **Kullanım:**
  ```cpp
  float dew = Calculations::calculateDewPoint(temp, humidity);
  float hi = Calculations::calculateHeatIndex(temp, humidity);
  ```

#### 4. Communication Modülü (var olan, yeniden kullanıldı)
- **Amaç:** Seri port ve LoRa iletişimi
- **Özellikler:**
  - Binary paket gönderimi
  - CRC-16 hesaplama
  - HEX dump debug çıktısı
- **Kullanım:**
  ```cpp
  Communication::sendLoRaPacket(packet, sizeof(packet));
  uint16_t crc = Communication::calculateCRC16(data, len);
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
3. MG995 Servo motoru D9'a bağlayın (harici 5V güç)
4. Fan rölesini D30'a bağlayın (Active LOW)
5. Işık rölesini D7'ye bağlayın (Active LOW) - *LoRa M1 pin ile çakışma yok*
6. Sulama rölesini D31'e bağlayın (Active LOW)
7. Güç kaynağını bağlayın (5V 3A)

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
5. **Manuel Kontrol:** Seri monitörden komut gönderin:
   - `1` → Kapak aç + Fan aç
   - `-1` → Kapak kapat + Fan kapat
   - `2` → Işık aç
   - `-2` → Işık kapat
   - `3` → Sulama aç
   - `-3` → Sulama kapat

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
1. **Sera Kapak (Manuel):** 
   - Komut `havaac` → Servo 0° (açık), Fan AÇIK
   - Komut `havakapa` → Servo 95° (kapalı), Fan KAPALI
   - Servo titreme olmadan hareket etmeli
   
2. **Sera Kapak (Otomatik):** 
   - Sıcaklık değişimlerinde kapak hareketini gözleyin
   
3. **Aydınlatma (Manuel):**
   - Komut `isikac` → D7 röle AÇIK
   - Komut `isikkapa` → D7 röle KAPALI
   
4. **Sulama Güvenlik Testi (Manuel):**
   - Test senaryosu:
     1. `havaac` ile kapağı ve fanı aç
     2. `isikac` ile ışığı aç
     3. `sulaac` komutu gönder
     4. **Kontrol:** Kapak kapanmalı, fan kapanmalı, ışık kapanmalı, sulama başlamalı
     5. `sulakapa` komutu gönder
     6. **Kontrol:** Sulama kapanmalı, tüm sistemler önceki durumuna (açık) dönmeli
   - ✅ Beklenen: Sistemler kaydedilen durumuna geri dönmeli
   
5. **Sulama (Otomatik):**
   - Toprak nem seviyesini değiştirerek pompayı test edin

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

1. **Kalman Filtresi:** Sensör gürültülerini azaltarak yanlış kararları önler
2. **Histerezis:** 30 saniye minimum hareket aralığı (titreme önleme)
3. **Servo Titreme Önleme:** Attach/detach pattern (PWM sinyali sadece hareket anında aktif)
4. **Sulama Güvenlik Sistemi:** 
   - Sulama başladığında tüm elektrikli sistemler otomatik kapatılır
   - Sulama bittiğinde sistemler önceki durumuna otomatik geri döner
   - Su-elektrik teması riski minimize edilir
5. **Durum Kaydetme/Geri Yükleme:** Manuel sulama komutlarında state management
6. **Timeout:** MH-Z14A 3 dakika ısınma süresi
7. **Sınır Kontrolü:** Tüm değerler min/max kontrollü
8. **Aşırı Sulama Koruması:** 90% üstü nemde sulama kilidi
9. **Donma Koruması:** 10°C altında kapak otomatik kapanır
10. **Non-Blocking Loop:** millis() tabanlı zamanlama (seri komutlar kesintisiz işlenir)
11. **Komut Doğrulama:** Sözel komutlar ile yanlış tetikleme önlenir
12. **Veri Bütünlüğü:** CRC-16 ile LoRa paketleri doğrulanır

---

## 📊 Performans Metrikleri

### Verici Sistem
- **Veri Okuma Frekansı:** 5 saniye
- **Kalman Filtresi İşlem Süresi:** <5ms (7 sensör için)
- **Karar Alma Süresi:** <100ms
- **Servo Yanıt Süresi:** ~500ms
- **Röle Yanıt Süresi:** <50ms
- **LoRa Gönderim Süresi:** ~100ms
- **Sensör Doğruluğu (Filtrelenmiş):**
  - Sıcaklık: ±0.3°C (Ham: ±1°C)
  - Nem: ±1% (Ham: ±3%)
  - CO2: ±20ppm (Ham: ±50ppm)
  - Işık: ±10% (Ham: ±20%)
  - Toprak Nem: ±2% (Ham: ±5%)

### Alıcı Sistem
- **Paket Alma Süresi:** <50ms
- **CRC Doğrulama:** <10ms
- **Veri İşleme:** <100ms
- **Serial Çıktı:** <500ms
- **Başarı Oranı:** >95% (ideal koşullar)

### LoRa İletişim
- **Bant Genişliği:** 125 kHz
- **Paket Boyutu:** 54 byte (Kalman filtreli, optimize)
- **Hava Süresi:** ~180ms/paket (v2.0: 200ms, %10 daha hızlı)
- **Maksimum Veri Hızı:** ~5 paket/saniye
- **Gerçek Kullanım:** 0.2 paket/saniye (5s aralık)
- **Enerji Verimliliği:** Yüksek (duty cycle %3.6, v2.0: %4)

### Kalman Filtresi Performansı
- **İşlem Süresi:** <1ms/sensör
- **Bellek Kullanımı:** 28 byte/filtre (7 filtre = 196 byte)
- **Gürültü Azaltma:** %60-80 (sensöre göre değişir)
- **Gecikme:** 1-2 okuma döngüsü (5-10 saniye)
- **Kararlılık:** 3-4 okuma sonrası optimal

---

## 🔮 Gelecek Geliştirmeler

### Yakın Vadede (1-3 ay)
1. **GSM/4G Modülü** - İnternet üzerinden uzaktan izleme
2. **SD Kart** - Veri kaydetme ve log tutma (filtrelenmiş + ham veriler)
3. **LCD Ekran** - Yerel veri görüntüleme (verici tarafta)
4. **Adaptif Kalman Filtresi** - Parametreleri otomatik ayarlama

### Orta Vadede (3-6 ay)
5. **Web Dashboard** - Grafiksel arayüz ve tarihsel veri analizi
6. **Mobil Uygulama** - Akıllı telefon kontrolü ve bildirimler
7. **Çoklu Bölge** - Farklı bitki türleri için bölgesel kontrol
8. **Hava Durumu API** - Dış hava durumu ile entegrasyon
9. **Kamera Modülü** - Bitki sağlığı görsel analizi

### Uzun Vadede (6-12 ay)
10. **Yapay Zeka** - Makine öğrenmesi ile optimizasyon ve tahminleme
11. **Güneş Paneli** - Enerji bağımsızlığı
12. **LoRaWAN Gateway** - The Things Network entegrasyonu
13. **Çoklu Alıcı** - Birden fazla yer istasyonu desteği
14. **Predictive Maintenance** - Sensör arızalarını önceden tespit
15. **Multi-Sensor Fusion** - Birden fazla sensörden optimal tahmin

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

**Son Güncelleme:** 19 Kasım 2025

**Versiyon:** 3.0 - Modüler Mimari + Kalman Filtresi Entegrasyonu

### Versiyon Geçmişi

**v3.0 (19 Kasım 2025)**
- ✅ Modüler mimari: Sensors, Calculations, KalmanFilter, Communication modülleri
- ✅ 1D Kalman filtresi entegrasyonu (7 sensör için ayrı parametreler)
- ✅ RAW ve FILTERED değerlerin karşılaştırmalı gösterimi
- ✅ LoRa paketlerinde sadece filtrelenmiş değerler gönderimi (54 byte)
- ✅ Bilimsel hesaplamaların ayrı modüle taşınması
- ✅ Kod organizasyonu ve bakım kolaylığı artırıldı

**v2.0 (27 Ekim 2025)**
- ✅ LoRa E32 kablosuz iletişim entegrasyonu
- ✅ Yer istasyonu alıcı sistemi
- ✅ Binary paket transferi + CRC hata kontrolü
- ✅ Sulama güvenlik sistemi (otomatik kapama/geri yükleme)

**v1.0 (Ekim 2025)**
- ✅ Temel sensör okuma (BH1750, BME680, MH-Z14A, Soil)
- ✅ Otomatik/Manuel kontrol modları
- ✅ Servo, röle kontrolleri
- ✅ 9 sera kodu + 8 sulama kodu
