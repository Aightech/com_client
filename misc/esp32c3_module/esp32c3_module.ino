// ESP32-C3 Firmware (Arduino Framework)
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>

#define DEBUG 1
#define DPRINT(...) if (DEBUG) Serial.print(__VA_ARGS__)
#define DPRINTLN(...) if (DEBUG) Serial.println(__VA_ARGS__)

#define BUTTON_PIN 0

Preferences preferences;//NVS storage for Wi-Fi credentials
WiFiUDP udp;//UDP server for provisioning
const int udpPort = 4210;
char incomingPacket[255];

void setup() {
    Serial.begin(115200);
    preferences.begin("wifi_config", false);

    // Check if Wi-Fi credentials exist in NVS storage
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");

    if (ssid.isEmpty() || pass.isEmpty() || digitalRead(0) == LOW) {  // Reset on button press
        DPRINTLN("No credentials found or reset triggered, listening for UDP packets...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("ESP32C3_Provision", "provision123");//Create an AP for provisioning

        // Wait for provisioning data
        udp.begin(udpPort);
        while (true) {
            int packetSize = udp.parsePacket();
            if (packetSize) {//if packet received
                int len = udp.read(incomingPacket, 255);
                incomingPacket[len] = 0;
                DPRINT("Received data");

                //expecting SSID,PASS format
                String data = String(incomingPacket);
                int commaIndex = data.indexOf(',');
                if (commaIndex > 0) {
                    String newSSID = data.substring(0, commaIndex);
                    String newPASS = data.substring(commaIndex + 1);

                    preferences.putString("ssid", newSSID);
                    preferences.putString("pass", newPASS);
                    preferences.end();

                    // Serial.println("Credentials saved, sending MAC address...");
                    DPRINTLN("Credentials saved, sending MAC address...");
                    uint8_t mac[6];
                    WiFi.macAddress(mac);
                    char macStr[18];
                    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

                    udp.beginPacket(udp.remoteIP(), udp.remotePort());
                    udp.print(macStr);
                    udp.endPacket();
                    delay(2000);
                    esp_restart();
                }
            }
            delay(100);
        }
    } else {
        // Serial.println("Connecting to saved Wi-Fi...");
        DPRINTLN("Connecting to saved Wi-Fi: " + ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
    }
}

void loop() {}