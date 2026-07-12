# Projek Akhir WiFi Analyze

Program analisis jaringan WiFi untuk tugas akhir Algoritma Pemrograman. Program
ini dibuat khusus untuk Windows karena proses scan menggunakan perintah
`netsh`.

## Struktur Folder

```text
Windows/
  build.bat
  wifi_analyze_windows.cpp
  wifi_analyze_windows.exe
```

## Cara Menjalankan

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

## Fitur

1. Scan jaringan WiFi
2. Mengurutkan kekuatan sinyal dengan selection sort
3. Memberikan rekomendasi kanal 2.4 GHz
4. Mencari jaringan dengan linear search
