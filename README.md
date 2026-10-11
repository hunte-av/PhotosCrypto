# PhotosCrypto

[English] PhotosCrypto is a modern C++17 application created to keep your photos and private files safe from curious eyes. It compresses your files to save disk space and encrypts them into safe text files. You can choose to protect your files with a strong password or save them without a password for fast one-click recovery.

[Turkce] PhotosCrypto, fotograflarinizi ve ozel dosyalarinizi merakli gozlerden korumak icin gelistirilmis modern bir C++17 uygulamasidir. Dosyalarinizi diskte daha az yer kaplamasi icin sikistirir ve guvenli metin dosyalarina donusturerek sifreler. Dosyalarinizi guclu bir sifreyle korumayi secebilir ya da hizli erisim icin sifresiz olarak kaydedebilirsiniz.

---

## Detailed Features

[English]
1. Privacy and Security: Your files are secured using modern AES-256-GCM encryption and the scrypt key algorithm. Without the correct password, nobody can read or open your files.
2. Built-in Data Compression: Every file is automatically compressed with zlib before encryption. This reduces the file size by 40 percent to 80 percent on typical files, saving significant storage space.
3. Optional Password Protection: Setting a password is completely your choice. You can add a strong password or leave it empty for easy one-click restoration.
4. Smart Format Detection: When restoring an encoded file, PhotosCrypto automatically recognizes the original file format such as PNG, JPG, WEBP, GIF, or BMP and restores it with the correct extension.
5. Visual Password Shield: The user interface includes a dynamic 5-level security shield that guides you to create strong passwords using letters, numbers, and symbols.
6. Clean Desktop and Command-Line Interfaces: You can use the modern GTK4 graphical interface or use the lightweight command-line tool in automated scripts.

[Turkce]
1. Gizlilik ve Guvenlik: Dosyalariniz modern AES-256-GCM sifreleme ve scrypt anahtar algoritmasi ile korunur. Dogru sifre olmadan hic kimse dosyalarinizi okuyamaz veya acamaz.
2. Dahili Veri Sikistirma: Her dosya sifrelenmeden once zlib ile otomatik olarak sikistirilir. Bu islem yaygin dosyalarda boyutu yuzde 40 ile yuzde 80 arasinda kuculterek onemli olcude depolama alani kazandirir.
3. Istege Bagli Sifre Korumasi: Sifre belirlemek tamamen sizin tercihinize baglidir. Guclu bir sifre ekleyebilir veya tek tikla kolayca geri yuklemek icin sifresiz kaydedebilirsiniz.
4. Akilli Format Tanima: Sifrelenmis bir dosyayi geri cevirirken PhotosCrypto PNG, JPG, WEBP, GIF veya BMP gibi orijinal dosya turunu otomatik olarak tanir ve dogru uzantiyla kaydeder.
5. Gorsel Guvenlik Kalkani: Kullanici arayuzu, harf, rakam ve semboller kullanarak guclu sifreler olusturmaniza rehberlik eden 5 asamali dinamik bir guvenlik kalkani icerir.
6. Sade Masaustu ve Komut Satiri Arayuzu: Modern GTK4 grafik arayuzunu kullanabilir veya komut satiri aracini otomatik scriptlerde calistirabilirsiniz.

---

## Performance and Architecture

[English]
- High Speed: Core operations are written in native C++17 without heavy dependencies. Compression and encryption run in streaming memory buffers to ensure high performance on large photos.
- Memory Efficiency: Memory allocations are controlled and cleaned up immediately after operations.
- Strong Cryptography: Uses industry-standard OpenSSL libcrypto for authenticated encryption. If any bit in the file is modified or corrupted, PhotosCrypto detects it instantly and stops processing to protect your system.
- Safe Storage: Encrypted outputs are created with restricted file permissions so other user accounts cannot read them.

[Turkce]
- Yuksek Hiz: Cekirdek islemler agir bagimliliklar olmadan dogrudan C++17 ile yazilmistir. Sikistirma ve sifreleme bellek tamponlari uzerinde hizla calisarak buyuk fotograflarda bile yuksek performans saglar.
- Bellek Verimliligi: Bellek kullanimi kontrol altinda tutulur ve islemler biter bitmez guvenle temizlenir.
- Guclu Kriptografi: Kimlik dogrulamali sifreleme icin endustri standardi OpenSSL libcrypto kullanilir. Dosyadaki tek bir bayt bile degistirilirse veya bozulursa PhotosCrypto bunu aninda fark eder ve guvenliginizi korumak icin islemi durdurur.
- Guvenli Depolama: Sifrelenen dosyalar kisitli izinlerle olusturulur, boylece diger kullanici hesaplari bu dosyalari okuyamaz.

---

## Installation Guide

### 1. Linux

#### Method A: Flatpak (Recommended)

[English] The easiest way to install and run PhotosCrypto on any Linux distribution is via Flatpak.

[Turkce] Herhangi bir Linux dagitiminda PhotosCrypto uygulamasini kurup calistirmanın en kolay yolu Flatpak kullanmaktir.

```bash
git clone https://github.com/hunte-av/PhotosCrypto.git
cd PhotosCrypto
flatpak install flathub org.freedesktop.Sdk//25.08
flatpak-builder --user --install --force-clean build-dir io.github.hunte_av.PhotosCrypto.yml
flatpak run io.github.hunte_av.PhotosCrypto
```

#### Method B: Build from Source on Linux

[English] Install the required compiler, OpenSSL, zlib, GTK4, and Libadwaita development packages.

