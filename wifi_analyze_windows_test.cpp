#include <cassert>
#include <string>
#include <vector>

#define _popen popen
#define _pclose pclose
#define main wifi_main
#include "wifi_analyze_windows.cpp"
#undef main

int main() {
    vector<Wifi> daftar = {
        {"Zulu", "b1", "WPA2", 11, 70, -65, "2.4 GHz"},
        {"Alpha", "b2", "WPA2", 1, 90, -55, "2.4 GHz"},
        {"Bravo", "b3", "Open", 6, 80, -60, "2.4 GHz"}
    };

    sortBySSID(daftar);

    assert(daftar[0].ssid == "Alpha");
    assert(daftar[1].ssid == "Bravo");
    assert(daftar[2].ssid == "Zulu");
    assert(cariSSID(daftar, "Bravo") == 1);
    assert(cariSSID(daftar, "Missing") == -1);

    Wifi kiri = {"Kiri", "left", "WPA2", 1, 10, -95, "2.4 GHz"};
    Wifi kanan = {"Kanan", "right", "Open", 11, 90, -55, "5 GHz"};
    manualSwap(kiri, kanan);

    assert(kiri.ssid == "Kanan");
    assert(kanan.ssid == "Kiri");

    return 0;
}
