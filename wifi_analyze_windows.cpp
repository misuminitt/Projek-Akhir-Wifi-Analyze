#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <cctype>

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

int konversiAngkakeInt(string s) {
    int hasil = 0;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isdigit((unsigned char)c)) {
            hasil = hasil * 10 + (c - '0');
        }
    }
    return hasil;
}

string ambilteksSeteleahtitikdua(const string &baris) {
    size_t pos = baris.find(":");
    if (pos == string::npos) return "";
    string hasil = baris.substr(pos + 1);
    size_t awal = hasil.find_first_not_of(" \t\r\n");
    size_t akhir = hasil.find_last_not_of(" \t\r\n");
    if (awal == string::npos) return "";
    return hasil.substr(awal, akhir - awal + 1);
}

string band(int channel) {
    if (channel >= 1 && channel <= 14) return "2.4 GHz";
    if (channel >= 32) return "5 GHz";
    return "Unknown";
}

vector<Wifi> scanWifi() {
    vector<Wifi> daftar;

    FILE* pipe = _popen("netsh wlan show networks mode=bssid", "r");
    if (!pipe) {
        cout << "Gagal menjalankan scan WiFi\n";
        return daftar;
    }

    string outputSemua;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        outputSemua += buffer;
    }
    _pclose(pipe);

    istringstream iss(outputSemua);
    string baris;
    string ssidSekarang, authSekarang;

    while (getline(iss, baris)) {
        size_t awal = baris.find_first_not_of(" \t\r\n");
        string trim = (awal == string::npos) ? "" : baris.substr(awal);

        if (trim.rfind("SSID", 0) == 0 && trim.find(":") != string::npos) {
            ssidSekarang = ambilteksSeteleahtitikdua(trim);
        }
        else if (trim.rfind("Authentication", 0) == 0) {
            authSekarang = ambilteksSeteleahtitikdua(trim);
        }
        else if (trim.rfind("BSSID", 0) == 0 && trim.find(":") != string::npos) {
            Wifi w;
            w.ssid = ssidSekarang.empty() ? "(Hidden)" : ssidSekarang;
            w.bssid = ambilteksSeteleahtitikdua(trim);
            w.keamanan = authSekarang;
            w.channel = 0;
            w.sinyal = 0;
            w.rssi = 0;
            w.band = "Unknown";
            daftar.push_back(w);
        }
        else if (trim.rfind("Signal", 0) == 0 && !daftar.empty()) {
            string nilai = ambilteksSeteleahtitikdua(trim);
            daftar.back().sinyal = konversiAngkakeInt(nilai);
            daftar.back().rssi = (daftar.back().sinyal / 2) - 100;
        }
        else if (trim.rfind("Channel", 0) == 0 && !daftar.empty()) {
            string nilai = ambilteksSeteleahtitikdua(trim);
            daftar.back().channel = konversiAngkakeInt(nilai);
            daftar.back().band = band(daftar.back().channel);
        }
    }

    return daftar;
}

void manualSwap(Wifi &a, Wifi &b) {
    Wifi sementara = a;
    a = b;
    b = sementara;
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
        if (idxMax != i) manualSwap(daftar[i], daftar[idxMax]);
    }
}

void sortBySSID(vector<Wifi> &daftar) {
    int n = daftar.size();
    for (int i = 0; i < n - 1; i++) {
        int idxMin = i;
        for (int j = i + 1; j < n; j++) {
            if (daftar[j].ssid < daftar[idxMin].ssid) {
                idxMin = j;
            }
        }
        if (idxMin != i) manualSwap(daftar[i], daftar[idxMin]);
    }
}

int cariSSID(const vector<Wifi> &daftar, const string &keyword) {
    int kiri = 0;
    int kanan = static_cast<int>(daftar.size()) - 1;

    while (kiri <= kanan) {
        int tengah = kiri + (kanan - kiri) / 2;

        if (daftar[tengah].ssid == keyword) {
            return tengah;
        }

        if (daftar[tengah].ssid < keyword) {
            kiri = tengah + 1;
        } else {
            kanan = tengah - 1;
        }
    }

    return -1;
}

