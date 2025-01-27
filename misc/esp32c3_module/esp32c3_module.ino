/*
 Channel-hopping ESP32 Sniffer on Arduino IDE 
*/

#include <WiFi.h>
extern "C" {
#include "esp_wifi.h"
}

static const uint8_t CHANNEL_MIN = 1;
static const uint8_t CHANNEL_MAX = 13;
static uint8_t currentChannel = CHANNEL_MIN;

// Called on every received packet
void promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_pkt_rx_ctrl_t rx_ctrl = pkt->rx_ctrl;
  
  Serial.print("Type: "); Serial.print(type);
  Serial.print(" | RSSI: "); Serial.print(rx_ctrl.rssi);
  Serial.print(" | Channel: "); Serial.print(rx_ctrl.channel);
  Serial.print(" | Length: "); Serial.println(rx_ctrl.sig_len);
}

// Hops to the next channel
void hopChannel() {
  currentChannel++;
  if (currentChannel > CHANNEL_MAX) {
    currentChannel = CHANNEL_MIN;
  }
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_MODE_NULL); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(promiscuous_cb);

  // Set initial channel
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
}

void loop() {
  // Hop channels every second
  static unsigned long lastHop = 0;
  if (millis() - lastHop > 1000) {
    lastHop = millis();
    hopChannel();
  }
}
