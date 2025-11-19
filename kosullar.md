# Akıllı Sera Sistemi - Kontrol Koşulları

## 🎮 Kontrol Modları

### 1. OTOMATIK MOD
Sistem sensör verilerine göre otomatik kararlar verir (aşağıdaki tüm koşullar aktif).

### 2. MANUEL MOD  
Kullanıcı serial port üzerinden komutlarla sistemi kontrol eder:

**Serial Komutlar:**
```
havaac    → Sera kapağını aç (0°) + Fan açık
havakapa  → Sera kapağını kapat (95°) + Fan kapalı
isikac    → Aydınlatmayı aç
isikkapa  → Aydınlatmayı kapat
sulaac    → Sulamayı aç
sulakapa  → Sulamayı kapat
```

**Kullanım:** Serial Monitor'da (115200 baud) komutu yazıp Enter'a basın.

**⚠️ Önemli - Sulama Güvenlik Modu:**
- `sulaac` komutu verildiğinde:
  - Mevcut sistem durumları otomatik kaydedilir
  - Kapak kapatılır + Fan kapatılır
  - Işık kapatılır
  - Sulama başlatılır
  
- `sulakapa` komutu verildiğinde:
  - Sulama durdurulur
  - Tüm sistemler önceki durumuna geri döner

---

## 🌱 Sera Üst Kapak Açma/Kapama Koşulları (OTOMATIK MOD)

### 📊 Mevcut Sensör Verileri:
- **Sıcaklık** (BME680): °C
- **Nem** (BME680): %
- **CO2** (MH-Z14A): ppm
- **Işık Şiddeti** (BH1750): lux
- **Basınç** (BME680): hPa
- **Toprak Nemi** (MH Water Sensor): %
- **Hissedilen Sıcaklık** (Heat Index): °C
- **Çiy Noktası** (Dew Point): °C

---

## 🔴 KODU 1: ACİL KAPAK AÇMA (YÜKSEK ÖNCELİK)

### Durum: Aşırı Sıcak + Yüksek Nem
**Koşullar:**
```
Sıcaklık > 32°C
VE
Nem > 70%
VE
Heat Index > 35°C
```
**Aksiyon:** 
- ✅ Sera kapağını %100 aç
- ✅ Soğutma fanını çalıştır
- ⚠️ Uyarı: "Aşırı sıcak ve nem - Bitki stres riski yüksek!"

---

## 🟠 KOD 2: KAPAK AÇMA (YÜKSEK SICAKLIK)

### Durum: Sera İçi Çok Sıcak
**Koşullar:**
```
Sıcaklık > 28°C
VE
CO2 > 800 ppm
```
**Aksiyon:**
- ✅ Sera kapağını %75 aç
- ✅ Havalandırma sistemini çalıştır
- 📝 Log: "Yüksek sıcaklık - Havalandırma başlatıldı"

---

## 🟡 KOD 3: KAPAK AÇMA (CO2 YÜKSEK)

### Durum: CO2 Konsantrasyonu Çok Yüksek
**Koşullar:**
```
CO2 > 1500 ppm
VE
Sıcaklık > 20°C
```
**Aksiyon:**
- ✅ Sera kapağını %50 aç
- ⏱️ 10 dakika bekle ve CO2'yi tekrar ölçü
- 📝 Log: "CO2 seviyesi yüksek - Havalandırma gerekli"

**Neden?** Bitkiler fotosentez için CO2'ye ihtiyaç duyar, ancak >1500 ppm bitki gelişimini engelleyebilir.

---

## 🟢 KOD 4: KAPAK AÇMA (NEM KONTROLÜ)

### Durum: Yüksek Nem - Küf Riski
**Koşullar:**
```
Nem > 85%
VE
Sıcaklık < 25°C
VE
(Sıcaklık - Çiy Noktası) < 3°C
```
**Aksiyon:**
- ✅ Sera kapağını %40 aç
- ⚠️ Uyarı: "Küf riski - Nem kontrolü gerekli"
- 📝 Log: "Yüksek nem tespit edildi"

**Neden?** Çiy noktası sıcaklığa yakınsa yoğuşma ve küf riski artar.

---

## 🔵 KOD 5: KAPAK AÇMA (GÜNDÜZ HAVALANDIRMA)

### Durum: Gündüz Normal Havalandırma
**Koşullar:**
```
Işık Şiddeti > 10000 lux
VE
Sıcaklık > 22°C
VE
Sıcaklık < 28°C
VE
CO2 < 1000 ppm
```
**Aksiyon:**
- ✅ Sera kapağını %25 aç (parsiyel havalandırma)
- 📝 Log: "Gündüz normal havalandırma"