[Turkce] Gerekli derleyici, OpenSSL, zlib, GTK4 ve Libadwaita gelistirme paketlerini yukleyin.

- Ubuntu / Debian:
```bash
sudo apt update
sudo apt install build-essential pkg-config libssl-dev zlib1g-dev libgtk-4-dev libadwaita-1-dev
```

- Fedora:
```bash
sudo dnf install gcc-c++ make pkgconf openssl-devel zlib-devel gtk4-devel libadwaita-devel
```

- Arch Linux:
```bash
sudo pacman -S base-devel openssl zlib gtk4 libadwaita
```

- Build and Install:
```bash
make
sudo make install
photoscrypto-cli --help
```

---

### 2. Windows

[English]
PhotosCrypto can be built on Windows using MSYS2 or Windows Subsystem for Linux (WSL). MSYS2 provides a native Windows binary that runs directly on your system.

[Turkce]
PhotosCrypto, MSYS2 veya WSL (Windows Subsystem for Linux) kullanilarak Windows uzerinde derlenebilir. MSYS2, sisteminizde dogrudan calisan yerel bir Windows calistirilabilir dosyasi olusturur.

#### Step by Step via MSYS2 (UCRT64)

1. [English] Download and install MSYS2 from msys2.org.
   [Turkce] msys2.org adresinden MSYS2 programini indirip kurun.

2. [English] Open the MSYS2 UCRT64 terminal and install the build tools and libraries:
   [Turkce] MSYS2 UCRT64 terminalini acin ve gerekli araclar ile kutuphaneleri yukleyin:
```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-gtk4 mingw-w64-ucrt-x86_64-libadwaita git
```

3. [English] Clone the repository and compile:
   [Turkce] Projeyi indirin ve derleyin:
```bash
git clone https://github.com/hunte-av/PhotosCrypto.git
cd PhotosCrypto
mingw32-make
```

4. [English] Run the command-line tool or graphical interface:
   [Turkce] Komut satiri aracini veya grafik arayuzunu calistirin:
```bash
./photoscrypto-cli.exe --help
./photoscrypto-gui.exe
```

---

### 3. macOS

[English]
PhotosCrypto can be installed on macOS using Homebrew. The command-line tool works natively, and the graphical interface works when GTK4 and Libadwaita are installed.

[Turkce]
PhotosCrypto, Homebrew kullanilarak macOS uzerinde kurulabilir. Komut satiri araci dogrudan calisir, grafik arayuz ise GTK4 ve Libadwaita yuklendiginde calisir.

#### Step by Step via Homebrew

1. [English] Open Terminal and install dependencies using Homebrew:
   [Turkce] Terminal uygulamasini acin ve Homebrew ile bagimliliklari yukleyin:
```bash
brew install openssl zlib pkg-config make gcc gtk4 libadwaita
```

2. [English] Configure environment variables for Homebrew paths:
   [Turkce] Homebrew yollari icin ortam degiskenlerini tanimlayin:
```bash
export PKG_CONFIG_PATH="$(brew --prefix openssl)/lib/pkgconfig:$(brew --prefix zlib)/lib/pkgconfig:$PKG_CONFIG_PATH"
```

3. [English] Clone the repository and compile:
   [Turkce] Projeyi indirin ve derleyin:
```bash
git clone https://github.com/hunte-av/PhotosCrypto.git
cd PhotosCrypto
make
```

4. [English] Run the application:
   [Turkce] Uygulamayi calistirin:
```bash
./photoscrypto-cli --help
./photoscrypto-gui
```

---

## Command Line Usage Examples

### Encoding a Photo with Password

[English] Encrypts the photo with a password and compresses it into a text file.
[Turkce] Fotografi sifreler, sikistirir ve guvenli bir metin dosyasina donusturur.

```bash
photoscrypto-cli encode my_photo.png safe_note.txt
```

### Decoding a Photo with Password

[English] Restores the original photo from the encoded text file using your password.
[Turkce] Sifrenizi girerek orijinal fotografi metin dosyasindan geri cikarir.

```bash
photoscrypto-cli decode safe_note.txt restored_photo
```

### Encoding without Password (Fast Mode)

[English] Compresses and hides the photo without asking for a password.
[Turkce] Sifre sormadan fotografi sikistirir ve gizli metin dosyasina donusturur.

```bash
photoscrypto-cli encode my_photo.png safe_note.txt --no-password
```

### Decoding without Password

[English] Restores the file directly in one step.
[Turkce] Dosyayi tek adimda dogrudan geri yukler.

```bash
photoscrypto-cli decode safe_note.txt restored_photo
```

---

## Important Security Notes

[English]
- If you set a password and forget it, the photo cannot be recovered. There is no backdoor or master recovery key.
- Choose a strong password with a mix of letters, numbers, and symbols for high security.
- Encrypted files can be backed up or transferred safely as standard text files.

[Turkce]
- Eger bir sifre belirler ve bu sifreyi unutursaniz, fotograf kurtariamaz. Hicbir arka kapi veya kurtarma anahtari bulunmamaktadir.
- Yuksek guvenlik icin harf, rakam ve sembol karisimi iceren guclu bir sifre secin.
- Sifrelenmis dosyalar standart metin dosyalari olarak guvenle yedeklenebilir veya baska cihazlara aktarilabilir.

---

## License

[English] Distributed under the MIT License. See LICENSE for details.
[Turkce] MIT Lisansi ile dagitilmaktadir. Ayrintilar icin LICENSE dosyasina bakiniz.
