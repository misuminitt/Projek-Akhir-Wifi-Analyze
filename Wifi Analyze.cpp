/*
=====================================================================
 PROGRAM ANALISIS JARINGAN WIFI
=====================================================================
 Fitur:
 1. Scan jaringan WiFi sekitar
 2. Analisis kekuatan sinyal (RSSI) - menggunakan SELECTION SORT
 3. Rekomendasi kanal terbaik (mengurangi gangguan/interferensi)
 4. Informasi detail jaringan - menggunakan LINEAR SEARCH (cari SSID)
 5. Exit

 CATATAN PENTING:
 - Program ini HANYA berjalan di Windows, karena memanfaatkan
   perintah bawaan Windows: "netsh wlan show networks mode=bssid"
 - Pastikan adapter WiFi laptop/PC dalam keadaan ON.
 - Compile dengan g++ (MinGW) atau Visual Studio (MSVC):
     g++ wifi_analyzer.cpp -o wifi_analyzer.exe
 - Jika hasil scan kosong, coba jalankan terminal/CMD sebagai
   Administrator, lalu jalankan program dari sana.
=====================================================================
*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <cctype>
#include <climits>
#include <cstdlib>

#ifdef _WIN32
#define POPEN  _popen
#define PCLOSE _pclose
#else
#define POPEN  popen
#define PCLOSE pclose
#endif

using namespace std;

// =====================================================================
// STRUKTUR DATA JARINGAN WIFI
// =====================================================================
struct WifiNetwork {
    string ssid;            // Nama jaringan
    string bssid;            // Alamat MAC access point
    string authentication;   // Jenis keamanan (WPA2/WPA3/dst)
    string encryption;       // Jenis enkripsi
    string radioType;        // 802.11n / ac / ax dst
    string band;              // 2.4 GHz / 5 GHz / 6 GHz
    string bandwidth;        // Perkiraan lebar kanal
    int channel = 0;          // Nomor kanal
    int signalPercent = 0;    // Kekuatan sinyal dalam persen (0-100)
    int rssi = 0;              // Perkiraan RSSI dalam dBm
};

// =====================================================================
// FUNGSI BANTU (HELPER)
// =====================================================================

// Menjalankan command di terminal/CMD dan mengambil hasil outputnya
string runCommand(const string &cmd) {
    string result;
    char buffer[256];
    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) return result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    PCLOSE(pipe);
    return result;
}

// Menghilangkan spasi/whitespace di awal & akhir string
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Mengambil nilai setelah tanda ":" pada sebuah baris teks
string getValueAfterColon(const string &line) {
    size_t pos = line.find(":");
    if (pos == string::npos) return "";
    return trim(line.substr(pos + 1));
}

// Menentukan band frekuensi berdasarkan nomor kanal
string determineBand(int channel) {
    if (channel >= 1 && channel <= 14)        return "2.4 GHz";
    else if (channel >= 32 && channel <= 177) return "5 GHz";
    else if (channel > 177)                   return "6 GHz";
    else                                        return "Unknown";
}

// Memperkirakan bandwidth berdasarkan jenis radio (standar WiFi)
string estimateBandwidth(const string &radioType) {
    if (radioType.find("ax") != string::npos) return "20/40/80/160 MHz (Wi-Fi 6/6E)";
    if (radioType.find("ac") != string::npos) return "20/40/80/160 MHz (Wi-Fi 5)";
    if (radioType.find("n")  != string::npos) return "20/40 MHz (Wi-Fi 4)";
    if (radioType.find("g")  != string::npos) return "20 MHz (Wi-Fi 3)";
    if (radioType.find("b")  != string::npos) return "20 MHz (Wi-Fi 1)";
    return "Tidak diketahui";
}

// Konversi sinyal persen (%) ke perkiraan RSSI (dBm)
// Rumus umum: RSSI = (persen / 2) - 100
int signalToRSSI(int percent) {
    return (percent / 2) - 100;
}

// Kategori kualitas sinyal berdasarkan RSSI
string signalQuality(int rssi) {
    if (rssi >= -50) return "Sangat Baik";
    if (rssi >= -60) return "Baik";
    if (rssi >= -70) return "Cukup";
    if (rssi >= -80) return "Lemah";
    return "Sangat Lemah";
}

// Membuat visualisasi bar sinyal seperti [#####-----]
string signalBar(int percent) {
    int filled = percent / 10;
    string bar = "[";
    for (int i = 0; i < 10; i++) bar += (i < filled) ? "#" : "-";
    bar += "]";
    return bar;
}

// =====================================================================
// FITUR 1: SCAN JARINGAN WIFI
// =====================================================================
vector<WifiNetwork> scanWifi() {
    vector<WifiNetwork> networks;
    string output = runCommand("netsh wlan show networks mode=bssid");

    if (output.empty()) {
        cout << "Gagal mendapatkan data WiFi. Pastikan WiFi aktif dan\n";
        cout << "coba jalankan program sebagai Administrator.\n";
        return networks;
    }

    istringstream iss(output);
    string line;
    string currentSSID, currentAuth, currentEnc;

    while (getline(iss, line)) {
        string trimmed = trim(line);

        if (trimmed.rfind("SSID", 0) == 0 && trimmed.find(":") != string::npos) {
            currentSSID = getValueAfterColon(trimmed);
        }
        else if (trimmed.rfind("Authentication", 0) == 0) {
            currentAuth = getValueAfterColon(trimmed);
        }
        else if (trimmed.rfind("Encryption", 0) == 0) {
            currentEnc = getValueAfterColon(trimmed);
        }
        else if (trimmed.rfind("BSSID", 0) == 0 && trimmed.find(":") != string::npos) {
            WifiNetwork net;
            net.ssid = currentSSID.empty() ? "(Jaringan Tersembunyi)" : currentSSID;
            net.bssid = getValueAfterColon(trimmed);
            net.authentication = currentAuth;
            net.encryption = currentEnc;
            networks.push_back(net);
        }
        else if (trimmed.rfind("Signal", 0) == 0 && !networks.empty()) {
            string val = getValueAfterColon(trimmed);
            val.erase(remove(val.begin(), val.end(), '%'), val.end());
            try {
                networks.back().signalPercent = atoi(val.c_str());
                networks.back().rssi = signalToRSSI(networks.back().signalPercent);
            } catch (...) {}
        }
        else if (trimmed.rfind("Radio type", 0) == 0 && !networks.empty()) {
            networks.back().radioType = getValueAfterColon(trimmed);
            networks.back().bandwidth = estimateBandwidth(networks.back().radioType);
        }
        else if (trimmed.rfind("Channel", 0) == 0 && !networks.empty()) {
            string val = getValueAfterColon(trimmed);
            try {
                networks.back().channel = stoi(val);
                networks.back().band = determineBand(networks.back().channel);
            } catch (...) {}
        }
    }

    return networks;
}

// =====================================================================
// FITUR 2: SELECTION SORT BERDASARKAN KEKUATAN SINYAL (DESCENDING)
// =====================================================================
void selectionSortBySignal(vector<WifiNetwork> &nets) {
    int n = (int)nets.size();
    for (int i = 0; i < n - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < n; j++) {
            if (nets[j].signalPercent > nets[maxIdx].signalPercent) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) swap(nets[i], nets[maxIdx]);
    }
}

// =====================================================================
// FITUR 4 (PENDUKUNG): LINEAR SEARCH BERDASARKAN SSID
// =====================================================================
vector<int> linearSearchBySSID(const vector<WifiNetwork> &nets, const string &keyword) {
    vector<int> indices;
    string lowerKeyword = keyword;
    transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    for (int i = 0; i < (int)nets.size(); i++) {
        string lowerSSID = nets[i].ssid;
        transform(lowerSSID.begin(), lowerSSID.end(), lowerSSID.begin(), ::tolower);
        if (lowerSSID.find(lowerKeyword) != string::npos) {
            indices.push_back(i);
        }
    }
    return indices;
}

// =====================================================================
// FITUR 3: REKOMENDASI KANAL TERBAIK
// =====================================================================
void recommendBestChannel(const vector<WifiNetwork> &nets) {
    int count24[15] = {0};   // kanal 1-14 (2.4 GHz)
    int count5[200] = {0};   // kanal sampai 177 (5 GHz)

    for (const auto &net : nets) {
        if (net.band == "2.4 GHz" && net.channel >= 1 && net.channel <= 14) {
            count24[net.channel]++;
        }
        else if (net.band == "5 GHz" && net.channel >= 1 && net.channel < 200) {
            count5[net.channel]++;
        }
    }

    // --- Analisis 2.4 GHz ---
    cout << "\n=== Analisis Kepadatan Kanal 2.4 GHz ===\n";
    bool any24 = false;
    for (int ch = 1; ch <= 13; ch++) {
        if (count24[ch] > 0) {
            cout << "Kanal " << ch << " : " << count24[ch] << " jaringan\n";
            any24 = true;
        }
    }
    if (!any24) cout << "Tidak ada jaringan 2.4 GHz terdeteksi.\n";

    // Kanal 1, 6, 11 dipakai karena tidak saling overlap (standar industri)
    int nonOverlap[3] = {1, 6, 11};
    int bestChannel = nonOverlap[0];
    int minCount = count24[nonOverlap[0]];
    for (int i = 1; i < 3; i++) {
        if (count24[nonOverlap[i]] < minCount) {
            minCount = count24[nonOverlap[i]];
            bestChannel = nonOverlap[i];
        }
    }
    cout << "\n>> Rekomendasi kanal terbaik 2.4 GHz : Kanal " << bestChannel
         << " (hanya " << minCount << " jaringan memakai kanal ini)\n";
    cout << "   (Kanal 1, 6, dan 11 dipilih karena tidak saling overlap)\n";

    // --- Analisis 5 GHz ---
    cout << "\n=== Analisis Kepadatan Kanal 5 GHz ===\n";
    bool any5 = false;
    for (int ch = 36; ch <= 165; ch++) {
        if (count5[ch] > 0) {
            cout << "Kanal " << ch << " : " << count5[ch] << " jaringan\n";
            any5 = true;
        }
    }

    if (any5) {
        int standar5[] = {36,40,44,48,52,56,60,64,100,104,108,112,
                           116,120,124,128,132,136,140,144,149,153,157,161,165};
        int best5Channel = standar5[0];
        int min5Count = count5[standar5[0]];
        for (int c : standar5) {
            if (count5[c] < min5Count) {
                min5Count = count5[c];
                best5Channel = c;
            }
        }
        cout << "\n>> Rekomendasi kanal terbaik 5 GHz : Kanal " << best5Channel
             << " (" << min5Count << " jaringan memakai kanal ini)\n";
    } else {
        cout << "Tidak ada jaringan 5 GHz terdeteksi.\n";
    }
}

// =====================================================================
// FUNGSI TAMPILAN (OUTPUT)
// =====================================================================
void printNetworkTable(const vector<WifiNetwork> &nets) {
    cout << "\n" << left
         << setw(4)  << "No"
         << setw(25) << "SSID"
         << setw(20) << "BSSID"
         << setw(8)  << "Sinyal"
         << setw(8)  << "Kanal"
         << setw(10) << "Band"
         << setw(20) << "Keamanan" << "\n";
    cout << string(95, '-') << "\n";

    for (size_t i = 0; i < nets.size(); i++) {
        cout << left
             << setw(4)  << (i + 1)
             << setw(25) << nets[i].ssid
             << setw(20) << nets[i].bssid
             << setw(8)  << (to_string(nets[i].signalPercent) + "%")
             << setw(8)  << nets[i].channel
             << setw(10) << nets[i].band
             << setw(20) << nets[i].authentication << "\n";
    }
    cout << "\nTotal jaringan ditemukan: " << nets.size() << "\n";
}

void printSignalAnalysis(const vector<WifiNetwork> &nets) {
    cout << "\n=== Analisis Kekuatan Sinyal (diurutkan dengan SELECTION SORT) ===\n\n";
    for (size_t i = 0; i < nets.size(); i++) {
        cout << (i + 1) << ". " << nets[i].ssid << "  (" << nets[i].bssid << ")\n";
        cout << "   Sinyal : " << nets[i].signalPercent << "%  " << signalBar(nets[i].signalPercent) << "\n";
        cout << "   RSSI   : " << nets[i].rssi << " dBm  (" << signalQuality(nets[i].rssi) << ")\n";
        cout << "   Kanal  : " << nets[i].channel << " (" << nets[i].band << ")\n\n";
    }
}

void printNetworkDetail(const WifiNetwork &net) {
    cout << "\n------------------------------------------------\n";
    cout << "SSID          : " << net.ssid << "\n";
    cout << "BSSID (MAC)   : " << net.bssid << "\n";
    cout << "Frekuensi     : " << net.band << "\n";
    cout << "Kanal         : " << net.channel << "\n";
    cout << "Keamanan      : " << net.authentication << " / " << net.encryption << "\n";
    cout << "Radio Type    : " << net.radioType << "\n";
    cout << "Bandwidth     : " << net.bandwidth << "\n";
    cout << "Sinyal        : " << net.signalPercent << "%\n";
    cout << "RSSI Estimasi : " << net.rssi << " dBm  (" << signalQuality(net.rssi) << ")\n";
    cout << "------------------------------------------------\n";
}

// =====================================================================
// PROGRAM UTAMA (MENU)
// =====================================================================
int main() {
    vector<WifiNetwork> networks;
    int choice;

    do {
        cout << "\n================================================\n";
        cout << "      PROGRAM ANALISIS JARINGAN WIFI\n";
        cout << "================================================\n";
        cout << "1. Scan Jaringan WiFi\n";
        cout << "2. Analisis Kekuatan Sinyal (RSSI)\n";
        cout << "3. Rekomendasi Kanal Terbaik\n";
        cout << "4. Informasi Jaringan (Cari berdasarkan SSID)\n";
        cout << "5. Exit\n";
        cout << "================================================\n";
        cout << "Pilih menu (1-5): ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "\nInput tidak valid, masukkan angka 1-5.\n";
            continue;
        }

        switch (choice) {
            case 1: {
                cout << "\nMemindai jaringan WiFi di sekitar...\n";
                networks = scanWifi();
                if (networks.empty()) {
                    cout << "Tidak ada jaringan ditemukan atau scan gagal.\n";
                } else {
                    printNetworkTable(networks);
                }
                break;
            }
            case 2: {
                if (networks.empty()) {
                    cout << "\nSilakan scan jaringan WiFi terlebih dahulu (menu 1).\n";
                } else {
                    vector<WifiNetwork> sorted = networks;
                    selectionSortBySignal(sorted);
                    printSignalAnalysis(sorted);
                }
                break;
            }
            case 3: {
                if (networks.empty()) {
                    cout << "\nSilakan scan jaringan WiFi terlebih dahulu (menu 1).\n";
                } else {
                    recommendBestChannel(networks);
                }
                break;
            }
            case 4: {
                if (networks.empty()) {
                    cout << "\nSilakan scan jaringan WiFi terlebih dahulu (menu 1).\n";
                } else {
                    cin.ignore();
                    cout << "\nMasukkan nama SSID (boleh sebagian) yang ingin dicari: ";
                    string keyword;
                    getline(cin, keyword);

                    vector<int> result = linearSearchBySSID(networks, keyword);
                    if (result.empty()) {
                        cout << "Jaringan dengan SSID mengandung \"" << keyword << "\" tidak ditemukan.\n";
                    } else {
                        cout << "\nDitemukan " << result.size() << " hasil:\n";
                        for (int idx : result) {
                            printNetworkDetail(networks[idx]);
                        }
                    }
                }
                break;
            }
            case 5:
                cout << "\nTerima kasih telah menggunakan program ini. Sampai jumpa!\n";
                break;
            default:
                cout << "\nPilihan tidak valid. Silakan pilih menu 1-5.\n";
        }

    } while (choice != 5);

    return 0;
}
