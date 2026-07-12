# Projek Akhir WiFi Analyze

Program C++ untuk tugas akhir mata kuliah Algoritma Pemrograman. Program dapat
memindai jaringan WiFi, mengurutkan kekuatan sinyal dengan selection sort,
memberikan rekomendasi kanal, dan mencari SSID dengan linear search.

## File program

Gunakan file `Wifi Analyze.cpp`. Sebelumnya terdapat dua file `.cpp` yang
masing-masing memiliki fungsi `main()`, sehingga keduanya merupakan dua versi
program yang berbeda dan tidak boleh dikompilasi menjadi satu secara bersamaan.
Versi sederhana sudah dijadikan file utama agar struktur kode lebih mudah
dipahami.

## Cara compile

Program scan WiFi menggunakan perintah `netsh`, sehingga fitur utamanya hanya
dapat digunakan di Windows.

```bash
g++ "Wifi Analyze.cpp" -o wifi_analyze.exe
```

Jalankan hasil compile melalui Command Prompt:

```bash
wifi_analyze.exe
```

Pastikan WiFi aktif. Jika jaringan tidak muncul, coba jalankan Command Prompt
sebagai Administrator.