**Neden?** Gün içinde ılımlı hava sirkülasyonu bitki sağlığı için önemli.

---

## 🟣 KOD 6: KAPAK KAPAMA (GECE)

### Durum: Gece Soğuk Koruma
**Koşullar:**
```
Işık Şiddeti < 50 lux
VE
Sıcaklık < 18°C
```
**Aksiyon:**
- ❌ Sera kapağını %100 kapat
- 🔥 Isıtma sistemini aktif et (varsa)
- 📝 Log: "Gece modu - Sıcaklık koruması"

**Neden?** Geceleyin sera içi sıcaklık düşmesin, bitkiler soğuktan zarar görmesin.

---

## ⚫ KOD 7: KAPAK KAPAMA (AŞIRI SOĞUK)

### Durum: Donma Riski
**Koşullar:**
```
Sıcaklık < 10°C
VEYA
Çiy Noktası < 5°C
```
**Aksiyon:**
- ❌ Sera kapağını %100 kapat
- 🚨 ACİL UYARI: "Donma riski - Acil müdahale!"
- 🔥 Isıtıcıyı maksimuma çıkar

---

## 🌧️ KOD 8: KAPAK KAPAMA (YAĞMUR/FIRTINA)

### Durum: Hava Basıncı Düşüşü (Fırtına Öncesi)
**Koşullar:**
```
Basınç < 985 hPa
VEYA
1 saat içinde basınç düşüşü > 3 hPa
```
**Aksiyon:**
- ❌ Sera kapağını %100 kapat
- ⚠️ Uyarı: "Hava şartları kötüleşiyor - Koruma modu"

**Neden?** Düşük basınç fırtına/yağış habercisidir. Sera korunmalı.

---

## 🌤️ KOD 9: İDEAL DURUM (KAPAK KAPALI)

### Durum: Optimum Sera Koşulları
**Koşullar:**
```
Sıcaklık: 20-26°C
Nem: 50-70%
CO2: 400-1000 ppm
Işık: 5000-15000 lux (gündüz)
```
**Aksiyon:**
- ✅ Sera kapağını kapat (enerji tasarrufu)
- 📝 Log: "İdeal koşullar - Sistem stabil"

---

## 📋 Öncelik Sıralaması (En Yüksekten En Düşüğe)

1. **KOD 7** - Donma riski (ACİL)
2. **KOD 1** - Aşırı sıcak + nem (ACİL)
3. **KOD 8** - Fırtına riski (YÜKSEKÖNCELİK)
4. **KOD 2** - Yüksek sıcaklık (YÜKSEK)
5. **KOD 3** - Yüksek CO2 (ORTA)
6. **KOD 4** - Yüksek nem (ORTA)
7. **KOD 6** - Gece kapama (DÜŞÜK)
8. **KOD 5** - Gündüz havalandırma (DÜŞÜK)
9. **KOD 9** - İdeal durum (BİLGİ)

---

## 🎯 Ek Akıllı Özellikler

### 1. Histerezis (Titreme Önleme)
Kapak sürekli açılıp kapanmasın diye:
```
Kapak açma eşiği: 28°C
Kapak kapama eşiği: 26°C (2°C fark)
```

### 2. Zaman Bazlı Gecikme
Ani değişimlerde hemen hareket etme:
```
Koşul 30 saniye boyunca sağlanırsa aksiyon al
```

### 3. Güneş Işığı Optimizasyonu
```
Eğer Işık > 20000 lux VE Sıcaklık > 30°C:
  -> Kapağı aç VE gölgelik perdesi aç
```

### 4. Nem-Sıcaklık Dengesi (VPD - Vapor Pressure Deficit)
```
VPD = Doymuş Buhar Basıncı - Gerçek Buhar Basıncı
İdeal VPD: 0.8-1.2 kPa (Vejetatif büyüme)
```

---

## 📊 Örnek Karar Ağacı

