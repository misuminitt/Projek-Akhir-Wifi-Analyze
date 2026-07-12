# Projek Akhir WiFi Analyze

Program analisis jaringan WiFi untuk tugas akhir Algoritma Pemrograman. Source
dipisahkan berdasarkan sistem operasi karena Windows dan macOS menggunakan
perintah scan WiFi yang berbeda.

## Struktur Folder

```text
Macos/
  build.sh
  wifi_analyze_macos.cpp

Windows/
  build.bat
  wifi_analyze_windows.cpp
```

## Windows

Versi Windows menggunakan perintah:

```cmd
netsh wlan show networks mode=bssid
```

Compile melalui Command Prompt:

```cmd
cd /d "lokasi-proyek\Windows"
build.bat
```

Jalankan:

```cmd
wifi_analyze_windows.exe
```

Executable dibuat secara statis agar tidak membutuhkan `libgcc_s_seh-1.dll`
atau `libstdc++-6.dll`.

## macOS

Versi macOS menggunakan:

```bash
system_profiler SPAirPortDataType
```

Compile melalui Terminal:

```bash
cd "lokasi-proyek/Macos"
sh build.sh
```

Jalankan:

```bash
./wifi_analyze_macos
```

Pada macOS modern, sistem dapat menyamarkan nama SSID menjadi `<redacted>` dan
tidak menampilkan BSSID. Kanal, keamanan, dan RSSI tetap dibaca jika tersedia.

## Fitur

1. Scan jaringan WiFi
2. Mengurutkan kekuatan sinyal dengan selection sort
3. Memberikan rekomendasi kanal 2.4 GHz
4. Mencari jaringan dengan linear search
