# Akıllı Sera Sistemi - Kontrol Koşulları

## 🌱 Sera Üst Kapak Açma/Kapama Koşulları

### 📊 Mevcut Sensör Verileri:
- **Sıcaklık** (BME680): °C
- **Nem** (BME680): %
- **CO2** (MH-Z14A): ppm
- **Işık Şiddeti** (BH1750): lux
- **Basınç** (BME680): hPa
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
2. **Röle Modülü** - Servo kontrol için
3. **Yağmur Sensörü** (Opsiyonel) - Fırtına tespiti için
4. **Toprak Nem Sensörü** (Opsiyonel) - Sulama kontrolü

---

## 📝 Not

Bu koşullar **genel sera bitkileri** için optimize edilmiştir. Özel bitkiler için (örn: tropikal bitkiler, kakütüsler) değerler ayarlanmalıdır.

**Referanslar:**
- ASHRAE Greenhouse Design Standards
- FAO Agricultural Guidelines
- Plant Climate Control Systems (Wageningen University)
