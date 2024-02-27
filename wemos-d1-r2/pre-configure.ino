// ========== LIBRARiES ==========
#include <EEPROM.h>

// ========== SETTINGS ==========
struct DataSettings {
    // device
    char device_name[31];
       
    // wifi 
    char wifi_ssid[33];
    char wifi_psk[65];

    // api
    char api_url[151];
    char api_authorization[101];
    unsigned int api_data_sending_interval_seconds;
        
    // webserver
    unsigned int webserver_port;

    // softap
    char softap_ssid[33];
    char softap_psk[65];
};

// ========== INITIAL SETTINGS ==========
void setup() {
    // serial
    Serial.begin(115200);
    delay(3000);
    Serial.println("Entering standard data...");

    // enter standard data
    DataSettings data;

    strlcpy(data.device_name, "device-name", sizeof(data.device_name) - 1);

    strlcpy(data.wifi_ssid, "wifi-ssid", sizeof(data.wifi_ssid) - 1);
    strlcpy(data.wifi_psk, "12345678", sizeof(data.wifi_psk) - 1);

    strlcpy(data.api_url, "api-url", sizeof(data.api_url) - 1);
    strlcpy(data.api_authorization, "api-authorization", sizeof(data.api_authorization) - 1);
    data.api_data_sending_interval_seconds = 1;

    data.webserver_port = 80;

    strlcpy(data.softap_ssid, "softap-ssid", sizeof(data.softap_ssid) - 1);
    strlcpy(data.softap_psk, "12345678", sizeof(data.softap_psk) - 1);

    // save
    EEPROM.begin(sizeof(DataSettings));

    EEPROM.put(0, data);

    EEPROM.commit();
}

// ========== RUNNING PROGRAM ==========
void loop() {
    Serial.println("Saved...");
}