# AkÄ±llÄ± Sera Sistemi - Sistem TasarÄ±mÄ±

## ğŸ“‹ Proje Ã–zeti

**Ä°ki katmanlÄ± akÄ±llÄ± sera kontrol sistemi:**
1. **Arduino Mega (Veri Toplama):** SensÃ¶rlerden veri okur, UART Ã¼zerinden NodeMCU'ya JSON gÃ¶nderir
2. **NodeMCU ESP8266 (Karar & Kontrol):** JSON verilerini parse eder, akÄ±llÄ± karar aÄŸacÄ± ile Arduino'ya komut gÃ¶nderir

**Sistem Mimarisi:**
- âœ… Arduino Mega: SensÃ¶r okuma + Kalman filtreleme + Pasif veri gÃ¶nderimi
- âœ… NodeMCU: JSON parsing + Karar aÄŸacÄ± + Otomatik/Manuel komut gÃ¶nderimi
- âœ… Web Kontrol Paneli (NodeMCU Ã¼zerinde)
- âœ… LoRa kablosuz veri iletimi (3 km menzil)
- âœ… Firebase & SD Kart veri kayÄ±t

---

## ğŸ® Kontrol ModlarÄ±

### MOD 1: OTOMATIK KONTROL (NodeMCU Karar AÄŸacÄ±)
NodeMCU, Arduino'dan aldÄ±ÄŸÄ± JSON sensÃ¶r verilerini 10 saniyede bir analiz eder ve akÄ±llÄ± kararlar verir.

**Karar AÄŸacÄ± Ã–zellikleri:**
- 9 Sera Kontrol Kodu (KOD-1 â†’ KOD-9)
- 8 Sulama Kontrol Kodu (SULAMA-1 â†’ SULAMA-8)
- Ã–ncelik tabanlÄ± karar mekanizmasÄ±
- Tekrar Ã¶nleme sistemi (30 saniye cooldown)
- Web arayÃ¼zÃ¼nden aÃ§/kapa

### MOD 2: MANUEL KONTROL (Web veya Serial)
NodeMCU web paneli veya Serial Monitor Ã¼zerinden manuel komutlar gÃ¶nderilebilir.

**Manuel Komutlar:**
```
havaac    â†’ Sera kapaÄŸÄ±nÄ± aÃ§ (0Â°) + Fan aÃ§Ä±k
havakapa  â†’ Sera kapaÄŸÄ±nÄ± kapat (95Â°) + Fan kapalÄ±
isikac    â†’ AydÄ±nlatmayÄ± aÃ§
isikkapa  â†’ AydÄ±nlatmayÄ± kapat
sulaac    â†’ SulamayÄ± aÃ§
sulakapa  â†’ SulamayÄ± kapat
```

**Web Kontrol Paneli:**
- URL: `http://<NodeMCU-IP>/`
- GerÃ§ek zamanlÄ± sensÃ¶r verileri
- Tek tÄ±kla komut gÃ¶nderme
- Otomatik kontrol aÃ§/kapa butonu

---

## ğŸ”§ DonanÄ±m BileÅŸenleri

### 1. Mikrocontroller

#### Arduino Mega 2560 (SensÃ¶r Sistemi)
- **Rol:** SensÃ¶r okuma, Kalman filtreleme, JSON veri gÃ¶nderimi
- **Ã–zellikler:**
  - 54 dijital I/O pin
  - 16 analog giriÅŸ
  - 4 donanÄ±msal UART
  - I2C desteÄŸi
  - 256KB Flash bellek
- **Ä°letiÅŸim:**
  - UART0 (USB): 115200 baud - Debug ve manuel komutlar
  - UART2 (D16/TX2, D17/RX2): 9600 baud - NodeMCU ile JSON iletiÅŸimi
  - UART1 (D18/TX1, D19/RX1): 9600 baud - MH-Z14A CO2 sensÃ¶rÃ¼
  
#### NodeMCU ESP8266 (Kontrol Sistemi)
- **Rol:** JSON parsing, karar aÄŸacÄ±, Arduino'ya komut gÃ¶nderme, web server
- **Ã–zellikler:**
  - WiFi 802.11 b/g/n
  - 80MHz CPU (160MHz boost)
  - 4MB Flash
  - 80KB RAM
- **Ä°letiÅŸim:**
  - SoftwareSerial (D1/RX=GPIO5, D2/TX=GPIO4): 9600 baud - Arduino ile Ã§ift yÃ¶nlÃ¼
  - WiFi: Web server (Port 80)
  - NTP: Zaman senkronizasyonu
  - SD Kart: CS=D8 (GPIO15) - Veri kayÄ±t

### 2. SensÃ¶rler

#### a) BH1750 (GY-30) - IÅŸÄ±k SensÃ¶rÃ¼
- **Ä°letiÅŸim:** I2C
- **Adres:** 0x23 veya 0x5C
- **Ã–lÃ§Ã¼m AralÄ±ÄŸÄ±:** 1-65535 lux
- **Ã‡Ã¶zÃ¼nÃ¼rlÃ¼k:** 1 lux
- **Pinler:**
  - SDA â†’ D20
  - SCL â†’ D21
  - VCC â†’ 5V
  - GND â†’ GND

#### b) BME680 - Ã‡evre SensÃ¶rÃ¼
- **Ä°letiÅŸim:** I2C
- **Adres:** 0x76 veya 0x77
- **Ã–lÃ§Ã¼mler:**
  - SÄ±caklÄ±k: -40Â°C ~ +85Â°C (Â±1Â°C)
  - Nem: 0% ~ 100% (Â±3%)
  - BasÄ±nÃ§: 300 ~ 1100 hPa (Â±1 hPa)
  - Gaz Direnci: 0 ~ 500 KOhm
- **Pinler:**
  - SDA â†’ D20
  - SCL â†’ D21
  - VCC â†’ 3.3V veya 5V
  - GND â†’ GND

#### c) MH-Z14A - CO2 SensÃ¶rÃ¼
- **Ä°letiÅŸim:** UART (9600 baud)
- **Ã–lÃ§Ã¼m AralÄ±ÄŸÄ±:** 0-5000 ppm
- **DoÄŸruluk:** Â±50 ppm + 5%
- **IsÄ±nma SÃ¼resi:** 3 dakika
- **Pinler:**
  - TX â†’ D19 (RX1)
  - RX â†’ D18 (TX1)
  - VCC â†’ 5V (150mA)
  - GND â†’ GND

#### d) MH Water Sensor - Toprak Nem SensÃ¶rÃ¼
- **Ä°letiÅŸim:** Analog
- **Ã‡Ä±kÄ±ÅŸ:** 0-1023 (ADC)
- **Ã–lÃ§Ã¼m:** Kapasitif toprak nemi
- **Pinler:**
  - A0 â†’ A0 (Analog)
  - VCC â†’ 5V
  - GND â†’ GND
- **Kalibrasyon:**
  - Kuru (Hava): 1023
  - Islak (Su): 300

### 3. Kablosuz Ä°letiÅŸim

#### LoRa E32 ModÃ¼lÃ¼ (Verici)
- **Model:** E32-TTL-100
- **Ä°letiÅŸim:** UART (Software Serial)
- **Frekans:** 433 MHz (veya 868/915 MHz)
- **Menzil:** 3 km (aÃ§Ä±k alan)
- **GÃ¼Ã§:** 100 mW
- **Pinler (Verici):**
  - RX â†’ D10 (Software Serial)
  - TX â†’ D11 (Software Serial)
  - M0 â†’ D6
  - M1 â†’ D7
  - VCC â†’ 5V
  - GND â†’ GND
- **Ã–zellikler:**
  - Binary paket transferi
  - CRC hata kontrolÃ¼
  - Otomatik yeniden gÃ¶nderim
  - DÃ¼ÅŸÃ¼k gÃ¼Ã§ tÃ¼ketimi

#### LoRa E32 ModÃ¼lÃ¼ (AlÄ±cÄ± - Yer Ä°stasyonu)
- **BaÄŸÄ±msÄ±z Arduino sistemi**
- **AynÄ± pin konfigÃ¼rasyonu**
- **Serial Monitor Ã§Ä±ktÄ±sÄ± (9600 baud)**

### 4. AktÃ¼atÃ¶rler

#### a) Servo Motor (Sera KapaÄŸÄ±)
- **Model:** MG995 (Metal diÅŸlili, yÃ¼ksek tork)
- **Kontrol:** PWM
- **AÃ§Ä±:** 0Â° (Tam AÃ§Ä±k) ~ 95Â° (Tam KapalÄ±)
- **Pin:** D9
- **GÃ¼Ã§:** 4.8-7.2V, 2.5A (yÃ¼k altÄ±nda)
- **Tork:** 10 kg-cm
- **Ã–zellikler:**
  - Metal diÅŸliler (dayanÄ±klÄ±)
  - Ã‡ift rulman (hassas)
  - Su geÃ§irmez koruma
  
#### b) HavalandÄ±rma FanÄ± RÃ¶lesi
- **Pin:** D30
- **Kontrol:** Dijital (LOW=AÃ§Ä±k, HIGH=KapalÄ±)
- **Ã–zellikler:**
  - Sera kapaÄŸÄ± >30% aÃ§Ä±kken otomatik aktif
  - Manuel kontrol ile baÄŸÄ±msÄ±z Ã§alÄ±ÅŸtÄ±rÄ±labilir
  
#### c) AydÄ±nlatma RÃ¶lesi
- **Pin:** D29 (Active LOW)
- **Kontrol:** Dijital (LOW=AÃ§Ä±k, HIGH=KapalÄ±)
- **KullanÄ±m:**
  - Otomatik: IÅŸÄ±k < 200 lux â†’ AÃ§Ä±k
  - Manuel: Komut isikac/isikkapa ile kontrol
- **Not:** D7'den D29'a taÅŸÄ±ndÄ± (LoRa M1 pin Ã§akÄ±ÅŸmasÄ± Ã¶nlendi)
  
