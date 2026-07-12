/*
=====================================================================
 PROGRAM ANALISIS JARINGAN WIFI
=====================================================================
 Fitur:
 1. Scan jaringan WiFi sekitar
 2. Analisis kekuatan sinyal (RSSI) - SELECTION SORT
 3. Rekomendasi kanal terbaik
 4. Informasi jaringan - LINEAR SEARCH (cari SSID)
 5. Exit

 CATATAN:
 - Fitur scan berjalan di Windows karena memakai perintah "netsh wlan show networks"
 - Compile: g++ "Wifi Analyze.cpp" -o wifi_analyze.exe
 - Jika hasil scan kosong, jalankan CMD sebagai Administrator
=====================================================================
*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#define BUKA_PIPE _popen
#define TUTUP_PIPE _pclose
#else
#define BUKA_PIPE popen
#define TUTUP_PIPE pclose
#endif

using namespace std;

// ---------------- STRUKTUR DATA ----------------
struct Wifi {
    string ssid;
    string bssid;
    string keamanan;
    int channel;
    int sinyal;   // dalam persen (0-100)
    int rssi;     // perkiraan dBm
    string band;  // 2.4 GHz / 5 GHz
};

// ---------------- FUNGSI BANTU ----------------

// Mengubah string angka jadi int, kalau gagal kembalikan 0
int toIntSafe(string s) {
    int hasil = 0;
    for (char c : s) {
        if (isdigit((unsigned char)c)) {
            hasil = hasil * 10 + (c - '0');
        }
    }
    return hasil;
}

// Mengambil teks setelah tanda ":"
string ambilSetelahTitikDua(const string &baris) {
    size_t pos = baris.find(":");
    if (pos == string::npos) return "";
    string hasil = baris.substr(pos + 1);
    // hapus spasi di awal/akhir
    size_t awal = hasil.find_first_not_of(" \t\r\n");
    size_t akhir = hasil.find_last_not_of(" \t\r\n");
    if (awal == string::npos) return "";
    return hasil.substr(awal, akhir - awal + 1);
}

// Menentukan band berdasarkan nomor kanal
string tentukanBand(int channel) {
    if (channel >= 1 && channel <= 14) return "2.4 GHz";
    if (channel >= 32) return "5 GHz";
    return "Unknown";
}

// ---------------- FITUR 1: SCAN WIFI ----------------
vector<Wifi> scanWifi() {
    vector<Wifi> daftar;

    FILE* pipe = BUKA_PIPE("netsh wlan show networks mode=bssid", "r");
    if (!pipe) {
        cout << "Gagal menjalankan scan WiFi.\n";
        return daftar;
    }

    string outputSemua;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        outputSemua += buffer;
    }
    TUTUP_PIPE(pipe);

    istringstream iss(outputSemua);
    string baris;
    string ssidSekarang, authSekarang;

    while (getline(iss, baris)) {
        // hapus spasi di awal baris untuk mempermudah pengecekan
        size_t awal = baris.find_first_not_of(" \t\r\n");
        string trim = (awal == string::npos) ? "" : baris.substr(awal);

        if (trim.rfind("SSID", 0) == 0 && trim.find(":") != string::npos) {
            ssidSekarang = ambilSetelahTitikDua(trim);
        }
        else if (trim.rfind("Authentication", 0) == 0) {
            authSekarang = ambilSetelahTitikDua(trim);
        }
        else if (trim.rfind("BSSID", 0) == 0 && trim.find(":") != string::npos) {
            Wifi w;
            w.ssid = ssidSekarang.empty() ? "(Tersembunyi)" : ssidSekarang;
            w.bssid = ambilSetelahTitikDua(trim);
            w.keamanan = authSekarang;
            w.channel = 0;
            w.sinyal = 0;
            w.rssi = 0;
            w.band = "Unknown";
            daftar.push_back(w);
        }
        else if (trim.rfind("Signal", 0) == 0 && !daftar.empty()) {
            string nilai = ambilSetelahTitikDua(trim);
            daftar.back().sinyal = toIntSafe(nilai);
            daftar.back().rssi = (daftar.back().sinyal / 2) - 100;
        }
        else if (trim.rfind("Channel", 0) == 0 && !daftar.empty()) {
            string nilai = ambilSetelahTitikDua(trim);
            daftar.back().channel = toIntSafe(nilai);
            daftar.back().band = tentukanBand(daftar.back().channel);
        }
    }

    return daftar;
}

// ---------------- FITUR 2: SELECTION SORT ----------------
void sortBySinyal(vector<Wifi> &daftar) {
    int n = daftar.size();
    for (int i = 0; i < n - 1; i++) {
        int idxMax = i;
        for (int j = i + 1; j < n; j++) {
            if (daftar[j].sinyal > daftar[idxMax].sinyal) {
                idxMax = j;
            }
        }
        if (idxMax != i) swap(daftar[i], daftar[idxMax]);
    }
}

// ---------------- FITUR 4 (pendukung): LINEAR SEARCH ----------------
int cariSSID(const vector<Wifi> &daftar, const string &keyword) {
    for (int i = 0; i < (int)daftar.size(); i++) {
        if (daftar[i].ssid.find(keyword) != string::npos) {
            return i; // ditemukan, kembalikan posisinya
        }
    }
    return -1; // tidak ditemukan
}

// ---------------- FITUR 3: REKOMENDASI KANAL ----------------
void rekomendasiKanal(const vector<Wifi> &daftar) {
    int hitung24[15] = {0}; // kanal 1-14

    for (const auto &w : daftar) {
        if (w.band == "2.4 GHz" && w.channel >= 1 && w.channel <= 14) {
            hitung24[w.channel]++;
        }
    }

    cout << "\n=== Kepadatan Kanal 2.4 GHz ===\n";
    for (int ch = 1; ch <= 13; ch++) {
        if (hitung24[ch] > 0) {
            cout << "Kanal " << ch << " : " << hitung24[ch] << " jaringan\n";
        }
    }

    // kanal 1, 6, 11 tidak saling overlap (standar)
    int kanalTerbaik = 1;
    int jumlahMin = hitung24[1];
    if (hitung24[6] < jumlahMin)  { jumlahMin = hitung24[6];  kanalTerbaik = 6; }
    if (hitung24[11] < jumlahMin) { jumlahMin = hitung24[11]; kanalTerbaik = 11; }

    cout << "\n>> Rekomendasi kanal terbaik (2.4 GHz): Kanal " << kanalTerbaik
         << " (hanya " << jumlahMin << " jaringan memakainya)\n";
}

// ---------------- TAMPILAN ----------------
void tampilkanTabel(const vector<Wifi> &daftar) {
    cout << "\nNo  SSID                      Sinyal  Kanal  Band      Keamanan\n";
    cout << "----------------------------------------------------------------\n";
    for (size_t i = 0; i < daftar.size(); i++) {
        cout << (i + 1) << ". " << daftar[i].ssid
             << "\t" << daftar[i].sinyal << "%"
             << "\t" << daftar[i].channel
             << "\t" << daftar[i].band
             << "\t" << daftar[i].keamanan << "\n";
    }
    cout << "\nTotal jaringan: " << daftar.size() << "\n";
}

void tampilkanDetail(const Wifi &w) {
    cout << "\n------------------------------\n";
    cout << "SSID      : " << w.ssid << "\n";
    cout << "BSSID     : " << w.bssid << "\n";
    cout << "Band      : " << w.band << "\n";
    cout << "Kanal     : " << w.channel << "\n";
    cout << "Keamanan  : " << w.keamanan << "\n";
    cout << "Sinyal    : " << w.sinyal << "%\n";
    cout << "RSSI      : " << w.rssi << " dBm\n";
    cout << "------------------------------\n";
}

// ---------------- PROGRAM UTAMA ----------------
int main() {
    vector<Wifi> daftarWifi;
    int pilihan;

    do {
        cout << "\n========================================\n";
        cout << "   PROGRAM ANALISIS JARINGAN WIFI\n";
        cout << "========================================\n";
        cout << "1. Scan Jaringan WiFi\n";
        cout << "2. Analisis Kekuatan Sinyal\n";
        cout << "3. Rekomendasi Kanal Terbaik\n";
        cout << "4. Cari Jaringan (berdasarkan SSID)\n";
        cout << "5. Exit\n";
        cout << "========================================\n";
        cout << "Pilih menu: ";
        cin >> pilihan;

        if (pilihan == 1) {
            cout << "\nSedang scan WiFi...\n";
            daftarWifi = scanWifi();
            if (daftarWifi.empty()) {
                cout << "Tidak ada jaringan ditemukan.\n";
            } else {
                tampilkanTabel(daftarWifi);
            }
        }
        else if (pilihan == 2) {
            if (daftarWifi.empty()) {
                cout << "\nScan WiFi dulu (menu 1).\n";
            } else {
                sortBySinyal(daftarWifi);
                cout << "\n=== Sinyal Diurutkan (Selection Sort) ===\n";
                tampilkanTabel(daftarWifi);
            }
        }
        else if (pilihan == 3) {
            if (daftarWifi.empty()) {
                cout << "\nScan WiFi dulu (menu 1).\n";
            } else {
                rekomendasiKanal(daftarWifi);
            }
        }
        else if (pilihan == 4) {
            if (daftarWifi.empty()) {
                cout << "\nScan WiFi dulu (menu 1).\n";
            } else {
                cout << "\nMasukkan SSID yang dicari: ";
                string keyword;
                cin.ignore();
                getline(cin, keyword);

                int idx = cariSSID(daftarWifi, keyword);
                if (idx == -1) {
                    cout << "Jaringan tidak ditemukan.\n";
                } else {
                    tampilkanDetail(daftarWifi[idx]);
                }
            }
        }
        else if (pilihan == 5) {
            cout << "\nTerima kasih!\n";
        }
        else {
            cout << "\nPilihan tidak valid.\n";
        }

    } while (pilihan != 5);

    return 0;
}
