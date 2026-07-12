#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <iomanip>

using namespace std;

struct Wifi {
    string ssid;
    string bssid;
    string keamanan;
    int channel;
    int sinyal;
    int rssi;
    string band;
};

int toIntSafe(string s) {
    int hasil = 0;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isdigit((unsigned char)c)) {
            hasil = hasil * 10 + (c - '0');
        }
    }
    return hasil;
}

int ambilAngkaBertanda(const string &teks) {
    istringstream input(teks);
    int angka = 0;
    input >> angka;
    return angka;
}

int rssiKePersen(int rssi) {
    if (rssi >= -50) return 100;
    if (rssi <= -100) return 0;
    return 2 * (rssi + 100);
}

string ambilSetelahTitikDua(const string &baris) {
    size_t pos = baris.find(":");
    if (pos == string::npos) return "";
    string hasil = baris.substr(pos + 1);
    size_t awal = hasil.find_first_not_of(" \t\r\n");
    size_t akhir = hasil.find_last_not_of(" \t\r\n");
    if (awal == string::npos) return "";
    return hasil.substr(awal, akhir - awal + 1);
}

string tentukanBand(int channel) {
    if (channel >= 1 && channel <= 14) return "2.4 GHz";
    if (channel >= 32) return "5 GHz";
    return "Unknown";
}

vector<Wifi> scanWifi() {
    vector<Wifi> daftar;

    FILE* pipe = popen("system_profiler SPAirPortDataType 2>/dev/null", "r");
    if (!pipe) {
        cout << "Gagal menjalankan scan WiFi.\n";
        return daftar;
    }

    string outputSemua;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        outputSemua += buffer;
    }
    pclose(pipe);

    istringstream iss(outputSemua);
    string baris;
    bool bagianJaringan = false;
    int nomorJaringan = 0;

    while (getline(iss, baris)) {
        size_t awal = baris.find_first_not_of(" \t\r\n");
        string trim = (awal == string::npos) ? "" : baris.substr(awal);

        if (trim == "Current Network Information:" || trim == "Other Local Wi-Fi Networks:") {
            bagianJaringan = true;
        }
        else if (bagianJaringan && trim == "awdl0:") {
            bagianJaringan = false;
        }
        else if (bagianJaringan && awal == 12 && !trim.empty() && trim[trim.length() - 1] == ':') {
            Wifi w;
            nomorJaringan++;
            w.ssid = trim.substr(0, trim.length() - 1);
            if (w.ssid == "<redacted>") {
                ostringstream nama;
                nama << "Jaringan " << nomorJaringan << " (SSID disamarkan macOS)";
                w.ssid = nama.str();
            }
            w.bssid = "Tidak ditampilkan macOS";
            w.keamanan = "Tidak diketahui";
            w.channel = 0;
            w.sinyal = 0;
            w.rssi = -100;
            w.band = "Unknown";
            daftar.push_back(w);
        }
        else if (bagianJaringan && trim.rfind("Security:", 0) == 0 && !daftar.empty()) {
            daftar.back().keamanan = ambilSetelahTitikDua(trim);
        }
        else if (bagianJaringan && trim.rfind("Channel:", 0) == 0 && !daftar.empty()) {
            string nilai = ambilSetelahTitikDua(trim);
            daftar.back().channel = ambilAngkaBertanda(nilai);
            daftar.back().band = tentukanBand(daftar.back().channel);
        }
        else if (bagianJaringan && trim.rfind("Signal / Noise:", 0) == 0 && !daftar.empty()) {
            string nilai = ambilSetelahTitikDua(trim);
            daftar.back().rssi = ambilAngkaBertanda(nilai);
            daftar.back().sinyal = rssiKePersen(daftar.back().rssi);
        }
    }

    return daftar;
}

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

int cariSSID(const vector<Wifi> &daftar, const string &keyword) {
    for (int i = 0; i < (int)daftar.size(); i++) {
        if (daftar[i].ssid.find(keyword) != string::npos) {
            return i;
        }
    }
    return -1;
}

void rekomendasiKanal(const vector<Wifi> &daftar) {
    int hitung24[15] = {0};

    for (size_t i = 0; i < daftar.size(); i++) {
        if (daftar[i].band == "2.4 GHz" && daftar[i].channel >= 1 && daftar[i].channel <= 14) {
            hitung24[daftar[i].channel]++;
        }
    }

    cout << "\n=== Kepadatan Kanal 2.4 GHz ===\n";
    for (int ch = 1; ch <= 13; ch++) {
        if (hitung24[ch] > 0) {
            cout << "Kanal " << ch << " : " << hitung24[ch] << " jaringan\n";
        }
    }

    int kanalTerbaik = 1;
    int jumlahMin = hitung24[1];
    if (hitung24[6] < jumlahMin)  { jumlahMin = hitung24[6];  kanalTerbaik = 6; }
    if (hitung24[11] < jumlahMin) { jumlahMin = hitung24[11]; kanalTerbaik = 11; }

    cout << "\n>> Rekomendasi kanal terbaik (2.4 GHz): Kanal " << kanalTerbaik
         << " (hanya " << jumlahMin << " jaringan memakainya)\n";
}

string potongTeks(const string &teks, size_t panjang) {
    if (teks.length() <= panjang) return teks;
    if (panjang <= 3) return teks.substr(0, panjang);
    return teks.substr(0, panjang - 3) + "...";
}

void tampilkanTabel(const vector<Wifi> &daftar) {
    cout << "\n+----+----------------------------------+--------+-------+----------+----------------------+\n";
    cout << "| " << left << setw(2) << "No"
         << " | " << setw(32) << "SSID"
         << " | " << setw(6) << "Sinyal"
         << " | " << setw(5) << "Kanal"
         << " | " << setw(8) << "Band"
         << " | " << setw(20) << "Keamanan" << " |\n";
    cout << "+----+----------------------------------+--------+-------+----------+----------------------+\n";

    for (size_t i = 0; i < daftar.size(); i++) {
        ostringstream sinyal;
        sinyal << daftar[i].sinyal << "%";

        cout << "| " << left << setw(2) << (i + 1)
             << " | " << setw(32) << potongTeks(daftar[i].ssid, 32)
             << " | " << setw(6) << sinyal.str()
             << " | " << setw(5) << daftar[i].channel
             << " | " << setw(8) << potongTeks(daftar[i].band, 8)
             << " | " << setw(20) << potongTeks(daftar[i].keamanan, 20) << " |\n";
    }

    cout << "+----+----------------------------------+--------+-------+----------+----------------------+\n";
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