```
BAŞLA
  │
  ├─ Sıcaklık < 10°C? ──> EVET ──> [KOD 7: KAPAK KAPAT - ACIL]
  │
  ├─ Sıcaklık > 32°C VE Nem > 70%? ──> EVET ──> [KOD 1: KAPAK AÇ %100]
  │
  ├─ Basınç < 985 hPa? ──> EVET ──> [KOD 8: KAPAK KAPAT - FIRTINA]
  │
  ├─ Sıcaklık > 28°C VE CO2 > 800? ──> EVET ──> [KOD 2: KAPAK AÇ %75]
  │
  ├─ CO2 > 1500 ppm? ──> EVET ──> [KOD 3: KAPAK AÇ %50]
  │
  ├─ Nem > 85% VE ΔT < 3°C? ──> EVET ──> [KOD 4: KAPAK AÇ %40]
  │
  ├─ Işık < 50 lux VE Sıcaklık < 18°C? ──> EVET ──> [KOD 6: KAPAK KAPAT]
  │
  ├─ Işık > 10000 VE 22<T<28? ──> EVET ──> [KOD 5: KAPAK AÇ %25]
  │
  └─ Aksi halde ──> [KOD 9: İDEAL - KAPAK KAPAT]
```

---

## 🔧 Gerekli Donanım Eklemeleri

1. **Servo Motor / Lineer Aktüatör** - Kapak açma mekanizması
2. **Röle Modülü (2 Kanal)** - Servo kontrol + Sulama pompası
3. **MH Water Sensor** - Toprak nem ölçümü ✅ EKLENMIŞ
4. **Su Pompası / Vana** - Sulama sistemi
5. **Yağmur Sensörü** (Opsiyonel) - Fırtına tespiti için

---

## 💧 SULAMA SİSTEMİ KONTROL KOŞULLARI

### 📊 Sulama Kararı Parametreleri:
- **Toprak Nemi** (MH Water Sensor): %
- **Sıcaklık** (BME680): °C
- **Hava Nemi** (BME680): %
- **Işık Şiddeti** (BH1750): lux
- **Basınç** (BME680): hPa

---

## 🚰 SULAMA KODU 1: ACİL SULAMA

### Durum: Çok Kuru Toprak + Yüksek Sıcaklık
**Koşullar:**
```
Toprak Nemi < 20%
VE
Sıcaklık > 28°C
```
**Aksiyon:**
- 🚰 Sulamayı AÇIK (30 saniye)
- ✅ Sera kapağını %50 aç (buharlaşma kontrolü)
- 🚨 UYARI: "Acil sulama - Bitki stres riski!"
- ⏱️ 10 dakika sonra tekrar kontrol et

**Neden?** Yüksek sıcaklık + kuru toprak = bitki dehidrasyonu riski

---

## 🟠 SULAMA KODU 2: NORMAL SULAMA (KURU TOPRAK)

### Durum: Toprak Kuru
**Koşullar:**
```
Toprak Nemi < 40%
VE
Sıcaklık > 20°C
VE
Işık > 1000 lux (Gündüz)
```
**Aksiyon:**
- 🚰 Sulamayı AÇIK (20 saniye)
- 📝 Log: "Normal sulama başlatıldı"
- ⏱️ 15 dakika sonra tekrar kontrol et

---

## 🟢 SULAMA KODU 3: AKŞAM SULAMA (OPTİMAL)

### Durum: Akşam Saatleri + Orta Kuru Toprak
**Koşullar:**
```
Toprak Nemi < 50%
VE
Işık < 1000 lux (Akşam)
VE
Sıcaklık > 15°C
```
**Aksiyon:**
- 🚰 Sulamayı AÇIK (25 saniye)
- 💡 En verimli sulama zamanı!
- 📝 Log: "Akşam sulama - Optimal zaman"

**Neden?** Akşam sulama buharlaşmayı minimize eder, su verimliliği maksimum

---

## 🟡 SULAMA KODU 4: SULAMA İPTAL (YAĞMUR)

### Durum: Yağış Tespit Edildi
**Koşullar:**
```
Basınç < 990 hPa
VE
Hava Nemi > 85%
```
**VEYA**
```
Toprak Nemi > 80%
```
**Aksiyon:**
- ❌ Sulamayı DURDUR
- 📝 Log: "Sulama iptal - Doğal yağış/aşırı nem"
- ⏱️ 30 dakika bekle

**Neden?** Doğal yağış sulamayı gereksiz kılar, enerji tasarrufu

---

## 🔴 SULAMA KODU 5: AŞIRI SULAMA KORUMASI

### Durum: Toprak Çok Islak
**Koşullar:**
```
Toprak Nemi > 90%
```
**Aksiyon:**
- 🚨 UYARI: "AŞIRI SULAMA - Drenaj problemi!"
- ❌ Sulama sistemini kilitle (24 saat)
- ✅ Sera kapağını %75 aç (kuruma için)
- 📝 Log: "Toprak aşırı ıslak - Kök çürümesi riski"

**Neden?** Aşırı su bitki köklerini çürütür, oksijen eksikliğine neden olur

