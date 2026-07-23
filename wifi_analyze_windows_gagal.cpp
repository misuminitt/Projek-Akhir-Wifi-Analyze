#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <iomanip>
#include <windows.h>

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

void tampilkanTabel(const vector<Wifi> &daftar) {
    cout << "\n+----+----------------------------------+--------+---------+----------+----------------------+\n";
    cout << "| " << left << setw(2) << "No"
         << " | " << setw(32) << "SSID"
         << " | " << setw(6) << "Sinyal"
         << " | " << setw(7) << "Channel"
         << " | " << setw(8) << "Band"
         << " | " << setw(20) << "Keamanan" << " |\n";
    cout << "+----+----------------------------------+--------+---------+----------+----------------------+\n";

    for (size_t i = 0; i < daftar.size(); i++) {
        ostringstream sinyal;
        sinyal << daftar[i].sinyal << "%";

        cout << "| " << left << setw(2) << (i + 1)
             << " | " << setw(32) << potongTeks(daftar[i].ssid, 32)
             << " | " << setw(6) << sinyal.str()
             << " | " << setw(7) << daftar[i].channel
             << " | " << setw(8) << potongTeks(daftar[i].band, 8)
             << " | " << setw(20) << potongTeks(daftar[i].keamanan, 20) << " |\n";
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

// Fungsi untuk memanggil ESP8266
void panggilESP8266() {
    cout << "\n=== Memanggil ESP8266 ===\n";
    cout << "Membuka koneksi serial ke ESP8266...\n";
    cout << "ESP8266 aktif (display mati untuk stealth mode)\n";
    cout << "Memonitor status ESP8266...\n";
    cout << "Tekan Ctrl+C untuk berhenti\n\n";
    
    // Membuka serial port (COM3 - sesuaikan dengan port ESP8266 Anda)
    HANDLE hSerial = CreateFile("COM3", 
                                GENERIC_READ,
                                0,
                                0,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                0);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        cout << "Gagal membuka port serial COM3.\n";
        cout << "Pastikan ESP8266 terhubung dan sesuaikan port serial.\n";
        return;
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        cout << "Gagal mendapatkan pengaturan serial.\n";
        CloseHandle(hSerial);
        return;
    }
    
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        cout << "Gagal mengatur parameter serial.\n";
        CloseHandle(hSerial);
        return;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    SetCommTimeouts(hSerial, &timeouts);
    
    char buffer[256];
    DWORD bytesRead;
    
    cout << "Status ESP8266:\n";
    cout << "---------------------------\n";
    
    for (int i = 0; i < 10; i++) {
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                string output(buffer);
                
                // Bersihkan output
                output.erase(remove(output.begin(), output.end(), '\r'), output.end());
                output.erase(remove(output.begin(), output.end(), '\n'), output.end());
                
                if (output.find("GOOD") != string::npos) {
                    cout << "[OK] ESP8266 Terhubung\n";
                } else if (output.find("BAD") != string::npos) {
                    cout << "[!] ESP8266 Terputus\n";
                } else if (output.length() > 0) {
                    cout << "[INFO] " << output << "\n";
                }
            }
        }
        Sleep(1000);
    }
    
    cout << "\nESP8266 aktif dan berjalan di stealth mode.\n";
    cout << "Display mati untuk menghindari deteksi.\n";
    
    CloseHandle(hSerial);
}

// Fungsi untuk menyalakan ESP8266
void nyalakanESP8266() {
    cout << "\n=== Menyalakan ESP8266 ===\n";
    cout << "Mengirim perintah untuk menyalakan ESP8266...\n";
    
    // Membuka serial port untuk menulis
    HANDLE hSerial = CreateFile("COM3", 
                                GENERIC_WRITE,
                                0,
                                0,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                0);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        cout << "Gagal membuka port serial COM3.\n";
        cout << "Pastikan ESP8266 terhubung dan sesuaikan port serial.\n";
        return;
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        cout << "Gagal mendapatkan pengaturan serial.\n";
        CloseHandle(hSerial);
        return;
    }
    
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        cout << "Gagal mengatur parameter serial.\n";
        CloseHandle(hSerial);
        return;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    SetCommTimeouts(hSerial, &timeouts);
    
    // Mengirim perintah untuk menyalakan display
    const char* command = "WAKEUP\n";
    DWORD bytesWritten;
    
    if (WriteFile(hSerial, command, strlen(command), &bytesWritten, NULL)) {
        cout << "Perintah terkirim: " << command;
        cout << "ESP8266 akan menyala...\n";
        cout << "Display akan aktif dalam beberapa detik.\n";
    } else {
        cout << "Gagal mengirim perintah ke ESP8266.\n";
    }
    
    CloseHandle(hSerial);
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
        cout << "5. Panggil ESP8266 (Stealth Mode)\n";
        cout << "6. Nyalakan ESP8266\n";
        cout << "7. Exit\n";
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
            panggilESP8266();
        }
        else if (pilihan == 6) {
            nyalakanESP8266();
        }
        else if (pilihan == 7) {
            cout << "\nTerima kasih!\n";
        }
        else {
            cout << "\nPilihan tidak valid.\n";
        }

    } while (pilihan != 7);

    return 0;
}
