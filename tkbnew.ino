#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "BluetoothSerial.h"

String ssid = "TKB Free Wifi";

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

BluetoothSerial SerialBT;

// Fungsi untuk restart AP dengan SSID baru
void restartAP() {
  WiFi.softAPdisconnect(true); // Matikan AP lama
  WiFi.softAP(ssid.c_str(), NULL, 1); // Buat AP baru dengan SSID baru
  Serial.printf("AP Restarted with new SSID: %s\n", ssid.c_str());
  SerialBT.printf("AP Restarted with new SSID: %s\n", ssid.c_str());
}

void handlePortal() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    char macStr[18];
    sprintf(macStr,"%02X:%02X:%02X:%02X:%02X:%02X",
      info.wifi_ap_staconnected.mac[0],
      info.wifi_ap_staconnected.mac[1],
      info.wifi_ap_staconnected.mac[2],
      info.wifi_ap_staconnected.mac[3],
      info.wifi_ap_staconnected.mac[4],
      info.wifi_ap_staconnected.mac[5]);
    String msg = "TARGET CONNECT: " + String(macStr);
    Serial.println(msg);
    SerialBT.println(msg);
  }
  else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    char macStr[18];
    sprintf(macStr,"%02X:%02X:%02X:%02X:%02X:%02X",
      info.wifi_ap_stadisconnected.mac[0],
      info.wifi_ap_stadisconnected.mac[1],
      info.wifi_ap_stadisconnected.mac[2],
      info.wifi_ap_stadisconnected.mac[3],
      info.wifi_ap_stadisconnected.mac[4],
      info.wifi_ap_stadisconnected.mac[5]);
    String msg = "TARGET DISCONNECT: " + String(macStr);
    Serial.println(msg);
    SerialBT.println(msg);
  }
}

void setup() {
  Serial.begin(115200);

  /* LittleFS */
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS gagal");
    return;
  }

  /* Bluetooth Serial */
  SerialBT.begin("ESP32 Monitor");
  Serial.println("Bluetooth Serial Ready");

  /* WiFi AP */
  WiFi.mode(WIFI_AP);
  WiFi.onEvent(WiFiEvent);

  WiFi.softAP(ssid.c_str(), NULL, 1); // channel stabil
  delay(500);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Serve file statis dari LittleFS
  server.serveStatic("/", LittleFS, "/");
  server.on("/", handlePortal);
  server.onNotFound(handlePortal);

  server.begin();

  Serial.println("Captive Portal Ready");
  SerialBT.println("Captive Portal Ready - Send 'ssid <new_name>' to change SSID");
  Serial.println("Sound file 'sound.mp3' should be served from LittleFS for auto-play.");
}

void loop() {
  // Baca input dari Bluetooth Serial
  if (SerialBT.available()) {
    String input = SerialBT.readStringUntil('\n');
    input.trim(); // Hapus spasi/tab di awal/akhir

    // Cek apakah input adalah perintah untuk ganti SSID
    if (input.startsWith("ssid ")) {
      String newSSID = input.substring(5); // Ambil bagian setelah "ssid "
      newSSID.trim();
      
      if (newSSID.length() > 0 && newSSID.length() <= 32) { // Validasi panjang SSID
        ssid = newSSID; // Update SSID
        restartAP(); // Restart AP dengan SSID baru
        
        SerialBT.println("SSID changed to: " + ssid);
      } else {
        SerialBT.println("Invalid SSID length. Must be 1-32 characters.");
      }
    }
  }

  dnsServer.processNextRequest();
  server.handleClient();
}