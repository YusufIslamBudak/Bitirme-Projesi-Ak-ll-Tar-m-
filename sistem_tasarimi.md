# Akıllı Sera Sistemi - Sistem Tasarımı

## 📋 Proje Özeti

**İki katmanlı akıllı sera kontrol sistemi:**
1. **Arduino Mega (Veri Toplama):** Sensörlerden veri okur, UART üzerinden NodeMCU'ya JSON gönderir
2. **NodeMCU ESP8266 (Karar & Kontrol):** JSON verilerini parse eder, akıllı karar ağacı ile Arduino'ya komut gönderir

**Sistem Mimarisi:**
- ✅ Arduino Mega: Sensör okuma + Kalman filtreleme + Pasif veri gönderimi
- ✅ NodeMCU: JSON parsing + Karar ağacı + Otomatik/Manuel komut gönderimi
- ✅ Web Kontrol Paneli (NodeMCU üzerinde)
- ✅ LoRa kablosuz veri iletimi (3 km menzil)
- ✅ Firebase & SD Kart veri kayıt

---

## 🎮 Kontrol Modları

### MOD 1: OTOMATİK KONTROL (NodeMCU Karar Ağacı)
NodeMCU, Arduino'dan aldığı JSON sensör verilerini 10 saniyede bir analiz eder ve akıllı kararlar verir.

**Karar Ağacı Özellikleri:**
- 9 Sera Kontrol Kodu (KOD-1 → KOD-9)
- 8 Sulama Kontrol Kodu (SULAMA-1 → SULAMA-8)
- Öncelik tabanlı karar mekanizması
- Tekrar önleme sistemi (30 saniye cooldown)
- Web arayüzünden aç/kapa

### MOD 2: MANUEL KONTROL (Web veya Serial)
NodeMCU web paneli veya Serial Monitor üzerinden manuel komutlar gönderilebilir.

**Manuel Komutlar:**
```
havaac    → Sera kapağını aç (0°) + Fan açık
havakapa  → Sera kapağını kapat (95°) + Fan kapalı
isikac    → Aydınlatmayı aç
isikkapa  → Aydınlatmayı kapat
sulaac    → Sulamayı aç
sulakapa  → Sulamayı kapat
```

**Web Kontrol Paneli:**
- URL: `http://<NodeMCU-IP>/`
- Gerçek zamanlı sensör verileri
- Tek tıkla komut gönderme
- Otomatik kontrol aç/kapa butonu

---

## 📡 Seri İletişim Protokolleri

### 1. USB Seri Port (Arduino ↔ PC)
- **Bağlantı:** Arduino USB (Serial0)
- **Baud Rate:** 115200
- **Amaç:** Debugging, manuel komutlar, sistem izleme

### 2. UART2 Bidirectional (Arduino Mega ↔ NodeMCU)
- **Arduino Tarafı:** Serial2 (TX2/RX2 - Pin 16/17)
- **NodeMCU Tarafı:** SoftwareSerial (D1=RX, D2=TX)
- **Baud Rate:** 9600
- **Amaç:** JSON sensör verisi + Komutlar

#### Arduino → NodeMCU (JSON Sensör Verisi)
**Frekans:** 5 saniye  
**Format:** Compact JSON (newline terminated)

```json
{"temp":25.14,"hum":59.36,"pres":994.74,"gas":165.89,"lux":474.85,"co2":450,"soil":45.48,"dew":16.67,"heat":25.09,"roof":25,"fan":false,"light":false,"pump":false,"uptime":330}
```

**JSON Key Mapping:**
| Key | Açıklama | Veri Tipi | Birim |
|-----|----------|-----------|-------|
| `temp` | Kalman filtreli sıcaklık | float | °C |
| `hum` | Kalman filtreli nem | float | % |
| `pres` | Kalman filtreli basınç | float | hPa |
| `gas` | Kalman filtreli gaz direnci | float | KΩ |
| `lux` | Kalman filtreli ışık şiddeti | float | lux |
| `co2` | Kalman filtreli CO2 | int | ppm |
| `soil` | Kalman filtreli toprak nemi | float | % |
| `dew` | Hesaplanan çiy noktası | float | °C |
| `heat` | Hesaplanan hissedilen sıcaklık | float | °C |
| `roof` | Kapak pozisyonu | int | % (0-100) |
| `fan` | Fan durumu | bool | true/false |
| `light` | Işık durumu | bool | true/false |
| `pump` | Pompa durumu | bool | true/false |
| `uptime` | Sistem çalışma süresi | int | saniye |

**⚠️ Önemli:**
- `lux` key'i ışık sensörü için (float, lux birimi)
- `light` key'i aydınlatma durumu için (boolean)
- Bu ayrım çakışmayı önler!

