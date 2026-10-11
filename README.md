<div align="center">

# 🔒 PhotosCrypto

**Next-Generation C++17 Secure Photo Vault & Compression Engine**  
*Encrypt, compress, and conceal your private images into compact, authenticated symbol text files.*

[![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C.svg?logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Crypto: AES-256-GCM](https://img.shields.io/badge/Encryption-AES--256--GCM-blue.svg)](https://openssl.org)
[![KDF: scrypt](https://img.shields.io/badge/KDF-scrypt%20128MB-blueviolet.svg)](#security-architecture)
[![Compression: zlib](https://img.shields.io/badge/Compression-zlib%20deflate-orange.svg)](#storage-efficiency)
[![UI: GTK4 + Libadwaita](https://img.shields.io/badge/GUI-GTK4%20%2F%20Libadwaita-emerald.svg?logo=gnome)](https://gtk.org)
[![Packaging: Flatpak](https://img.shields.io/badge/Package-Flatpak%20Sandbox-teal.svg?logo=flatpak)](https://flatpak.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: Cross-Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-black.svg)](#installation)

<br/>

> 🇹🇷 **Türkçe:** Fotoğraflarınızı ve hassas görsellerinizi modern C++17, donanım hızlandırmalı AES-256-GCM ve scrypt ile şifreler, dahili zlib ile otomatik sıkıştırıp güvenli metin dosyalarına dönüştürür. Tek ve birleşik nihai sürümdür.  
> 🇬🇧 **English:** PhotosCrypto compresses and encrypts your photos into safe, portable text files using industry-standard cryptography. Designed for absolute privacy, memory safety, and cross-platform simplicity.

---

</div>

## 📌 İçindekiler / Table of Contents
1. [🌟 Öne Çıkan Özellikler / Highlights](#-öne-çıkan-özellikler--highlights)
2. [🧠 Mimari ve Güvenlik Modeli / Architecture & Security](#-mimari-ve-güvenlik-modeli--architecture--security)
3. [🚀 Kurulum / Installation](#-kurulum--installation)
   - [Linux (Flatpak & Kaynak Kod)](#1-linux)
   - [Windows (MSYS2 & WSL)](#2-windows)
   - [macOS (Homebrew)](#3-macos)
4. [💻 Kullanım Kılavuzu / Usage](#-kullanım-kılavuzu--usage)
   - [Komut Satırı (CLI)](#komut-satırı-cli)
   - [Grafik Arayüz (GTK4 GUI)](#grafik-arayüz-gtk4-gui)
5. [🧪 Test ve Doğrulama / Tests](#-test-ve-doğrulama--tests)
6. [📄 Lisans / License](#-lisans--license)

---

## 🌟 Öne Çıkan Özellikler / Highlights

### 🇬🇧 English
- **Single Unified Application:** Consolidates all encoding, decoding, compression, and UI features into one unified and lightweight executable family (`photoscrypto-cli` & `photoscrypto-gui`).
- **Military-Grade Authenticated Encryption:** AES-256-GCM combined with scrypt key derivation ($N=2^{17}, r=8, p=1$, ~128 MiB RAM cost per guess) shields your images against brute-force attacks and bit tampering.
- **Built-in zlib Compression:** Automatic compression reduces file size by **40% to 80%** prior to encryption, drastically cutting down storage space.
- **Smart Automatic Format Detection:** Original metadata and format headers (`.png`, `.jpg`, `.jpeg`, `.webp`, `.avif`, `.heic`, `.gif`, `.bmp`) are preserved inside the encrypted payload and seamlessly restored byte-for-byte upon decoding.
- **Optional Password Protection:** Choose between strong passphrase protection or `--no-password` high-speed obfuscated archival for instant single-click recovery.
- **Self-Destruct Guard (Anti-Tamper):** Optional configurable threshold (`--max-attempts N`) to permanently shred and erase the file upon repeated incorrect password attempts.
- **Dynamic 5-Level Password Shield:** Visual GTK4 indicator guiding users to create resilient passphrases mixing letters, numerals, and symbols.

### 🇹🇷 Türkçe
- **Tek ve Birleşik Uygulama:** Eski prototipleri ve ayrık yapıları tek bir çatı altında toplayan; komut satırı ve grafik arayüzü tek bir optimize kod tabanında birleştiren modern uygulama.
- **Askeri Düzey Kimlik Doğrulamalı Şifreleme:** AES-256-GCM ve scrypt anahtar türetim fonksiyonu (tahmin başına ~128 MB RAM maliyeti) ile kaba kuvvet (brute-force) saldırılarına ve bayt tahrifatına karşı tam koruma.
- **Dahili zlib Veri Sıkıştırması:** Şifrelemeden önce otomatik zlib sıkıştırması yaparak disk alanından **%40 ile %80** arasında tasarruf sağlar.
- **Akıllı Format ve Dosya Türü Tanıma:** Fotoğrafın özgün formatı (`.png`, `.jpg`, `.webp`, `.avif`, `.heic`, `.gif`, `.bmp`) şifreli gövdede güvenle saklanır; geri çevrildiğinde uzantı otomatik eklenir ve bayt bayt eksiksiz geri gelir.
- **İsteğe Bağlı Parola Koruması:** İster güçlü parola koruması uygulayın, ister tek tıkla hızlı arşivleme için `--no-password` şifresiz modunu kullanın.
- **Kendi Kendini İmha Modu (Opsiyonel):** Belirlenen limit aşıldığında (`--max-attempts N`) yanlış şifre denemelerinde dosyayı kalıcı olarak silen koruma kalkanı.
- **5 Seviyeli Dinamik Parola Kalkanı:** Harf, rakam ve sembol çeşitliliğine göre şifre gücünü anlık hesaplayan sezgisel arayüz göstergesi.

---

## 🧠 Mimari ve Güvenlik Modeli / Architecture & Security

```text
Ham Fotoğraf (Raw Photo) 
      │
      ▼
┌──────────────────────────────────────────────┐
│ 1. Akıllı Format ve Meta Veri Çıkarımı       │
└──────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────┐
│ 2. zlib Deflate Sıkıştırma Bellek Tamponu    │ (Boyut %40-%80 Küçülür)
└──────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────┐
│ 3. scrypt Anahtar Türetimi (Salt [16B])     │ (128 MB RAM Maliyeti / Deneme)
└──────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────┐
│ 4. OpenSSL AES-256-GCM Şifreleme             │ (IV [12B] + Ciphertext + Tag [16B])
└──────────────────────────────────────────────┘
      │
      ▼
┌──────────────────────────────────────────────┐
│ 5. Taşınabilir Sembol Alfabesi / Metin Çıktı │ (0600 Güvenli İzinler)
└──────────────────────────────────────────────┘
```

- **Sıfır Sızıntı (Zero Metadata Leak):** Şifrelenmiş metin çıktısı dışarıdan bakıldığında rastgele sembollerden farksızdır; başlık, fotoğraf boyutu veya kamera bilgisi asla sızdırılmaz.
- **Tahrifat Tespiti (AEAD Tamper Proof):** Dosyadaki tek bir harf veya bit değiştirilse dahi OpenSSL GCM doğrulama etiketi işlemi anında durdurur ve hiçbir çıktı yazılmaz.
- **Güvenli İzinler:** Üretilen dosyalar yalnızca dosya sahibinin okuyabileceği `0600` Linux dosya izinleriyle oluşturulur.

---

## 🚀 Kurulum / Installation

### 1. Linux

#### Seçenek A: Flatpak (Önerilen / Recommended)
Tüm Linux dağıtımlarında bağımlılık derdi olmadan güvenli sandbox ortamında çalıştırmak için:

```bash
git clone https://github.com/hunte-av/photoscrypto.git
cd photoscrypto
flatpak install flathub org.freedesktop.Sdk//25.08
flatpak-builder --user --install --force-clean build-dir io.github.hunte_av.PhotosCrypto.yml
flatpak run io.github.hunte_av.PhotosCrypto
```

#### Seçenek B: Kaynak Koddan Derleme (Native Build)
Sistem paket yöneticinizle derleme araçlarını ve kütüphaneleri kurun:

**Ubuntu / Debian:**
```bash
sudo apt update
sudo apt install build-essential pkg-config libssl-dev zlib1g-dev libgtk-4-dev libadwaita-1-dev
```

**Fedora / RHEL:**
```bash
sudo dnf install gcc-c++ make pkgconf openssl-devel zlib-devel gtk4-devel libadwaita-devel
```

**Arch Linux / Manjaro:**
```bash
sudo pacman -S base-devel openssl zlib gtk4 libadwaita
```

**Derleme ve Kurulum:**
```bash
make
make test
sudo make install # /usr/local/bin dizinine yükler
```

---

### 2. Windows

Windows üzerinde PhotosCrypto **MSYS2 (UCRT64)** ile yerel `.exe` olarak veya **WSL** (Windows Subsystem for Linux) üzerinden çalıştırılabilir.

#### MSYS2 (UCRT64) ile Yerel Derleme:
1. [msys2.org](https://www.msys2.org/) adresinden MSYS2'yi kurun.
2. **MSYS2 UCRT64** terminalini açın ve bağımlılıkları yükleyin:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-openssl \
          mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-gtk4 mingw-w64-ucrt-x86_64-libadwaita git
```
3. Projeyi derleyin:
```bash
git clone https://github.com/hunte-av/photoscrypto.git
cd photoscrypto
mingw32-make
./photoscrypto-cli.exe --help
```

---

### 3. macOS

macOS üzerinde Homebrew ile kolayca kurulur:

```bash
# 1. Bağımlılıkları yükleyin
brew install openssl zlib pkg-config make gcc gtk4 libadwaita

# 2. PKG_CONFIG ortam değişkenlerini ayarlayın
export PKG_CONFIG_PATH="$(brew --prefix openssl)/lib/pkgconfig:$(brew --prefix zlib)/lib/pkgconfig:$PKG_CONFIG_PATH"

# 3. Derleyin
git clone https://github.com/hunte-av/photoscrypto.git
cd photoscrypto
make
./photoscrypto-cli --help
```

---

## 💻 Kullanım Kılavuzu / Usage

### Komut Satırı (CLI)

```text
photoscrypto encode <photo> <output.txt> [options]
photoscrypto decode <input.txt> [output] [options]
```

| Seçenek / Flag | Açıklama (TR) | Description (EN) |
|---|---|---|
| `--force` | Var olan dosyanın üzerine yaz | Overwrite existing output file |
| `--no-password` | Parola sormadan şifrele / geri yükle | Fast obfuscated encode without password |
| `--password-stdin` | Parolayı stdin'den oku (otomasyon için) | Read password from standard input line |
| `--max-attempts N` | N yanlış şifrede dosyayı yok et (1-10) | Self-destruct after N failed attempts |
| `-h, --help` | Yardım mesajını göster | Display command-line help screen |
| `-V, --version` | Sürüm bilgisini göster | Print binary version information |

#### Örnekler / Practical Examples:

```bash
# 1. Parolalı Fotoğraf Şifreleme (Terminalden şifre sorar, ekranda görünmez)
photoscrypto-cli encode aile_fotografi.jpg gizli_not.txt

# 2. Parolalı Fotoğrafı Geri Çözme (Orijinal uzantıyı otomatik tamamlar -> aile_fotografi.jpg)
photoscrypto-cli decode gizli_not.txt aile_fotografi

# 3. Şifresiz Hızlı Sıkıştırma ve Gizleme (Tek tıkla açılabilen arşiv)
photoscrypto-cli encode belge.png guvenli_arsiv.txt --no-password

# 4. Şifresiz Dosyayı Tek Adımda Çözme
photoscrypto-cli decode guvenli_arsiv.txt belge

# 5. Kendi Kendini İmha Korumasıyla Şifreleme (3 yanlış denemede dosyayı siler)
photoscrypto-cli encode gizli.png safe.txt --max-attempts 3
```

---

### Grafik Arayüz (GTK4 GUI)

Modern Libadwaita temalı grafik arayüzü başlatmak için:

```bash
photoscrypto-gui
# veya Flatpak üzerinden:
flatpak run io.github.hunte_av.PhotosCrypto
```

- **Sürükle-Bırak:** Fotoğrafınızı pencereye sürükleyin.
- **Dinamik Kalkan:** Parolanızı yazarken 5 aşamalı kalkan ile şifre karmaşıklığını takip edin.
- **Tek Tıkla Dönüştürme:** Saniyeler içinde metin dosyasına çevirin ya da geri çözün.

---

## 🧪 Test ve Doğrulama / Tests

Tüm kriptografik doğrulamaları, zlib sıkıştırma oranlarını, AEAD tahrifat tespitini ve round-trip tutarlılığını test etmek için:

```bash
make test
```

Beklenen çıktı:
```text
Running photoscrypto v1.1 tests...
  [PASS] encode with password creates output
  [PASS] output has mode 600
  [PASS] zlib compression active (file size shrank from 320008B to 970B)
  [PASS] round-trip with password is byte-identical (.png detected)
  [PASS] wrong password rejected
  [PASS] no output on wrong password
  [PASS] encode with --no-password succeeds
  [PASS] round-trip without password is byte-identical
  [PASS] tampered file rejected by AES-GCM tag verification
  [PASS] refuses to overwrite without --force
  [PASS] --force overwrites
  [PASS] does not write through symlinks
  [PASS] unknown option rejected

Result: 13 passed, 0 failed
```

---

## ⚠️ Önemli Güvenlik Hatırlatmaları / Security Notice

1. **Parolanızı Unutursanız Fotoğraf Kurtarılamaz:** Sistemde hiçbir arka kapı (backdoor) veya merkezi kurtarma anahtarı yoktur.
2. **Kendi Kendini İmha Uyarısı:** `--max-attempts` seçeneği dosyanın üzerindeki deneme sayacını kullanır. Dosyanın önceden kopyasını alan bir saldırgana karşı koruma sağlamaz; asıl koruma her zaman güçlü ve uzun bir paroladır.
3. **Güvenli Yedekleme:** Üretilen metin dosyaları UTF-8 uyumlu standart semboller içerir; bulutta, e-posta taslaklarında veya harici disklerde güvenle saklanabilir.

---

## 📄 Lisans / License

Bu proje [MIT Lisansı](LICENSE) kapsamında açık kaynak olarak sunulmaktadır.