---

## 🟣 SULAMA KODU 6: KÜFLENME RİSKİ

### Durum: Yüksek Toprak Nemi + Yüksek Hava Nemi
**Koşullar:**
```
Toprak Nemi > 80%
VE
Hava Nemi > 85%
VE
Sıcaklık < 22°C
```
**Aksiyon:**
- ❌ Sulamayı DURDUR
- ✅ Sera kapağını %40 aç (kuruma + hava sirkülasyonu)
- 📝 Log: "Küf riski - Havalandırma aktif"

**Neden?** Yüksek nem + düşük sıcaklık = küf üremesi için ideal ortam

---

## 🔵 SULAMA KODU 7: GEJ SULAMA YASAĞI

### Durum: Gece Soğuk
**Koşullar:**
```
Işık < 50 lux (Gece)
VE
Sıcaklık < 12°C
```
**Aksiyon:**
- ❌ Sulamayı DURDUR
- 📝 Log: "Gece sulama yasağı - Soğuk koruma"
- ⏱️ Sabah güneş çıkana kadar bekle

**Neden?** Gece sulama toprak sıcaklığını düşürür, bitki stresine neden olur

---

## 🟢 SULAMA KODU 8: İDEAL DURUM (SULAMA YOK)

### Durum: Optimal Toprak Nemi
**Koşullar:**
```
Toprak Nemi: 50-70%
```
**Aksiyon:**
- ✅ Sulama sistemini KAPAT
- 📝 Log: "Toprak nem seviyesi ideal"
- ⏱️ Normal monitoring devam et

---

## 📋 Sulama Öncelik Sıralaması

1. **SULAMA KODU 5** - Aşırı sulama koruması (ACİL)
2. **SULAMA KODU 7** - Gece sulama yasağı (YÜKSEK)
3. **SULAMA KODU 4** - Yağmur iptali (YÜKSEK)
4. **SULAMA KODU 6** - Küf riski (ORTA)
5. **SULAMA KODU 1** - Acil sulama (ORTA)
6. **SULAMA KODU 3** - Akşam sulama (DÜŞÜK)
7. **SULAMA KODU 2** - Normal sulama (DÜŞÜK)
8. **SULAMA KODU 8** - İdeal durum (BİLGİ)

---

## 🌊 Sulama Karar Ağacı

```
BAŞLA (Sulama Kontrolü)
  │
  ├─ Toprak Nemi > 90%? ──> EVET ──> [SULAMA KODU 5: AŞIRI SULAMA - DURDUR]
  │
  ├─ Gece VE Sıcaklık < 12°C? ──> EVET ──> [SULAMA KODU 7: GECE YASAĞI]
  │
  ├─ Basınç < 990 VE Nem > 85%? ──> EVET ──> [SULAMA KODU 4: YAĞMUR İPTAL]
  │
  ├─ Toprak > 80% VE Hava Nem > 85%? ──> EVET ──> [SULAMA KODU 6: KÜF RİSKİ]
  │
  ├─ Toprak < 20% VE Sıcaklık > 28°C? ──> EVET ──> [SULAMA KODU 1: ACİL]
  │
  ├─ Toprak < 50% VE Işık < 1000? ──> EVET ──> [SULAMA KODU 3: AKŞAM]
  │
  ├─ Toprak < 40% VE Gündüz? ──> EVET ──> [SULAMA KODU 2: NORMAL]
  │
  └─ Toprak 50-70%? ──> EVET ──> [SULAMA KODU 8: İDEAL - DURDUR]
```

---

## 💧 Sulama Süre Tablosu

| Toprak Nemi | Sıcaklık | Sulama Süresi | Bekleme |
|-------------|----------|---------------|---------|
| < 20% | > 28°C | 30 saniye | 10 dk |
| < 40% | 20-28°C | 20 saniye | 15 dk |
| < 50% | Akşam | 25 saniye | 20 dk |
| 50-70% | Herhangi | DURDUR | - |
| > 80% | Herhangi | DURDUR | 30 dk |
| > 90% | Herhangi | KİLİTLİ | 24 saat |

---

## 📝 Not

Bu koşullar **genel sera bitkileri** için optimize edilmiştir. Özel bitkiler için (örn: tropikal bitkiler, kakütüsler) değerler ayarlanmalıdır.

**Referanslar:**
- ASHRAE Greenhouse Design Standards
- FAO Agricultural Guidelines
- Plant Climate Control Systems (Wageningen University)
- Irrigation Scheduling for Greenhouse Production (UC Davis)