#### NodeMCU → Arduino (Kontrol Komutları)
**Frekans:** Olay tabanlı (karar ağacı tetiklemesinde)

**Buffer Yönetimi (Arduino tarafında):**
```cpp
void sendToNodeMCU(const char* jsonString) {
  // RX buffer'ı temizle (olası artık veri)
  while (Serial2.available()) {
    Serial2.read();
  }
  
  // JSON'ı gönder
  Serial2.println(jsonString);
  
  // TX buffer boşalana kadar bekle (veri bütünlüğü)
  Serial2.flush();
}
```

**Cooldown Mekanizması (NodeMCU tarafında):**
```cpp
void sendCommandSafe(String cmd) {
  // 30 saniye içinde aynı komut tekrar gönderilmez
  if (cmd == lastCommand && (millis() - lastCommandTime) < 30000) {
    return;
  }
  arduinoSerial.println(cmd);
  lastCommand = cmd;
  lastCommandTime = millis();
}
```

### 3. LoRa Wireless (Arduino → Yer İstasyonu)
- **Modül:** E220-900T22D(JP)
- **Baud Rate:** 9600
- **Menzil:** 3 km (açık alan)
- **Format:** Binary struct (CRC-16 korumalı)
- **Paket Boyutu:** 54 byte

---

## 📁 Dosya Yapısı

```
Tarhun Bitirme Projesi/
├── platformio.ini                # PlatformIO konfigürasyonu
├── README.md                     # Proje açıklaması
├── kosullar.md                   # Kontrol koşulları
├── sistem_tasarimi.md            # Bu dosya
├── NODEMCU_KARAR_AGACI_README.md # Karar ağacı dokümantasyonu
│
├── NodeMCU_Receiver.ino          # NodeMCU karar ağacı kodu
├── YerIstasyonu_Alici.ino        # LoRa alıcı kodu
│
├── src/
│   ├── main.cpp                  # Ana program
│   ├── Sensors.cpp               # Kalman filtreli sensör okuma
│   ├── Calculations.cpp          # Bilimsel hesaplamalar
│   ├── KalmanFilter.cpp          # 1D Kalman filtresi
│   ├── Communication.cpp         # LoRa + UART2 iletişimi
│   ├── JSONFormatter.cpp         # Compact JSON oluşturma
│   └── SerialCommands.cpp        # Komut işleme
│
└── include/
    ├── Sensors.h
    ├── Calculations.h
    ├── KalmanFilter.h
    ├── Communication.h
    ├── JSONFormatter.h
    └── SerialCommands.h
```

---

## 🛡️ Güvenlik Özellikleri

### Arduino Mega (Sensör Katmanı)
1. **Kalman Filtresi:** Sensör gürültüsü azaltma
2. **Servo Titreme Önleme:** Attach/detach pattern
3. **Buffer Yönetimi:** Serial2.flush() ile veri bütünlüğü
4. **MH-Z14A Timeout:** 3 dakika ısınma kontrolü

### NodeMCU (Karar Katmanı)
1. **Cooldown Mekanizması:** 30 saniye tekrar önleme
2. **JSON Parsing Toleransı:** Hata durumunda eski değerler
3. **Öncelik Sistemi:** Kritik > Yüksek > Normal > Optimal
4. **WiFi Auto Reconnect:** Bağlantı kaybında otomatik yeniden bağlanma

---

## 📊 Performans Metrikleri

| Metrik | Arduino Mega | NodeMCU |
|--------|--------------|---------|
| Sensör Okuma | 5 saniye | - |
| JSON Oluşturma | <20ms | - |
| JSON Parsing | - | <50ms |
| Karar Algoritması | - | 10 saniye |
| RAM Kullanımı | 34.6% | ~50% |
| Flash Kullanımı | 16.6% | 7.5% |

---

## 📝 Son Güncelleme

**Tarih:** 29 Kasım 2025  
**Versiyon:** 4.1

### Son Değişiklikler (v4.1)
- ✅ JSON key isimleri düzeltildi (`temperature` → `temp`, `humidity` → `hum`)
- ✅ `light` key çakışması giderildi (`lux` = sensör, `light` = boolean)
- ✅ Buffer yönetimi eklendi (Serial2.flush())
- ✅ RX buffer temizliği eklendi
- ✅ Dokümantasyon güncellendi

### Versiyon Geçmişi
- **v4.0 (27 Kasım 2025):** NodeMCU karar ağacı entegrasyonu
- **v3.0 (19 Kasım 2025):** Modüler mimari + Kalman filtresi
- **v2.0 (27 Ekim 2025):** LoRa kablosuz iletişim
- **v1.0 (Ekim 2025):** Temel sensör okuma + kontrol

---

**Geliştirici:** Yusuf Islam Budak  
**GitHub:** https://github.com/YusufIslamBudak/Bitirme-Projesi-Ak-ll-Tar-m-