#### d) Sulama PompasÄ± RÃ¶lesi
- **Pin:** D31 (D10'dan taÅŸÄ±ndÄ± - LoRa Ã§akÄ±ÅŸmasÄ± Ã§Ã¶zÃ¼ldÃ¼)
- **Kontrol:** Dijital (LOW=AÃ§Ä±k, HIGH=KapalÄ±)
- **KullanÄ±m:**
  - Otomatik: Toprak nemi < 40% â†’ 20-30 saniye
  - Manuel: Komut 3/-3 ile kontrol

---

## ğŸ”Œ BaÄŸlantÄ± ÅemasÄ±

```
ARDUINO MEGA 2560 (SENSÃ–R SÄ°STEMÄ° - SADECE VERÄ° TOPLA)
â”‚
â”œâ”€ I2C Bus (D20/SDA, D21/SCL)
â”‚  â”œâ”€ BH1750 (0x23)
â”‚  â””â”€ BME680 (0x76)
â”‚
â”œâ”€ UART0 (USB) - 115200 baud
â”‚  â””â”€ Serial Monitor (Debug + Manuel komut alÄ±mÄ±)
â”‚
â”œâ”€ UART1 (D18/TX1, D19/RX1) - 9600 baud
â”‚  â””â”€ MH-Z14A CO2 Sensor
â”‚
â”œâ”€ UART2 (D16/TX2, D17/RX2) - 9600 baud
â”‚  â””â”€ NodeMCU ESP8266 (Ã‡Ä°FT YÃ–NLÃœ)
â”‚     â”œâ”€ Arduino â†’ NodeMCU: JSON sensÃ¶r verileri (her 5 saniye)
â”‚     â””â”€ NodeMCU â†’ Arduino: Komutlar (havaac, isikac, sulaac vb.)
â”‚
â”œâ”€ Software Serial (D10/RX, D11/TX)
â”‚  â””â”€ LoRa E32 ModÃ¼lÃ¼ (Verici)
â”‚     - M0 â†’ D6
â”‚     - M1 â†’ D8 (LoRa kontrol pini)
â”‚
â”œâ”€ Analog Input
â”‚  â””â”€ A0 â†’ MH Water Sensor
â”‚
â”œâ”€ PWM Output
â”‚  â””â”€ D9 â†’ MG995 Servo Motor (Sera KapaÄŸÄ±)
â”‚
â””â”€ Digital Outputs (AktuatÃ¶rler - MANUEL KONTROL)
   â”œâ”€ D29 â†’ RÃ¶le (IÅŸÄ±k) - Active LOW
   â”œâ”€ D30 â†’ RÃ¶le (Fan) - Active LOW
   â””â”€ D31 â†’ RÃ¶le (Sulama PompasÄ±) - Active LOW

         â†“â†“â†“ UART2 (9600 baud, Ã‡Ä°FT YÃ–NLÃœ) â†“â†“â†“

NODEMCU ESP8266 (KARAR VE KONTROL SÄ°STEMÄ°)
â”‚
â”œâ”€ SoftwareSerial (D1=GPIO5/RX, D2=GPIO4/TX) - 9600 baud
â”‚  â””â”€ Arduino Mega UART2 (Ã‡Ä°FT YÃ–NLÃœ)
â”‚     â”œâ”€ RX: JSON sensÃ¶r verileri alÄ±r
â”‚     â””â”€ TX: Kontrol komutlarÄ± gÃ¶nderir
â”‚
â”œâ”€ WiFi (802.11 b/g/n)
â”‚  â”œâ”€ NTP Sunucu (Zaman senkronizasyonu)
â”‚  â””â”€ Web Server (Port 80)
â”‚     â”œâ”€ Ana Sayfa: Kontrol paneli
â”‚     â”œâ”€ /command: Komut gÃ¶nder
â”‚     â””â”€ /status: Durum sorgula
â”‚
â””â”€ SPI (SD Kart) - CS=D8 (GPIO15)
   â””â”€ SD Kart ModÃ¼lÃ¼
      - sensor_log.txt (JSON + Zaman damgasÄ±)

         â†“â†“â†“ LoRa 433MHz Kablosuz â†“â†“â†“
         
ARDUINO (ALICI - YER Ä°STASYONU)
â”‚
â””â”€ Software Serial (D10/RX, D11/TX)
   â””â”€ LoRa E32 ModÃ¼lÃ¼ (AlÄ±cÄ±)
      - M0 â†’ D6
      - M1 â†’ D8
      - Serial Monitor â†’ USB (9600 baud)
```

---

## ğŸ“Š Veri AkÄ±ÅŸÄ± ve Sistem Mimarisi

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                  ARDUINO MEGA 2560 (VERÄ°CÄ°)                   â”‚
â”‚                   SADECE VERÄ° TOPLAMA                         â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                             â”‚
    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
    â”‚                        â”‚                        â”‚
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”          â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”          â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚SENSÃ–RLERâ”‚          â”‚   KALMAN     â”‚          â”‚  MANUEL  â”‚
â”‚         â”‚â”€â”€READâ”€â”€> â”‚  FÄ°LTRESÄ°    â”‚â”€â”€â”€â”€â”€â”€>   â”‚  KOMUT   â”‚
â”‚ BH1750  â”‚          â”‚              â”‚          â”‚  Ä°ÅLEME  â”‚
â”‚ BME680  â”‚          â”‚ 7 FÄ°LTRE     â”‚          â”‚(USB/UART)â”‚
â”‚ MH-Z14A â”‚          â”‚ PARALEL      â”‚          â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
â”‚ SOIL    â”‚          â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜                 â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜                   â”‚                       â”‚
                              â”‚                       â”‚
                        â”Œâ”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”
                        â”‚    JSON FORMATTER               â”‚
                        â”‚ - SÄ±caklÄ±k (FILTERED)           â”‚
                        â”‚ - Nem (FILTERED)                â”‚
                        â”‚ - CO2 (FILTERED)                â”‚
                        â”‚ - Toprak (FILTERED)             â”‚
                        â”‚ - IÅŸÄ±k (FILTERED)               â”‚
                        â”‚ - Ã‡iy NoktasÄ±                   â”‚
                        â”‚ - Heat Index                    â”‚
                        â”‚ - Sistem DurumlarÄ±              â”‚
                        â””â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                              â”‚
                â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
                â”‚                            â”‚
          â”Œâ”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”            â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”
          â”‚ UART2 TX2  â”‚            â”‚  LoRa E32 TX   â”‚
          â”‚ 9600 baud  â”‚            â”‚   433 MHz      â”‚
          â”‚   JSON     â”‚            â”‚  Binary Packet â”‚
          â””â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”˜            â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                â”‚                            â”‚
                â”‚                            â”‚
                â”‚                      â”Œâ”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”
                â”‚                      â”‚ YER        â”‚
                â”‚                      â”‚ Ä°STASYONU  â”‚
                â”‚                      â”‚ (ALICI)    â”‚
                â”‚                      â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                â”‚
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚            NODEMCU ESP8266 (KARAR SÄ°STEMÄ°)                    â”‚
â”‚              AKÄ±LLÄ± KONTROL MERKEZÄ°                           â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                             â”‚
                    â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
                    â”‚                   â”‚
            â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”   â”Œâ”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”
            â”‚  JSON PARSING  â”‚   â”‚ WEB SERVER â”‚
            â”‚  - SensÃ¶r ver. â”‚   â”‚  Port 80   â”‚
            â”‚  - String iÅŸl. â”‚   â”‚ Kontrol UI â”‚
            â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”˜   â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                    â”‚
            â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
            â”‚    KARAR AÄACI ALGORÄ°TMASI      â”‚
            â”‚                                 â”‚
            â”‚  Her 10 saniyede bir:          â”‚
            â”‚  1. JSON'u parse et            â”‚
            â”‚  2. SensÃ¶r verilerini analiz   â”‚
            â”‚  3. Ã–ncelik tabanlÄ± kararlar   â”‚
            â”‚  4. Komut cooldown kontrolÃ¼    â”‚
            â”‚  5. Arduino'ya komut gÃ¶nder    â”‚
            â”‚                                 â”‚
            â”‚  KRÄ°TÄ°K Ã–NCELÄ°K:               â”‚
            â”‚  â”œâ”€ KOD-7: Donma Riski         â”‚
            â”‚  â””â”€ KOD-8: FÄ±rtÄ±na Riski       â”‚
            â”‚                                 â”‚
            â”‚  YÃœKSEK Ã–NCELÄ°K:               â”‚
            â”‚  â”œâ”€ KOD-1: AÅŸÄ±rÄ± SÄ±cak+Nem     â”‚
            â”‚  â”œâ”€ KOD-2: YÃ¼ksek SÄ±cak+CO2    â”‚
            â”‚  â”œâ”€ KOD-3: YÃ¼ksek CO2          â”‚
            â”‚  â””â”€ KOD-4: KÃ¼f Riski           â”‚
            â”‚                                 â”‚
            â”‚  NORMAL Ã–NCELÄ°K:               â”‚
            â”‚  â”œâ”€ KOD-6: Gece Modu           â”‚
            â”‚  â”œâ”€ KOD-5: GÃ¼ndÃ¼z HavalandÄ±rma â”‚
            â”‚  â””â”€ KOD-9: Optimal Durum       â”‚
            â”‚                                 â”‚
            â”‚  SULAMA KONTROL:               â”‚
            â”‚  â”œâ”€ SULAMA-1: Acil (Toprak<20%)â”‚
            â”‚  â”œâ”€ SULAMA-2: Normal (T<40%)   â”‚
            â”‚  â”œâ”€ SULAMA-3: AkÅŸam (T<50%)    â”‚
            â”‚  â”œâ”€ SULAMA-4: YaÄŸmur Ä°ptali    â”‚
            â”‚  â”œâ”€ SULAMA-5: AÅŸÄ±rÄ± Koruma     â”‚
            â”‚  â””â”€ SULAMA-6: KÃ¼f Riski        â”‚
            â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                    â”‚
            â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
            â”‚ KOMUT GÃ–NDERME   â”‚
            â”‚ - havaac/kapa    â”‚
            â”‚ - isikac/kapa    â”‚
            â”‚ - sulaac/kapa    â”‚
            â”‚ SoftwareSerial TXâ”‚
            â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                    â”‚
            â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
            â”‚  VERI KAYIT              â”‚
            â”‚  â”œâ”€ SD Kart (Local)      â”‚
            â”‚  â”‚  sensor_log.txt       â”‚
            â”‚  â”‚  + Zaman damgasÄ±      â”‚
            â”‚  â”‚                       â”‚
            â”‚  â””â”€ Firebase (Cloud)     â”‚
            â”‚     (Ä°steÄŸe baÄŸlÄ±)       â”‚
            â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                    â”‚
         â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
         â”‚                     â”‚
    â”Œâ”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”        â”Œâ”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”
    â”‚ UART2 RX2â”‚        â”‚   WEB    â”‚
    â”‚ 9600 baudâ”‚        â”‚ KULLANICIâ”‚
    â”‚  KOMUT   â”‚        â”‚  ARAYÃœZÃœ â”‚
    â””â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”˜        â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
         â”‚
         â”‚ Komutlar: havaac, havakapa,
         â”‚ isikac, isikkapa, sulaac, sulakapa
         â”‚
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚               ARDUINO MEGA 2560 (ALICI)                      â”‚
â”‚                  MANUEL KONTROL Ä°ÅLEME                      â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                              â”‚
                 â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
                 â”‚                         â”‚
          â”Œâ”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”         â”Œâ”€â”€â”€â”€â”€â”€â”€â–¼â”€â”€â”€â”€â”€â”€â”€â”€â”
          â”‚   SERVO     â”‚         â”‚    RÃ–LELER     â”‚
          â”‚   MOTOR     â”‚         â”‚                â”‚
          â”‚  MG995 D9   â”‚         â”‚ â”œâ”€ Fan D30     â”‚
          â”‚  0Â°-95Â°     â”‚         â”‚ â”œâ”€ IÅŸÄ±k D29    â”‚
          â”‚             â”‚         â”‚ â””â”€ Pompa D31   â”‚
          â”‚ Titreme Ã–nl.â”‚         â”‚ Active LOW     â”‚
          â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜         â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

### Sistem AkÄ±ÅŸ Ã–zeti

1. **Arduino Mega (Veri KatmanÄ±):**
   - SensÃ¶rleri okur (her 5 saniye)
   - Kalman filtresi uygular (7 sensÃ¶r)
   - JSON formatÄ±nda NodeMCU'ya gÃ¶nderir (UART2 TX2)
   - LoRa ile yer istasyonuna broadcast eder
   - NodeMCU'dan gelen komutlarÄ± iÅŸler (UART2 RX2)
   - AktuatÃ¶rleri kontrol eder (Servo, RÃ¶leler)

2. **NodeMCU ESP8266 (Karar KatmanÄ±):**
   - JSON alÄ±r ve parse eder (SoftwareSerial RX)
   - Karar aÄŸacÄ± algoritmasÄ± Ã§alÄ±ÅŸtÄ±rÄ±r (her 10 saniye)
   - AkÄ±llÄ± kararlar verir (Ã¶ncelik tabanlÄ±)
   - Arduino'ya komut gÃ¶nderir (SoftwareSerial TX)
   - SD Karta ve Firebase'e kaydeder
   - Web arayÃ¼zÃ¼ sunar (WiFi)

3. **Web KullanÄ±cÄ±sÄ±:**
   - GerÃ§ek zamanlÄ± sensÃ¶r verileri gÃ¶rÃ¼r
   - Manuel komutlar gÃ¶nderebilir
   - Otomatik kontrolÃ¼ aÃ§/kapa yapabilir

---

## ğŸ§  YazÄ±lÄ±m Mimarisi

### 1. Arduino Mega - Veri KatmanÄ± (Pasif)

**Sorumluluk:** SensÃ¶r okuma, filtreleme, veri gÃ¶nderimi, komut alma

#### Ana DÃ¶ngÃ¼ (Loop)
```cpp
loop() {
  // Manuel komut iÅŸleme (Serial USB + UART2 NodeMCU'dan)
  processSerialCommand()  // havaac, isikac, sulaac vb.
  
  // Her 5 saniyede bir sensÃ¶r okuma
  if (millis() - lastSensorRead >= 5000) {
    sensors.readAllSensors()  // Kalman filtreli okuma
    sendLoRaData()            // LoRa broadcast
    sendJSONtoNodeMCU()       // UART2 TX â†’ NodeMCU
    lastSensorRead = millis()
  }
}
```

#### JSON Veri FormatÄ± (Arduino â†’ NodeMCU)
```json
{
  "temp": 25.14,
  "hum": 59.36,
  "pres": 994.74,
  "gas": 165.89,
  "lux": 474.85,
  "co2": 450,
  "soil": 45.48,
  "dew": 16.67,
  "heat": 25.09,
  "roof": 25,
  "fan": false,
  "light": false,
  "pump": false,
  "uptime": 330
}
```

#### ModÃ¼ler YapÄ±
- **Sensors.cpp/h:** Kalman filtreli sensÃ¶r okuma
- **KalmanFilter.cpp/h:** 1D Kalman algoritmasÄ±
- **Calculations.cpp/h:** Bilimsel hesaplamalar
- **Communication.cpp/h:** LoRa iletiÅŸimi
- **SerialCommands.cpp/h:** Komut iÅŸleme (havaac, isikac vb.)
- **JSONFormatter.cpp/h:** Compact JSON oluÅŸturma

### 2. NodeMCU ESP8266 - Karar KatmanÄ± (Aktif)

**Sorumluluk:** JSON parsing, karar aÄŸacÄ±, komut gÃ¶nderme, web arayÃ¼zÃ¼

#### Ana DÃ¶ngÃ¼ (Loop)
```cpp
loop() {
  server.handleClient();  // Web isteklerini iÅŸle
  
  // Arduino'dan JSON oku (SoftwareSerial)
  while (arduinoSerial.available()) {
    // JSON topla ve parse et
    parseSensorData(jsonBuffer);
  }
  
  // Otomatik karar aÄŸacÄ± (her 10 saniye)
  if (autoControlEnabled && (millis() - lastDecision >= 10000)) {
    makeDecision();  // AkÄ±llÄ± kontrol algoritmasÄ±
    lastDecision = millis();
  }
  
  // Manuel komutlarÄ± Arduino'ya ilet
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    sendCommandToArduino(cmd);
  }
}
```

#### Karar AÄŸacÄ± AlgoritmasÄ±
```cpp
void makeDecision() {
  // SensÃ¶r verilerini al
  float temp = currentSensors.temperature;
  float hum = currentSensors.humidity;
  int co2 = currentSensors.co2;
  float soil = currentSensors.soilMoisture;
  float lux = currentSensors.lux;
  
  // KRÄ°TÄ°K Ã–NCELÄ°K 1: Donma Riski
  if (temp < 10.0 || dewPoint < 5.0) {
    sendCommandSafe("havakapa");
    sendCommandSafe("sulakapa");
    return; // DiÄŸer kontroller atla
  }
  
  // KRÄ°TÄ°K Ã–NCELÄ°K 2: FÄ±rtÄ±na Riski
  if (pres < 985.0) {
    sendCommandSafe("havakapa");
    sendCommandSafe("sulakapa");
    return;
  }
  
  // YÃœKSEK Ã–NCELÄ°K: AÅŸÄ±rÄ± SÄ±cak + Nem
  if (temp > 32.0 && hum > 70.0) {
    sendCommandSafe("havaac");
    return;
  }
  
  // ... diÄŸer karar kodlarÄ±
  
  // SULAMA KONTROL
  if (soil < 20.0 && temp > 28.0) {
    sendCommandSafe("sulaac");
  }
  
  // OPTIMAL DURUM: Enerji tasarrufu
  if (temp >= 20 && temp <= 26 && hum >= 50 && hum <= 70) {
    // Gereksiz sistemleri kapat
  }
}
```

#### JSON Parsing (Manuel)
```cpp
float parseJsonFloat(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\":");
  int valueStart = keyIndex + key.length() + 3;
  int valueEnd = json.indexOf(',', valueStart);
  return json.substring(valueStart, valueEnd).toFloat();
}
```

#### Komut GÃ¼venliÄŸi
```cpp
void sendCommandSafe(String cmd, String& lastCmd, unsigned long& lastTime) {
  // 30 saniye cooldown - AynÄ± komut tekrar gÃ¶nderilmez
  if (cmd == lastCmd && (millis() - lastTime) < 30000) {
    return;
  }
  sendCommandToArduino(cmd);
  lastCmd = cmd;
  lastTime = millis();
}
```

#### Web Server Endpoints
```cpp
server.on("/", handleRoot);              // Ana sayfa (Kontrol paneli)
server.on("/command", handleCommand);    // Komut gÃ¶nder (?cmd=havaac)
server.on("/status", handleStatus);      // Durum sorgula (JSON)
```

### 3. Web Kontrol Paneli

**URL:** `http://<NodeMCU-IP>/`

**Ã–zellikler:**
- GerÃ§ek zamanlÄ± sensÃ¶r verileri (3 saniye refresh)
- 6 kontrol butonu (Hava aÃ§/kapat, IÅŸÄ±k aÃ§/kapat, Sulama aÃ§/kapat)
- Otomatik kontrol aÃ§/kapa butonu
- Sistem durumu (IP, RSSI, Uptime)
- Responsive tasarÄ±m (mobil uyumlu)

**JavaScript AJAX:**
```javascript
// Komut gÃ¶nder
function cmd(c) {
  fetch('/command?cmd=' + c)
    .then(r => r.text())
    .then(d => alert(d));
}

// Otomatik kontrol toggle
function toggleAuto() {
  fetch('/command?cmd=toggleauto')
    .then(r => r.text())
    .then(d => location.reload());
}

// Durum gÃ¼ncelle (3 saniye)
setInterval(updateStatus, 3000);
```

---

## ğŸ“ˆ Bilimsel Hesaplamalar

### 1. Ã‡iy NoktasÄ± (Dew Point)
**FormÃ¼l:** Magnus-Tetens
```
Td = (b Ã— Î±) / (a - Î±)
Î± = (aÃ—T)/(b+T) + ln(RH/100)
```
**KullanÄ±m:** KÃ¼f riski tespiti

### 2. Hissedilen SÄ±caklÄ±k (Heat Index)
**FormÃ¼l:** Rothfusz (NOAA)
```
HI = -42.379 + 2.049T + 10.143RH - 0.225TÃ—RH + ...
```
**KullanÄ±m:** Bitki stres tespiti

### 3. Mutlak Nem (Absolute Humidity)
**FormÃ¼l:** Termodinamik
```
AH = (e Ã— 2.1674) / (T + 273.15)
```
**KullanÄ±m:** BuharlaÅŸma hesabÄ±

### 4. CO2 Konsantrasyonu
**FormÃ¼l:** Ä°deal Gaz YasasÄ±
```
C(mg/mÂ³) = (ppm Ã— M Ã— P) / (R Ã— T)
```
**KullanÄ±m:** HavalandÄ±rma hesabÄ±

### 5. Deniz Seviyesi BasÄ±ncÄ±
**FormÃ¼l:** Barometrik
```
P0 = P Ã— exp((g Ã— M Ã— h) / (R Ã— T))
```
**KullanÄ±m:** Hava durumu tahmini

---

## ğŸ¯ Kontrol AlgoritmalarÄ±

### Sera Kapak Kontrol KodlarÄ±

| Kod | Ã–ncelik | KoÅŸul | Kapak | AÃ§Ä±klama |
|-----|---------|-------|-------|----------|
| KOD-7 | 1 | SÄ±caklÄ±k < 10Â°C | 0% | Donma riski |
| KOD-1 | 2 | SÄ±caklÄ±k > 32Â°C + Nem > 70% | 100% | AÅŸÄ±rÄ± sÄ±cak |
| KOD-8 | 3 | BasÄ±nÃ§ < 985 hPa | 0% | FÄ±rtÄ±na |
| KOD-2 | 4 | SÄ±caklÄ±k > 28Â°C + CO2 > 800 | 75% | YÃ¼ksek sÄ±caklÄ±k |
| KOD-3 | 5 | CO2 > 1500 ppm | 50% | YÃ¼ksek CO2 |
| KOD-4 | 6 | Nem > 85% | 40% | KÃ¼f riski |
| KOD-6 | 7 | Gece + SÄ±caklÄ±k < 18Â°C | 0% | Gece koruma |
| KOD-5 | 8 | GÃ¼ndÃ¼z + Normal sÄ±caklÄ±k | 25% | HavalandÄ±rma |
| KOD-9 | 9 | Ä°deal koÅŸullar | 0% | Stabil sistem |

### Sulama Kontrol KodlarÄ±

| Kod | Ã–ncelik | KoÅŸul | Pompa | SÃ¼re | AÃ§Ä±klama |
|-----|---------|-------|-------|------|----------|
| SULAMA-5 | 1 | Toprak > 90% | KAPAT | 24h | AÅŸÄ±rÄ± sulama |
| SULAMA-7 | 2 | Gece + SoÄŸuk | KAPAT | - | Gece yasaÄŸÄ± |
| SULAMA-4 | 3 | YaÄŸmur | KAPAT | 30dk | DoÄŸal yaÄŸÄ±ÅŸ |
| SULAMA-6 | 4 | Nem yÃ¼ksek | KAPAT | - | KÃ¼f riski |
| SULAMA-1 | 5 | Toprak < 20% + SÄ±cak | AÃ‡IK | 30s | Acil |
| SULAMA-3 | 6 | AkÅŸam + Kuru | AÃ‡IK | 25s | Optimal |
| SULAMA-2 | 7 | GÃ¼ndÃ¼z + Kuru | AÃ‡IK | 20s | Normal |
| SULAMA-8 | 8 | Toprak 50-70% | KAPAT | - | Ä°deal |

---

## ğŸ“¡ Seri Ä°letiÅŸim Protokolleri

### 1. USB Seri Port (Arduino â†” PC)
- **BaÄŸlantÄ±:** Arduino USB (Serial0)
- **Baud Rate:** 115200
- **AmaÃ§:** Debugging, manuel komutlar, sistem izleme
- **Veri FormatÄ±:** Human-readable text

**Ã‡Ä±ktÄ± FormatÄ±:**
```
--- New Reading ---

--- GY-30 (BH1750) Light Sensor ---
Light Level: 475.00 lux (RAW) | 474.85 lux (FILTERED)
  -> Bright

--- BME680 Air Quality Sensor ---
Temperature: 25.16 C (RAW) | 25.14 C (FILTERED)
Humidity: 59.38 % (RAW) | 59.36 % (FILTERED)
Pressure: 994.75 hPa (RAW) | 994.74 hPa (FILTERED)
Gas Resistance: 165.22 KOhm (RAW) | 165.89 KOhm (FILTERED)

--- MH-Z14A CO2 Sensor ---
CO2 Level: 450 ppm (RAW) | 450 ppm (FILTERED)

--- MH Water Soil Moisture Sensor ---
Soil Moisture: 45.50 % (RAW) | 45.48 % (FILTERED)
  -> NORMAL (Sulama gerekli)

>>> LORA VERI GONDERIMI <<<
Paket Boyutu: 54 byte (Kalman filtreli veriler)
CRC: 0x1A2B
[LORA] *** PAKET BASARIYLA GONDERILDI ***
```

**Manuel Komutlar:**
```
havaac     â†’ Kapak aÃ§ + Fan Ã§alÄ±ÅŸtÄ±r
havakapa   â†’ Kapak kapat + Fan durdur
isikac     â†’ LED/Lamba aÃ§
isikkapa   â†’ LED/Lamba kapat
sulaac     â†’ Sulama aÃ§
sulakapa   â†’ Sulama kapat
```

### 2. UART2 Bidirectional (Arduino Mega â†” NodeMCU)
- **Arduino TarafÄ±:** Serial2 (TX2/RX2 - Pin 16/17)
- **NodeMCU TarafÄ±:** SoftwareSerial (D1=RX, D2=TX)
- **Baud Rate:** 9600
- **AmaÃ§:** JSON sensÃ¶r verisi (Arduino â†’ NodeMCU) + Komutlar (NodeMCU â†’ Arduino)
- **Veri FormatÄ±:** Compact JSON + Text Commands

#### Arduino â†’ NodeMCU (JSON SensÃ¶r Verisi)
**Frekans:** 5 saniye  
**Format:** Compact JSON (newline terminated)

```json
{"temp":25.14,"hum":59.36,"pres":994.74,"gas":165.89,"lux":474.85,"co2":450,"soil":45.48,"dew":16.67,"heat":25.09,"roof":25,"fan":false,"light":false,"pump":false,"uptime":330}
```

**JSON Key Mapping:**
| Key | AÃ§Ä±klama | Veri Tipi | Birim |
|-----|----------|-----------|-------|
| `temp` | Kalman filtreli sÄ±caklÄ±k | float | Â°C |
| `hum` | Kalman filtreli nem | float | % |
| `pres` | Kalman filtreli basÄ±nÃ§ | float | hPa |
| `gas` | Kalman filtreli gaz direnci | float | KÎ© |
| `lux` | Kalman filtreli Ä±ÅŸÄ±k | float | lux |
| `co2` | Kalman filtreli CO2 | int | ppm |
| `soil` | Kalman filtreli toprak nemi | float | % |
| `dew` | Hesaplanan Ã§iy noktasÄ± | float | Â°C |
| `heat` | Hesaplanan hissedilen sÄ±caklÄ±k | float | Â°C |
| `roof` | Kapak pozisyonu | int | % (0-100) |
| `fan` | Fan durumu | bool | true/false |
| `light` | IÅŸÄ±k durumu | bool | true/false |
| `pump` | Pompa durumu | bool | true/false |
| `uptime` | Sistem Ã§alÄ±ÅŸma sÃ¼resi | int | saniye |

**GÃ¼venilirlik:**
- Kalman filtreli temiz veriler
- Compact format (~250 byte)
- Newline delimiter
- 5 saniyelik periyodik gÃ¶nderim

#### NodeMCU â†’ Arduino (Kontrol KomutlarÄ±)
**Frekans:** Olay tabanlÄ± (karar aÄŸacÄ± tetiklemesinde)  
**Format:** Newline-terminated text commands

**Komut Seti:**
```
havaac     â†’ Sera kapaÄŸÄ±nÄ± aÃ§ + Fan Ã§alÄ±ÅŸtÄ±r
havakapa   â†’ Sera kapaÄŸÄ±nÄ± kapat + Fan durdur
isikac     â†’ AydÄ±nlatma aÃ§
isikkapa   â†’ AydÄ±nlatma kapat
sulaac     â†’ Sulama pompasÄ±nÄ± aÃ§
sulakapa   â†’ Sulama pompasÄ±nÄ± kapat
```

**Komut Ä°ÅŸleme (Arduino tarafÄ±nda):**
```cpp
void processSerialCommand() {
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    // Komut iÅŸle
    if (cmd == "havaac") {
      // Servo aÃ§, fan Ã§alÄ±ÅŸtÄ±r
    } else if (cmd == "sulaac") {
      // Pompa aÃ§ (gÃ¼venlik: diÄŸer sistemleri kapat)
    }
  }
}
```

**Cooldown MekanizmasÄ± (NodeMCU tarafÄ±nda):**
```cpp
void sendCommandSafe(String cmd) {
  // 30 saniye iÃ§inde aynÄ± komut tekrar gÃ¶nderilmez
  if (cmd == lastCommand && (millis() - lastCommandTime) < 30000) {
    return;
  }
  arduinoSerial.println(cmd);
  lastCommand = cmd;
  lastCommandTime = millis();
}
```

### 3. LoRa Wireless (Arduino â†’ Yer Ä°stasyonu)
- **BaÄŸlantÄ±:** E220-900T22D(JP) modÃ¼lÃ¼ (Serial1 - TX1/RX1)
- **Baud Rate:** 9600
- **Frekans BandÄ±:** 868 MHz (TÃ¼rkiye iÃ§in 900 MHz ayarlandÄ±)
- **Menzil:** 3 km (aÃ§Ä±k alan)
- **AmaÃ§:** Uzak mesafe sensÃ¶r verisi broadcast
- **Veri FormatÄ±:** Binary struct (CRC-16 korumalÄ±)

**Binary Paket YapÄ±sÄ± (54 byte):**
```cpp
#pragma pack(push,1)
struct SensorDataPacket {
  // BME680 (16 byte)
  float temperature;         // Â°C
  float humidity;            // %
  float pressure;            // hPa
  float gas_resistance;      // KÎ©
  
  // BH1750 (4 byte)
  float lux;                 // lux
  
  // MH-Z14A (3 byte)
  uint16_t co2_ppm;          // ppm
  int8_t co2_temperature;    // Â°C
  
  // Soil Moisture (6 byte)
  float soil_moisture_percent;  // %
  uint16_t soil_moisture_raw;   // ADC
  
  // Control States (6 byte)
  uint8_t roof_position;        // 0-100%
  uint8_t fan_state;            // 0/1
  uint8_t light_state;          // 0/1
  uint8_t pump_state;           // 0/1
  uint16_t irrigation_duration; // s
  
  // Calculations (12 byte)
  float dew_point;              // Â°C
  float heat_index;             // Â°C
  float absolute_humidity;      // g/mÂ³
  
  // System (5 byte)
  uint32_t uptime;              // s
  uint8_t mhz14a_ready;         // 0/1
  
  // Integrity (2 byte)
  uint16_t crc;
};
#pragma pack(pop)
```

**CRC-16 DoÄŸrulama:**
```cpp
uint16_t calculateCRC16(uint8_t* data, uint16_t length) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
```

### 4. Ä°letiÅŸim ProtokolÃ¼ KarÅŸÄ±laÅŸtÄ±rmasÄ±

| Protokol | Baud | Format | Boyut | Frekans | Menzil | GÃ¼venilirlik |
|----------|------|--------|-------|---------|--------|--------------|
| USB Serial | 115200 | Text | ~500B | 5s | 5m | YÃ¼ksek (kablolu) |
| UART2 (TX) | 9600 | JSON | ~250B | 5s | 30cm | Ã‡ok YÃ¼ksek (kablolu) |
| UART2 (RX) | 9600 | Text | ~10B | Olay | 30cm | Ã‡ok YÃ¼ksek (kablolu) |
| LoRa | 9600 | Binary | 54B | 5s | 3km | Orta (CRC korumalÄ±) |

### 5. Hata YÃ¶netimi ve Tolerans

**UART2 JSON Parsing HatalarÄ±:**
- NodeMCU JSON parsing hatalarÄ±nda eski sensÃ¶r deÄŸerlerini kullanÄ±r
- 60 saniye boyunca veri gelmezse "BaÄŸlantÄ± KaybÄ±" uyarÄ±sÄ±
- Timeout durumunda karar aÄŸacÄ± Ã§alÄ±ÅŸmaz (gÃ¼venlik)

**LoRa Paket HatalarÄ±:**
- CRC kontrolÃ¼ baÅŸarÄ±sÄ±zsa paket atÄ±lÄ±r
- Yer istasyonu son geÃ§erli paketi kullanÄ±r
- 30 saniye boyunca paket gelmezse "Sinyal KaybÄ±" uyarÄ±sÄ±

**Komut Ä°letimi HatalarÄ±:**
- NodeMCU 30 saniye cooldown ile gereksiz komut tekrarÄ±nÄ± Ã¶nler
- Arduino komut iÅŸleme baÅŸarÄ±sÄ±z olursa sonraki dÃ¶ngÃ¼de tekrar dener
- GÃ¼venlik kritiÄŸi: Sulama komutlarÄ±nda tÃ¼m sistemler kapatÄ±lÄ±r

---

## ğŸ”‹ GÃ¼Ã§ TÃ¼ketimi

### Arduino Mega Verici Sistem (SensÃ¶r KatmanÄ±)
| BileÅŸen | AkÄ±m | GÃ¼Ã§ | AÃ§Ä±klama |
|---------|------|-----|----------|
| Arduino Mega | ~50mA | 0.25W | Ana iÅŸlemci |
| BH1750 | ~0.2mA | 0.001W | IÅŸÄ±k sensÃ¶rÃ¼ |
| BME680 | ~3.7mA | 0.018W | Hava kalitesi sensÃ¶rÃ¼ |
| MH-Z14A | ~150mA | 0.75W | CO2 sensÃ¶rÃ¼ (en yÃ¼ksek tÃ¼ketim) |
| Soil Sensor | ~20mA | 0.1W | Toprak nem sensÃ¶rÃ¼ |
| LoRa E32 TX | ~120mA | 0.6W | Uzak mesafe iletiÅŸim |
| Servo (SG90) | ~100-500mA | 0.5-2.5W | Sera kapaÄŸÄ± (sadece harekette) |
| RÃ¶le ModÃ¼lÃ¼ | ~50mA | 0.25W | Fan + IÅŸÄ±k + Pompa kontrol |
| Sulama PompasÄ± | ~200-500mA | 1-2.5W | Sadece sulama sÄ±rasÄ±nda |
| **TOPLAM (Normal)** | **~400mA** | **~2W** | SensÃ¶r okuma + LoRa TX |
| **TOPLAM (Peak)** | **~1000mA** | **~5W** | Servo + Pompa aktif |

### NodeMCU ESP8266 (Karar KatmanÄ±)
| BileÅŸen | AkÄ±m | GÃ¼Ã§ | AÃ§Ä±klama |
|---------|------|-----|----------|
| ESP8266 (Aktif) | ~80mA | 0.4W | WiFi + Karar algoritmasÄ± |
| ESP8266 (WiFi TX) | ~170mA | 0.85W | Web server isteÄŸi sÄ±rasÄ±nda |
| SD Card ModÃ¼lÃ¼ | ~30mA | 0.15W | Veri loglama |
| **TOPLAM (Normal)** | **~110mA** | **~0.55W** | JSON parsing + karar |
| **TOPLAM (Peak)** | **~200mA** | **~1W** | Web + SD yazma |

### AlÄ±cÄ± Sistem (Yer Ä°stasyonu)
| BileÅŸen | AkÄ±m | GÃ¼Ã§ |
|---------|------|-----|
| Arduino | ~50mA | 0.25W |
| LoRa E32 RX | ~20mA | 0.1W |
| **TOPLAM** | **~70mA** | **~0.35W** |

### GÃ¼Ã§ KaynaÄŸÄ± Ã–nerileri
- **Arduino Mega Sistemi:** 5V 3A adaptÃ¶r (gÃ¼venlik marjÄ± ile)
- **NodeMCU Sistemi:** 5V 1A adaptÃ¶r (USB gÃ¼Ã§ yeterli)
- **Yer Ä°stasyonu:** 5V 500mA adaptÃ¶r veya USB gÃ¼Ã§

### Enerji Tasarrufu Stratejileri
âœ… **Servo Detach:** Servo motor sadece harekette gÃ¼Ã§ alÄ±r (titreme Ã¶nleme)  
âœ… **Kalman Filtresi:** Gereksiz rÃ¶le/servo tetiklenmeleri Ã¶nlenir  
âœ… **30s Cooldown:** NodeMCU aynÄ± komutu tekrar gÃ¶ndermez  
âœ… **Optimal Durum Tespiti:** Ä°deal koÅŸullarda sistemler kapatÄ±lÄ±r  
âœ… **WiFi Sleep Mode:** NodeMCU idle durumda gÃ¼Ã§ tasarrufu (gelecek Ã¶zellik)

## ğŸ“ Dosya YapÄ±sÄ± (ModÃ¼ler Mimari)

```
Tarhun Bitirme Projesi/
â”‚
â”œâ”€â”€ platformio.ini                # PlatformIO konfigÃ¼rasyonu
â”œâ”€â”€ README.md                     # Proje aÃ§Ä±klamasÄ±
â”œâ”€â”€ kosullar.md                   # Kontrol koÅŸullarÄ± (NodeMCU karar aÄŸacÄ± kodlarÄ±)
â”œâ”€â”€ sistem_tasarimi.md            # Sistem tasarÄ±m dokÃ¼mantasyonu (gÃ¼ncel)
â”œâ”€â”€ NODEMCU_KARAR_AGACI_README.md # NodeMCU karar aÄŸacÄ± detaylÄ± dokÃ¼mantasyonu
â”‚
â”œâ”€â”€ NodeMCU_Receiver.ino          # NodeMCU karar aÄŸacÄ± kodu (Arduino IDE)
â”œâ”€â”€ YerIstasyonu_Alici.ino        # LoRa alÄ±cÄ± kodu (Arduino IDE)
â”‚
â”œâ”€â”€ src/                          # Arduino Mega kaynak kodlar (SensÃ¶r KatmanÄ±)
â”‚   â”œâ”€â”€ main.cpp                  # Ana program (sensÃ¶r okuma + JSON gÃ¶nderim)
â”‚   â”œâ”€â”€ Sensors.cpp               # SensÃ¶r okuma + Kalman filtreleme
â”‚   â”œâ”€â”€ Calculations.cpp          # Bilimsel hesaplamalar
â”‚   â”œâ”€â”€ KalmanFilter.cpp          # 1D Kalman filtresi algoritmasÄ±
â”‚   â”œâ”€â”€ Communication.cpp         # LoRa iletiÅŸimi
â”‚   â”œâ”€â”€ JSONFormatter.cpp         # Compact JSON oluÅŸturma (UART2)
â”‚   â””â”€â”€ SerialCommands.cpp        # UART komut iÅŸleme (NodeMCU'dan gelen)
â”‚
â”œâ”€â”€ include/                      # Header dosyalarÄ± (Interface)
â”‚   â”œâ”€â”€ Sensors.h                 # SensÃ¶r modÃ¼lÃ¼ arayÃ¼zÃ¼
â”‚   â”œâ”€â”€ Calculations.h            # Hesaplamalar arayÃ¼zÃ¼
â”‚   â”œâ”€â”€ KalmanFilter.h            # Kalman filtresi arayÃ¼zÃ¼
â”‚   â”œâ”€â”€ Communication.h           # LoRa iletiÅŸim arayÃ¼zÃ¼
â”‚   â”œâ”€â”€ JSONFormatter.h           # JSON formatter arayÃ¼zÃ¼
â”‚   â””â”€â”€ SerialCommands.h          # Komut iÅŸleme arayÃ¼zÃ¼
â”‚
â””â”€â”€ lib/                          # KÃ¼tÃ¼phaneler
    â””â”€â”€ README                    # KÃ¼tÃ¼phane aÃ§Ä±klamasÄ±
```

### ModÃ¼l DetaylarÄ± - Arduino Mega (SensÃ¶r KatmanÄ±)

#### 1. main.cpp (Pasif Veri Toplama)
- **AmaÃ§:** SensÃ¶r okuma, JSON gÃ¶nderme, komut iÅŸleme
- **Ana DÃ¶ngÃ¼:**
  - Her 5 saniyede sensÃ¶r okuma (Kalman filtreli)
  - JSON formatla ve Serial2'ye gÃ¶nder (NodeMCU)
  - LoRa broadcast yap (Yer Ä°stasyonu)
  - USB Serial debug Ã§Ä±ktÄ±sÄ± (PC)
  - UART2'den gelen komutlarÄ± iÅŸle (NodeMCU â†’ Arduino)
- **Otomatik Kontrol:** YOK (kaldÄ±rÄ±ldÄ±)
- **Manuel Kontrol:** UART2 + USB Serial komutlarÄ±

#### 2. Sensors.cpp (Kalman Filtreli Okuma)
- **AmaÃ§:** 7 sensÃ¶rÃ¼ Kalman filtresi ile oku
- **SensÃ¶rler:** BH1750, BME680, MH-Z14A, Soil Moisture
- **Ã‡Ä±ktÄ±:** RAW ve FILTERED deÄŸerler
- **Kalman Parametreleri:** Her sensÃ¶r iÃ§in optimize edilmiÅŸ

#### 3. JSONFormatter.cpp (Compact JSON)
- **AmaÃ§:** SensÃ¶r verilerini NodeMCU iÃ§in JSON formatla
- **Format:** `{"temp":25.14,"hum":59.36,...}`
- **Boyut:** ~250 byte (kompakt)
- **Frekans:** 5 saniye

#### 4. SerialCommands.cpp (Komut Ä°ÅŸleme)
- **AmaÃ§:** NodeMCU'dan gelen komutlarÄ± iÅŸle
- **Komutlar:** havaac, havakapa, isikac, isikkapa, sulaac, sulakapa
- **GÃ¼venlik:** Sulama sÄ±rasÄ±nda diÄŸer sistemleri kapat

### NodeMCU ESP8266 (Karar KatmanÄ±)

#### NodeMCU_Receiver.ino (AkÄ±llÄ± Kontrol Sistemi - ~600 satÄ±r)
- **AmaÃ§:** JSON parsing + karar aÄŸacÄ± + komut gÃ¶nderme + web server
- **Ana Fonksiyonlar:**
  - `loop()` - Ana dÃ¶ngÃ¼ (web server + JSON okuma + karar algoritmasÄ±)
  - `parseSensorData()` - Manuel JSON parsing (ArduinoJson kullanmadan)
  - `makeDecision()` - 17 kodlu karar aÄŸacÄ± (KOD-1 ~ KOD-9, SULAMA-1 ~ SULAMA-8)
  - `sendCommandSafe()` - 30 saniye cooldown ile komut gÃ¶nder
  - `handleRoot()` - Web kontrol paneli
  - `handleCommand()` - Web komut iÅŸleme
  - `handleStatus()` - Durum sorgulama API

**Karar AÄŸacÄ± Mimarisi:**
```cpp
// Ã–ncelik seviyeleri
1. KRÄ°TÄ°K (Donma, FÄ±rtÄ±na) - AnÄ±nda mÃ¼dahale
2. YÃœKSEK (AÅŸÄ±rÄ± sÄ±cak+nem) - Ã–ncelikli
3. NORMAL (HavalandÄ±rma, Ä±ÅŸÄ±k) - Rutin
4. OPTIMAL (Enerji tasarrufu) - Ä°deal durum

// Karar dÃ¶ngÃ¼sÃ¼
JSON al â†’ Parse et â†’ Ã–ncelik belirle â†’ Komut gÃ¶nder â†’ Cooldown
```

**Web Kontrol Paneli:**
- Real-time sensÃ¶r gÃ¶rÃ¼ntÃ¼leme (3s refresh)
- 6 kontrol butonu (Hava, IÅŸÄ±k, Sulama)
- Otomatik/Manuel toggle
- IP, RSSI, Uptime bilgisi
- Responsive mobil uyumlu tasarÄ±m

---

## ğŸ“š KullanÄ±lan KÃ¼tÃ¼phaneler

### Arduino Mega - Verici Sistem (PlatformIO)
```ini
[env:megaatmega2560]
platform = atmelavr
board = megaatmega2560
framework = arduino

lib_deps = 
    claws/BH1750@^1.3.0                           # BH1750 Ä±ÅŸÄ±k sensÃ¶rÃ¼
    adafruit/Adafruit BME680 Library@^2.0.4      # BME680 hava kalitesi
    adafruit/Adafruit Unified Sensor@^1.1.14     # Unified Sensor API
    xreef/EByte LoRa E32 library@^1.5.10         # LoRa E220 modÃ¼lÃ¼
```

**Ã–zel KÃ¼tÃ¼phaneler:**
- **KalmanFilter.cpp/h** - Proje iÃ§i geliÅŸtirilen 1D Kalman filtresi
- **Sensors.cpp/h** - ModÃ¼ler sensÃ¶r okuma sistemi
- **JSONFormatter.cpp/h** - Compact JSON oluÅŸturucu
- **SerialCommands.cpp/h** - UART komut iÅŸleyici

### NodeMCU ESP8266 (Arduino IDE)
**Gerekli KÃ¼tÃ¼phaneler (Arduino Library Manager):**
- `ESP8266WiFi` (Core ile gelir)
- `ESP8266WebServer` (Core ile gelir)
- `SoftwareSerial` (Core ile gelir)
- `SD` (Core ile gelir)

**ESP8266 Board Package:**
```
Board Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
Board: NodeMCU 1.0 (ESP-12E Module)
CPU Frequency: 80 MHz
Flash Size: 4M (3M SPIFFS)
Upload Speed: 115200
```

**Harici KÃ¼tÃ¼phane Gerekmez:**
- JSON parsing manuel olarak yapÄ±lÄ±r (ArduinoJson kullanÄ±lmaz)
- TÃ¼m gerekli fonksiyonlar kodda mevcut

### Yer Ä°stasyonu AlÄ±cÄ± (Arduino IDE)
**Gerekli KÃ¼tÃ¼phaneler:**
- `EByte LoRa E32 library` (Library Manager'dan)

---

## ğŸš€ Kurulum ve KullanÄ±m

### 1. DonanÄ±m MontajÄ±

#### Arduino Mega (SensÃ¶r KatmanÄ±)
1. **SensÃ¶rler:**
   - BH1750 â†’ I2C (SDA/SCL)
   - BME680 â†’ I2C (SDA/SCL)
   - MH-Z14A â†’ Serial3 (TX3/RX3)
   - Soil Moisture â†’ A0
2. **NodeMCU UART BaÄŸlantÄ±sÄ±:**
   - Arduino TX2 (Pin 16) â†’ NodeMCU D1 (RX)
   - Arduino RX2 (Pin 17) â†’ NodeMCU D2 (TX)
   - GND â†’ GND (ortak topraklama)
3. **AktÃ¼atÃ¶rler:**
   - Servo motor â†’ D9 (harici 5V gÃ¼Ã§)
   - Fan rÃ¶lesi â†’ D30 (Active LOW)
   - IÅŸÄ±k rÃ¶lesi â†’ D29 (Active LOW)
   - Pompa rÃ¶lesi â†’ D31 (Active LOW)
4. **LoRa ModÃ¼lÃ¼:**
   - E220-900T22D â†’ Serial1 (TX1/RX1)
   - M0 â†’ D6, M1 â†’ D7, AUX â†’ D10
5. **GÃ¼Ã§:** 5V 3A adaptÃ¶r

#### NodeMCU ESP8266 (Karar KatmanÄ±)
1. **Arduino UART BaÄŸlantÄ±sÄ±:**
   - NodeMCU D1 (RX) â†’ Arduino TX2
   - NodeMCU D2 (TX) â†’ Arduino RX2
   - GND â†’ GND
2. **SD Kart ModÃ¼lÃ¼ (Opsiyonel):**
   - CS â†’ D8
   - MOSI â†’ D7
   - MISO â†’ D6
   - SCK â†’ D5
3. **WiFi:** AP adÄ±nÄ± ve ÅŸifresini kodda ayarlayÄ±n
4. **GÃ¼Ã§:** 5V 1A adaptÃ¶r veya USB

#### Yer Ä°stasyonu
1. Arduino + LoRa E220 modÃ¼lÃ¼ (aynÄ± pin konfigÃ¼rasyonu)
2. USB ile PC baÄŸlantÄ±sÄ±
3. Serial Monitor (9600 baud)

### 2. YazÄ±lÄ±m YÃ¼kleme

#### Arduino Mega (PlatformIO)
```bash
cd "I:\Drive'Ä±m\Bitirme Tezi\CODÄ°NG\Tarhun Bitirme Projesi"
pio run --target upload
```

#### NodeMCU ESP8266 (Arduino IDE)
1. Arduino IDE â†’ Board Manager â†’ ESP8266 kurulumu
2. `NodeMCU_Receiver.ino` dosyasÄ±nÄ± aÃ§
3. **WiFi ayarlarÄ± dÃ¼zenle:**
   ```cpp
   const char* ssid = "SeraKontrol";
   const char* password = "12345678";
   ```
4. Board: NodeMCU 1.0, Upload Speed: 115200
5. Upload

#### Yer Ä°stasyonu (Arduino IDE)
1. `YerIstasyonu_Alici.ino` dosyasÄ±nÄ± aÃ§
2. EByte LoRa E32 kÃ¼tÃ¼phanesini kur
3. Upload

### 3. Ä°lk Ã‡alÄ±ÅŸtÄ±rma

#### Arduino Mega
1. USB Serial Monitor aÃ§ (115200 baud)
2. MH-Z14A 3 dakika Ä±sÄ±nma bekle
3. **RAW vs FILTERED** deÄŸerleri karÅŸÄ±laÅŸtÄ±r
4. JSON Ã§Ä±ktÄ±sÄ±nÄ± kontrol et (Serial2'ye gidiyor)
5. LoRa broadcast'i gÃ¶zlemle

**Beklenen Ã‡Ä±ktÄ±:**
```
--- New Reading ---
--- GY-30 (BH1750) Light Sensor ---
Light Level: 475.00 lux (RAW) | 474.85 lux (FILTERED)

{"temp":25.14,"hum":59.36,"pres":994.74,...}
```

#### NodeMCU
1. USB Serial Monitor aÃ§ (115200 baud)
2. WiFi baÄŸlantÄ±sÄ±nÄ± kontrol et
3. IP adresini not al (`http://192.168.x.x`)
4. Arduino'dan gelen JSON verisini gÃ¶zlemle
5. Karar aÄŸacÄ± Ã§Ä±ktÄ±larÄ±nÄ± izle

**Beklenen Ã‡Ä±ktÄ±:**
```
WiFi connected: 192.168.1.100
JSON received: {"temp":25.14,"hum":59.36,...}
[Karar] KOD-5: Hafif havalandÄ±rma (Temp: 25.14C)
Komut gÃ¶nderildi: havaac
```

#### Web Kontrol Paneli
1. TarayÄ±cÄ±da `http://192.168.x.x` aÃ§
2. SensÃ¶r verilerini gÃ¶rÃ¼ntÃ¼le (3s refresh)
3. **Manuel Mod:** Otomatik kontrolÃ¼ kapat
4. Butonlarla komut gÃ¶nder:
   - `Hava AÃ§/Kapat` â†’ Kapak + Fan
   - `IÅŸÄ±k AÃ§/Kapat` â†’ LED/Lamba
   - `Sulama AÃ§/Kapat` â†’ Pompa
5. **Otomatik Mod:** Toggle ile karar aÄŸacÄ±nÄ± aktifleÅŸtir

### 4. Manuel Kontrol

#### USB Serial (Arduino)
```
havaac     â†’ Kapak aÃ§ + Fan Ã§alÄ±ÅŸtÄ±r
havakapa   â†’ Kapak kapat + Fan durdur
isikac     â†’ IÅŸÄ±k aÃ§
isikkapa   â†’ IÅŸÄ±k kapat
sulaac     â†’ Sulama aÃ§
sulakapa   â†’ Sulama kapat
```

#### Web Paneli (NodeMCU)
```
http://192.168.x.x/command?cmd=havaac
http://192.168.x.x/command?cmd=sulaac
http://192.168.x.x/command?cmd=toggleauto
```

### 5. Kalibrasyon

**Toprak Nem SensÃ¶rÃ¼:**
```cpp
// main.cpp iÃ§inde
#define SOIL_DRY_VALUE 800    // Havada Ã¶lÃ§Ã¼len ADC
#define SOIL_WET_VALUE 300    // Suda Ã¶lÃ§Ã¼len ADC
```

**Kalman Filtresi (Gerekirse):**
```cpp
// Sensors.cpp iÃ§inde
tempFilter.setProcessNoise(0.001);     // Daha hÄ±zlÄ± deÄŸiÅŸim iÃ§in artÄ±r
tempFilter.setMeasurementNoise(0.5);   // SensÃ¶re gÃ¼ven dÃ¼ÅŸÃ¼kse artÄ±r
```

**NodeMCU Karar EÅŸikleri:**
```cpp
// NodeMCU_Receiver.ino iÃ§inde
// KOD-1: Donma riski
if (temp < 10.0 || dewPoint < 5.0) { ... }

// EÅŸikleri whiteboard fotoÄŸraflarÄ±ndan ayarlayÄ±n
```

---

## ğŸ” Test ve DoÄŸrulama

### SensÃ¶r Testleri
1. **BH1750:** El feneri ile Ä±ÅŸÄ±k deÄŸiÅŸimi â†’ RAW vs FILTERED karÅŸÄ±laÅŸtÄ±r
2. **BME680:** Ã‡akmak ile sÄ±caklÄ±k/gaz direnci â†’ Kalman yumuÅŸatmasÄ± gÃ¶zle
3. **MH-Z14A:** Nefes vererek CO2 artÄ±ÅŸÄ± â†’ 3 dakika Ä±sÄ±nma kontrolÃ¼
4. **Soil Sensor:** Kuru/Ä±slak toprak â†’ Kalibrasyon doÄŸrula

### Ä°letiÅŸim Testleri

#### UART2 (Arduino â†” NodeMCU)
```python
# NodeMCU Serial Monitor Ã§Ä±ktÄ±sÄ±
JSON received: {"temp":25.14,"hum":59.36,"pres":994.74,...}
Parsed: temp=25.14, hum=59.36, co2=450

# Arduino Serial Monitor Ã§Ä±ktÄ±sÄ±
[Serial2] JSON sent to NodeMCU
[Serial2] Command received: havaac
```

**Test:**
1. Arduino JSON gÃ¶nderimi â†’ NodeMCU parse etmeli
2. NodeMCU komut gÃ¶nderimi â†’ Arduino iÅŸlemeli
3. Cooldown mekanizmasÄ± â†’ 30s iÃ§inde aynÄ± komut tekrar gÃ¶nderilmemeli

#### LoRa (Arduino â†’ Yer Ä°stasyonu)
**Mesafe Testleri:**
- 1-5m: CRC baÅŸarÄ± >99%
- 10-50m: CRC baÅŸarÄ± >95%
- 100-500m: CRC baÅŸarÄ± >90%

**Hata KontrolÃ¼:**
```
[LORA] Paket gÃ¶nderildi (54 byte, CRC: 0x1A2B)
[YER] Paket alÄ±ndÄ± (CRC OK) - BaÅŸarÄ±: 66/68 (97.1%)
```

### Kontrol Sistemi Testleri

#### 1. Manuel Kontrol (NodeMCU Web)
```
Test 1: Hava aÃ§ â†’ Servo 0Â°, Fan ON, JSON roof:0, fan:true
Test 2: Hava kapat â†’ Servo 95Â°, Fan OFF, JSON roof:100, fan:false
Test 3: IÅŸÄ±k aÃ§ â†’ RÃ¶le ON, JSON light:true
Test 4: Sulama aÃ§ â†’ Pompa ON, diÄŸer sistemler OFF (gÃ¼venlik)
```

#### 2. Otomatik Karar AÄŸacÄ±
```
Senaryo 1: Temp < 10Â°C â†’ KOD-1 Donma â†’ havakapa + sulakapa
Senaryo 2: Temp > 32Â°C, Hum > 70% â†’ KOD-2 â†’ havaac
Senaryo 3: Soil < 20%, Temp > 28Â°C â†’ SULAMA-1 â†’ sulaac
Senaryo 4: 20Â°C â‰¤ Temp â‰¤ 26Â°C â†’ KOD-9 Optimal â†’ Enerji tasarrufu
```

#### 3. Cooldown Testi
```python
# NodeMCU Serial Monitor
[10:00:00] Komut: havaac gÃ¶nderildi
[10:00:15] Komut: havaac â†’ COOLDOWN (15s geÃ§ti, 30s bekleniyor)
[10:00:30] Komut: havaac gÃ¶nderildi (30s geÃ§ti, OK)
```

#### 4. Sulama GÃ¼venlik Testi
```
BaÅŸlangÄ±Ã§: Fan=ON, Light=ON, Roof=50%
sulaac komutu â†’ Fan=OFF, Light=OFF, Roof=100%, Pump=ON
sulakapa komutu â†’ Fan=ON, Light=ON, Roof=50%, Pump=OFF (geri yÃ¼kleme)
```

### Performans Metrikleri
- **SensÃ¶r Okuma:** 5 saniye periyot
- **JSON Ä°letimi:** ~250 byte/5s
- **LoRa Ä°letimi:** 54 byte/5s
- **Karar AlgoritmasÄ±:** 10 saniye periyot
- **Web Refresh:** 3 saniye
- **RAM KullanÄ±mÄ±:** Arduino %34.6 (2836/8192 byte)
- **Flash KullanÄ±mÄ±:** Arduino %16.6 (42160/253952 byte)

## ğŸ›¡ï¸ GÃ¼venlik Ã–zellikleri

### Arduino Mega (SensÃ¶r KatmanÄ±)
1. **Kalman Filtresi:** SensÃ¶r gÃ¼rÃ¼ltÃ¼sÃ¼ azaltarak yanlÄ±ÅŸ okumalarÄ± Ã¶nler
2. **Servo Titreme Ã–nleme:** Attach/detach pattern (PWM sadece harekette aktif)
3. **Sulama GÃ¼venlik Sistemi:** 
   - Sulama sÄ±rasÄ±nda tÃ¼m elektrikli sistemler otomatik kapatÄ±lÄ±r
   - Sulama bittiÄŸinde sistemler Ã¶nceki durumuna geri dÃ¶ner
   - Su-elektrik temasÄ± riski minimize edilir
4. **MH-Z14A Timeout:** 3 dakika Ä±sÄ±nma kontrolÃ¼ (sahte okumalar Ã¶nlenir)
5. **SÄ±nÄ±r KontrolÃ¼:** ADC deÄŸerleri 0-1023 arasÄ± kontrollÃ¼
6. **Non-Blocking Loop:** millis() tabanlÄ± zamanlama (seri komutlar kesintisiz)
7. **CRC-16 Veri DoÄŸrulama:** LoRa paketleri bÃ¼tÃ¼nlÃ¼k kontrolÃ¼ ile gÃ¶nderilir

### NodeMCU (Karar KatmanÄ±)
1. **Cooldown MekanizmasÄ±:** 30 saniye iÃ§inde aynÄ± komut tekrar gÃ¶nderilmez
2. **JSON Parsing Hata ToleransÄ±:** Parse hatalarÄ±nda eski deÄŸerler kullanÄ±lÄ±r
3. **Timeout KorumasÄ±:** 60 saniye boyunca veri gelmezse karar aÄŸacÄ± durur
4. **Ã–ncelik Sistemi:** Kritik durumlar (donma, fÄ±rtÄ±na) Ã¶ncelikle iÅŸlenir
5. **Donma KorumasÄ±:** Temp < 10Â°C â†’ Kapak + Sulama otomatik kapatÄ±lÄ±r
6. **FÄ±rtÄ±na KorumasÄ±:** BasÄ±nÃ§ < 985 hPa â†’ Acil kapanÄ±ÅŸ
7. **AÅŸÄ±rÄ± Sulama Ã–nleme:** Soil > 80% â†’ Sulama kilidi
8. **Web Server GÃ¼venlik:** GET istekleri ile komut doÄŸrulama
9. **WiFi Auto Reconnect:** BaÄŸlantÄ± kaybÄ±nda otomatik yeniden baÄŸlanma
10. **SD Card Loglama:** Veri kaybÄ± Ã¶nleme (isteÄŸe baÄŸlÄ±)

### Sistem Geneli
1. **Dual-Layer Architecture:** Arduino arÄ±zasÄ±nda NodeMCU Ã§alÄ±ÅŸÄ±r, NodeMCU arÄ±zasÄ±nda Arduino sensÃ¶r okumaya devam eder
2. **UART2 Hata ToleransÄ±:** BaÄŸlantÄ± kesilirse her iki cihaz da baÄŸÄ±msÄ±z Ã§alÄ±ÅŸÄ±r
3. **Manuel Override:** Her iki sistemde de USB/Serial manuel kontrol imkanÄ±
4. **State Recovery:** GÃ¼Ã§ kesintisi sonrasÄ± sistemler gÃ¼venli duruma dÃ¶ner
5. **Veri Yedekleme:** LoRa ile Yer Ä°stasyonuna paralel yedekleme

---

## ğŸ“Š Performans Metrikleri

### Arduino Mega (SensÃ¶r KatmanÄ±)
- **SensÃ¶r Okuma FrekansÄ±:** 5 saniye
- **Kalman Filtresi Ä°ÅŸlem SÃ¼resi:** <5ms (7 sensÃ¶r)
- **JSON OluÅŸturma:** <20ms (~250 byte)
- **LoRa GÃ¶nderim:** ~100ms (54 byte binary)
- **Servo YanÄ±t SÃ¼resi:** ~500ms (0-95Â° hareket)
- **RÃ¶le YanÄ±t SÃ¼resi:** <50ms
- **RAM KullanÄ±mÄ±:** 2836/8192 byte (34.6%)
- **Flash KullanÄ±mÄ±:** 42160/253952 byte (16.6%)
- **SensÃ¶r DoÄŸruluÄŸu (FiltrelenmiÅŸ):**
  - SÄ±caklÄ±k: Â±0.3Â°C (Ham: Â±1Â°C)
  - Nem: Â±1% (Ham: Â±3%)
  - CO2: Â±20ppm (Ham: Â±50ppm)
  - Toprak Nem: Â±2% (Ham: Â±5%)

### NodeMCU ESP8266 (Karar KatmanÄ±)
- **JSON Parsing SÃ¼resi:** <50ms (manuel parsing)
- **Karar AlgoritmasÄ± FrekansÄ±:** 10 saniye
- **Karar Ä°ÅŸlem SÃ¼resi:** <100ms (17 kod kontrolÃ¼)
- **Komut GÃ¶nderim:** <10ms (UART2 TX)
- **Web Server YanÄ±t:** ~200ms (GET request)
- **WiFi Latency:** <50ms (local network)
- **RAM KullanÄ±mÄ±:** ~40KB/80KB (50%)
- **Flash KullanÄ±mÄ±:** ~300KB/4MB (7.5%)
- **GÃ¼venilirlik:**
  - JSON Parse BaÅŸarÄ±: >99.9%
  - Cooldown EtkinliÄŸi: %100
  - Karar DoÄŸruluÄŸu: EÅŸik tabanlÄ± deterministik

### LoRa Ä°letiÅŸim
- **Paket Boyutu:** 54 byte (binary)
- **GÃ¶nderim FrekansÄ±:** 5 saniye
- **CRC BaÅŸarÄ± OranÄ±:**
  - 1-5m: >99%
  - 10-50m: >95%
  - 100-500m: >90%
- **Maksimum Menzil:** 3 km (aÃ§Ä±k alan, ideal koÅŸullar)
- **Throughput:** 10.8 byte/s (dÃ¼ÅŸÃ¼k gÃ¼Ã§ tÃ¼ketimi iÃ§in optimize)

### UART2 Ä°letiÅŸim
- **Baud Rate:** 9600 bps
- **JSON Paket:** ~250 byte (5 saniyede bir)
- **Komut Boyutu:** ~10 byte (olay tabanlÄ±)
- **Latency:** <10ms (kablolu baÄŸlantÄ±)
- **Hata OranÄ±:** <0.01% (kablolu gÃ¼venilirlik)

### Sistem Toplam
- **Ana DÃ¶ngÃ¼ (Arduino):** ~200ms (5 saniye wait)
- **Ana DÃ¶ngÃ¼ (NodeMCU):** ~500ms (10 saniye karar)
- **Toplam Veri AkÄ±ÅŸÄ±:** ~300 byte/5s (JSON + LoRa)
- **GÃ¼Ã§ TÃ¼ketimi (Normal):** ~2.5W
- **GÃ¼Ã§ TÃ¼ketimi (Peak):** ~6W (servo + pompa aktif)

---
  - IÅŸÄ±k: Â±10% (Ham: Â±20%)
  - Toprak Nem: Â±2% (Ham: Â±5%)

### AlÄ±cÄ± Sistem
- **Paket Alma SÃ¼resi:** <50ms
- **CRC DoÄŸrulama:** <10ms
- **Veri Ä°ÅŸleme:** <100ms
- **Serial Ã‡Ä±ktÄ±:** <500ms
- **BaÅŸarÄ± OranÄ±:** >95% (ideal koÅŸullar)

### LoRa Ä°letiÅŸim
- **Bant GeniÅŸliÄŸi:** 125 kHz
- **Paket Boyutu:** 54 byte (Kalman filtreli, optimize)
- **Hava SÃ¼resi:** ~180ms/paket (v2.0: 200ms, %10 daha hÄ±zlÄ±)
- **Maksimum Veri HÄ±zÄ±:** ~5 paket/saniye
- **GerÃ§ek KullanÄ±m:** 0.2 paket/saniye (5s aralÄ±k)
- **Enerji VerimliliÄŸi:** YÃ¼ksek (duty cycle %3.6, v2.0: %4)

### Kalman Filtresi PerformansÄ±
- **Ä°ÅŸlem SÃ¼resi:** <1ms/sensÃ¶r
- **Bellek KullanÄ±mÄ±:** 28 byte/filtre (7 filtre = 196 byte)
- **GÃ¼rÃ¼ltÃ¼ Azaltma:** %60-80 (sensÃ¶re gÃ¶re deÄŸiÅŸir)
- **Gecikme:** 1-2 okuma dÃ¶ngÃ¼sÃ¼ (5-10 saniye)
- **KararlÄ±lÄ±k:** 3-4 okuma sonrasÄ± optimal

---

## ğŸ”® Gelecek GeliÅŸtirmeler

### YakÄ±n Vadede (1-3 ay)
1. **WiFi Sleep Mode** - NodeMCU enerji tasarrufu optimizasyonu
2. **SD Kart Loglama** - NodeMCU'da geliÅŸmiÅŸ veri kaydetme (timestamp + JSON)
3. **LCD Ekran** - Yerel veri gÃ¶rÃ¼ntÃ¼leme (Arduino Mega tarafta)
4. **Adaptif Karar EÅŸikleri** - Mevsimsel otomatik ayarlama
5. **MQTT ProtokolÃ¼** - IoT cloud platformu entegrasyonu

### Orta Vadede (3-6 ay)
6. **Web Dashboard v2.0** - Grafiksel arayÃ¼z ve tarihsel veri analizi
7. **Mobil Uygulama** - iOS/Android kontrol ve bildirimler
8. **Ã‡oklu BÃ¶lge KontrolÃ¼** - FarklÄ± bitki tÃ¼rleri iÃ§in baÄŸÄ±msÄ±z zonlar
9. **Hava Durumu API** - DÄ±ÅŸ hava durumu ile entegrasyon (OpenWeatherMap)
10. **ESP32-CAM ModÃ¼lÃ¼** - Bitki saÄŸlÄ±ÄŸÄ± gÃ¶rsel izleme

### Uzun Vadede (6-12 ay)
11. **Yapay Zeka Optimizasyonu** - Machine learning ile karar aÄŸacÄ± iyileÅŸtirme
12. **GÃ¼neÅŸ Paneli + AkÃ¼** - Enerji baÄŸÄ±msÄ±zlÄ±ÄŸÄ±
13. **LoRaWAN Gateway** - The Things Network entegrasyonu
14. **Predictive Maintenance** - SensÃ¶r arÄ±zalarÄ±nÄ± Ã¶nceden tespit (Kalman drift analizi)
15. **Multi-Sensor Fusion** - Birden fazla sensÃ¶rden optimal tahmin (Extended Kalman Filter)
16. **Voice Control** - Google Assistant / Alexa entegrasyonu

---

## ğŸ“ Destek ve KatkÄ±

**GeliÅŸtirici:** Yusuf Islam Budak  
**Proje:** Bitirme Tezi - AkÄ±llÄ± TarÄ±m Sistemi  
**GitHub:** https://github.com/YusufIslamBudak/Bitirme-Projesi-Ak-ll-Tar-m-  
**Ãœniversite:** [Ãœniversite AdÄ±]  
**DanÄ±ÅŸman:** [DanÄ±ÅŸman AdÄ±]  
**Tarih:** Ekim 2025 - KasÄ±m 2025

**KatkÄ±da Bulunma:**
- Fork yapÄ±n ve pull request gÃ¶nderin
- Issue aÃ§arak hata bildirin veya Ã¶neride bulunun
- DokÃ¼mantasyonu iyileÅŸtirin

---

## ğŸ“„ Lisans

Bu proje bir bitirme tezi Ã§alÄ±ÅŸmasÄ±dÄ±r. Akademik amaÃ§lÄ± kullanÄ±m iÃ§in uygundur.

---

### Uzun Vadede (6-12 ay)
10. **Yapay Zeka** - Makine Ã¶ÄŸrenmesi ile optimizasyon ve tahminleme
11. **GÃ¼neÅŸ Paneli** - Enerji baÄŸÄ±msÄ±zlÄ±ÄŸÄ±
12. **LoRaWAN Gateway** - The Things Network entegrasyonu
13. **Ã‡oklu AlÄ±cÄ±** - Birden fazla yer istasyonu desteÄŸi
14. **Predictive Maintenance** - SensÃ¶r arÄ±zalarÄ±nÄ± Ã¶nceden tespit
15. **Multi-Sensor Fusion** - Birden fazla sensÃ¶rden optimal tahmin

---

## ğŸ“ Destek ve KatkÄ±

**GeliÅŸtirici:** Yusuf Islam Budak  
**Proje:** Bitirme Tezi - AkÄ±llÄ± TarÄ±m Sistemi  
**GitHub:** https://github.com/YusufIslamBudak/Bitirme-Projesi-Ak-ll-Tar-m-  
**Tarih:** Ekim 2025

---

## ğŸ“„ Lisans

Bu proje bir bitirme tezi Ã§alÄ±ÅŸmasÄ±dÄ±r.

---

**Son GÃ¼ncelleme:** 19 KasÄ±m 2025

**Versiyon:** 3.0 - ModÃ¼ler Mimari + Kalman Filtresi Entegrasyonu

### Versiyon GeÃ§miÅŸi

**v3.0 (19 KasÄ±m 2025)**
- âœ… ModÃ¼ler mimari: Sensors, Calculations, KalmanFilter, Communication modÃ¼lleri
- âœ… 1D Kalman filtresi entegrasyonu (7 sensÃ¶r iÃ§in ayrÄ± parametreler)
- âœ… RAW ve FILTERED deÄŸerlerin karÅŸÄ±laÅŸtÄ±rmalÄ± gÃ¶sterimi
- âœ… LoRa paketlerinde sadece filtrelenmiÅŸ deÄŸerler gÃ¶nderimi (54 byte)
- âœ… Bilimsel hesaplamalarÄ±n ayrÄ± modÃ¼le taÅŸÄ±nmasÄ±
- âœ… Kod organizasyonu ve bakÄ±m kolaylÄ±ÄŸÄ± artÄ±rÄ±ldÄ±

**v2.0 (27 Ekim 2025)**
- âœ… LoRa E32 kablosuz iletiÅŸim entegrasyonu
- âœ… Yer istasyonu alÄ±cÄ± sistemi
- âœ… Binary paket transferi + CRC hata kontrolÃ¼
- âœ… Sulama gÃ¼venlik sistemi (otomatik kapama/geri yÃ¼kleme)

**v1.0 (Ekim 2025)**
- âœ… Temel sensÃ¶r okuma (BH1750, BME680, MH-Z14A, Soil)
- âœ… Otomatik/Manuel kontrol modlarÄ±
- âœ… Servo, rÃ¶le kontrolleri
- âœ… 9 sera kodu + 8 sulama kodu

