Items:
- WiFi
- MQTT
- I2C

# WiFi
Connect to AP and remain connected.

Important events:
- WIFI_EVENT_STA_START
- WIFI_EVENT_STA_DISCONNECTED
- IP_EVENT_STA_GOT_IP

If WIFI_EVENT_STA_START, connect to WiFi (esp_wifi_connect()).

If WIFI_EVENT_STA_DISCONNECTED, try to reconnect for a preset number of times. If it fails, send WIFI_FAIL_BIT to WiFi event group.

If IP_EVENT_STA_GOT_IP, reset retry counter and notify WiFi event group with WIFI_CONNECTED_BIT.


# MQTT