void rekomendasiChannel(const vector<Wifi> &daftar) {
    int hitung24[15] = {0};

    for (size_t i = 0; i < daftar.size(); i++) {
        if (daftar[i].band == "2.4 GHz" && daftar[i].channel >= 1 && daftar[i].channel <= 14) {
            hitung24[daftar[i].channel]++;
        }
    }

    cout << "\n=== Kepadatan Channel 2.4 GHz ===\n";
    for (int ch = 1; ch <= 13; ch++) {
        if (hitung24[ch] > 0) {
            cout << "Channel " << ch << " : " << hitung24[ch] << " jaringan\n";
        }
    }

    int ChannelTerbaik = 1;
    int jumlahMin = hitung24[1];
    if (hitung24[6] < jumlahMin)  { jumlahMin = hitung24[6];  ChannelTerbaik = 6; }
    if (hitung24[11] < jumlahMin) { jumlahMin = hitung24[11]; ChannelTerbaik = 11; }

    cout << "\n>> Rekomendasi Channel terbaik (2.4 GHz): Channel " << ChannelTerbaik
         << " (hanya " << jumlahMin << " jaringan memakainya)\n";
}

string potongTeks(const string &teks, size_t panjang) {
    if (teks.length() <= panjang) return teks;
    if (panjang <= 3) return teks.substr(0, panjang);
    return teks.substr(0, panjang - 3) + "...";
}

string padKanan(const string &teks, size_t lebar) {
    if (teks.length() >= lebar) return teks;
    return teks + string(lebar - teks.length(), ' ');
}

void tampilkanTabel(const vector<Wifi> &daftar) {
    cout << "\n+----+----------------------------------+--------+---------+----------+----------------------+\n";
    cout << "| " << padKanan("No", 2)
         << " | " << padKanan("SSID", 32)
         << " | " << padKanan("Sinyal", 6)
         << " | " << padKanan("Channel", 7)
         << " | " << padKanan("Band", 8)
         << " | " << padKanan("Keamanan", 20) << " |\n";
    cout << "+----+----------------------------------+--------+---------+----------+----------------------+\n";

    for (size_t i = 0; i < daftar.size(); i++) {
        ostringstream sinyal;
        sinyal << daftar[i].sinyal << "%";

        ostringstream nomor;
        nomor << (i + 1);

        ostringstream channel;
        channel << daftar[i].channel;

        cout << "| " << padKanan(nomor.str(), 2)
             << " | " << padKanan(potongTeks(daftar[i].ssid, 32), 32)
             << " | " << padKanan(sinyal.str(), 6)
             << " | " << padKanan(channel.str(), 7)
             << " | " << padKanan(potongTeks(daftar[i].band, 8), 8)
             << " | " << padKanan(potongTeks(daftar[i].keamanan, 20), 20) << " |\n";
    }

    cout << "+----+----------------------------------+--------+---------+----------+----------------------+\n";
    cout << "\nTotal jaringan: " << daftar.size() << "\n";
}

void tampilkanDetail(const Wifi &w) {
    cout << "\n------------------------------\n";
    cout << "SSID      : " << w.ssid << "\n";
    cout << "BSSID     : " << w.bssid << "\n";
    cout << "Band      : " << w.band << "\n";
    cout << "Channel   : " << w.channel << "\n";
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
        cout << "3. Rekomendasi Channel Terbaik\n";
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
                rekomendasiChannel(daftarWifi);
            }
        }
        else if (pilihan == 4) {
            if (daftarWifi.empty()) {
                cout << "\nScan WiFi dulu (menu 1).\n";
            } else {
                sortBySSID(daftarWifi);
                cout << "\nMasukkan SSID yang dicari: ";
                string keyword;
                cin.ignore();
                getline(cin, keyword);

                int idx = cariSSID(daftarWifi, keyword);
                if (idx == -1) {
                    cout << "Jaringan tidak ditemukan.\n";
                } else {
                    cout << "\n=== Hasil pencarian (Binary Search) ===\n";
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
