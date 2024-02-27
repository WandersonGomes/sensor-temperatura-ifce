// ========== LIBRARIES ==========
#include <DHTesp.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <limits.h>

// ========== PIN CONFIGURATION ==========
#define PIN_BUTTON      D0
#define PIN_DHT11       D5
#define PIN_LED_BLUE    D1
#define PIN_LED_GREEN   D4
#define PIN_LED_RED     D2

// ========== MY TOOLS ==========
#define print(...) Serial.print(__VA_ARGS__)
#define printf(...) Serial.printf(__VA_ARGS__)
#define println(...) Serial.println(__VA_ARGS__)

#define ERROR_CODE_DHT      3
#define ERROR_CODE_SOFTAP   4
#define ERROR_CODE_WIFI     2
#define ERROR_CODE_API      1

void printSerialINFO(String message) {
    println("[INFO]: " + message);
}

void printSerialERROR(String message) {
    println("[ERROR]: " + message);
}

void turnOnRedLed() {
    digitalWrite(PIN_LED_RED, HIGH);
}

void turnOffRedLed() {
    digitalWrite(PIN_LED_RED, LOW);
}

void blinkRedLed(int code) {
    for (byte i = 0; i < code; i++) {
        delay(500);
        turnOnRedLed();
        delay(500);
        turnOffRedLed();
    }
}

void turnOnBlueLed() {
    digitalWrite(PIN_LED_BLUE, HIGH);
}

void turnOffBlueLed() {
    digitalWrite(PIN_LED_BLUE, LOW);
}

// ========== SETTINGS==========
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

class DeviceSettingsManager {
    private:
        DataSettings data;

    public:
        void loadDataEEPROM() {
            EEPROM.begin(sizeof(DataSettings));
            EEPROM.get(0, data);
            EEPROM.end();
        }

        void saveDataEEPROM() {
            EEPROM.begin(sizeof(DataSettings));
            EEPROM.put(0, data);
            EEPROM.commit();
            EEPROM.end();
        }

        // device
        void setDeviceName(String name) {
            if (name.length() > 0) {
                strlcpy(data.device_name, name.c_str(), sizeof(data.device_name) - 1);
            }
        }

        const char* getDeviceName() {
            return data.device_name;
        }

        // wifi
        void setWiFiSSID(String ssid) {
            if (ssid.length() > 0) {
                strlcpy(data.wifi_ssid, ssid.c_str(), sizeof(data.wifi_ssid) - 1);
            }
        }

        const char* getWiFiSSID() {
            return data.wifi_ssid;
        }

        void setWiFiPSK(String psk) {
            if (psk.length() > 0) {
                strlcpy(data.wifi_psk, psk.c_str(), sizeof(data.wifi_psk) - 1);
            }
        }

        const char* getWiFiPSK() {
            return data.wifi_psk;
        }

        // api
        void setAPIURL(String url) {
            if (url.length() > 0) {
                strlcpy(data.api_url, url.c_str(), sizeof(data.api_url) - 1);
            }
        }

        const char* getAPIURL() {
            return data.api_url;
        }

        void setAPIAuthorization(String authorization) {
            if (authorization.length() > 0) {
                strlcpy(data.api_authorization, authorization.c_str(), sizeof(data.api_authorization) - 1);
            }
        }

        const char* getAPIAuthorization() {
            return data.api_authorization;
        }

        void setAPIDataSendingIntervalSeconds(unsigned int seconds) {
            if ((seconds > 0) && (seconds <= 86400)) {
                data.api_data_sending_interval_seconds = seconds;
            }
        }

        unsigned int getAPIDataSendingIntervalSeconds() {
            return data.api_data_sending_interval_seconds;
        }

        unsigned int getAPIDataSendingIntervalMillis() {
            return data.api_data_sending_interval_seconds * 1000;
        }

        // webserver
        void setWebServerPort(unsigned int port) {
            if ((port > 0) && (port <= 65535)) {
                data.webserver_port = port;
            }
        }

        unsigned int getWebServerPort() {
            return data.webserver_port;
        }

        // softap
        void setSoftAPSSID(String ssid) {
            if (ssid.length() > 0) {
                strlcpy(data.softap_ssid, ssid.c_str(), sizeof(data.softap_ssid) - 1);
            }
        }

        const char* getSoftAPSSID() {
            return data.softap_ssid;
        }

        void setSoftAPPSK(String psk) {
            if (psk.length() > 0) {
                strlcpy(data.softap_psk, psk.c_str(), sizeof(data.softap_psk) - 1);
            }
        }

        const char* getSoftAPPSK() {
            return data.softap_psk;
        }
};

// ========== DHT 11 ==========
class DHT11Manager {
    private:
        float temperature;
        float humidity;
        unsigned int previousReadingTimeMillis;
        short readingIntervalMillis;
        DHTesp dht;

    public:
        void begin(byte pin_dht) {
            previousReadingTimeMillis = millis();
            readingIntervalMillis = 2000;
            dht.setup(pin_dht, DHTesp::DHT11);
        }

        void acquireData() {
            if (dht.getStatus() == dht.ERROR_NONE) {
                unsigned int currentTimeMillis = millis();
                unsigned int difference = currentTimeMillis - previousReadingTimeMillis;
                
                // check and handle possible overflows
                if ((currentTimeMillis < previousReadingTimeMillis)) {
                    difference = UINT_MAX - previousReadingTimeMillis + currentTimeMillis + 1;
                }

                if (difference >= readingIntervalMillis) {
                    temperature = dht.getTemperature();
                    humidity = dht.getHumidity();                    
                }

                previousReadingTimeMillis = currentTimeMillis;

                printSerialINFO("Temperature: " + String(temperature) + " °C");
                printSerialINFO("Humidity: " + String(humidity) + " %");
            } else {
              blinkRedLed(ERROR_CODE_DHT);
            }
        }

        float getTemperature() {
            return temperature;
        }

        float getHumidity() {
            return humidity;
        }

        void run() {
            acquireData();
        }
};

// ========== WEBSERVER ==========
const char HTML[] PROGMEM = R"=====(<!DOCTYPE html><html lang="pt-br"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Device Details</title><style>[CSS]</style></head><body><header><h1 class="logo">Device Details</h1></header><main><section id="section-data"><h2>Real-Time Data</h2><div class="box-gauge"><div class="gauge-chart"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 160" id="gauge-temperature"><path class="background" d="M41 149.5a77 77 0 1 1 117.93 0"/><path id="track-temperature" class="track" stroke="#249863" d="M41 149.5a77 77 0 1 1 117.93 0"/><path class="striped-background" d="M41 149.5a77 77 0 1 1 117.93 0"/><text x="55" y="80"><tspan>Temperature</tspan></text><text text-anchor="middle" class="text-value" x="100" y="120"><tspan id="value-temperature"></tspan></text></svg></div><div class="gauge-chart"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 160" id="gauge-humidity"><path class="background" d="M41 149.5a77 77 0 1 1 117.93 0"/><path id="track-humidity" class="track" stroke="#249863" d="M41 149.5a77 77 0 1 1 117.93 0"/><path class="striped-background" d="M41 149.5a77 77 0 1 1 117.93 0"/><text x="70" y="80"><tspan>Humidity</tspan></text><text text-anchor="middle" class="text-value" x="100" y="120"><tspan id="value-humidity"></tspan></text></svg></div></div></section><section id="section-device-settings"><h2>Device Settings</h2><form action="./save-settings" method="post" enctype="application/x-www-form-urlencoded"><fieldset><legend>Device</legend><label for="device-name">Name:</label><input name="device-name" value="[DEVICE-NAME]" type="text" autocomplete="off" required></fieldset><fieldset><legend>WiFi</legend><label for="wifi-ssid">SSID:</label><input name="wifi-ssid" value="[WIFI-SSID]" type="text" autocomplete="off" required><label for="wifi-psk">PSK:</label><input name="wifi-psk" value="[WIFI-PSK]" type="password" autocomplete="off" required><label for="wifi-ip">IP:</label><input name="wifi-ip" value="[WIFI-IP]" type="text" disabled="disabled"></fieldset><fieldset><legend>API</legend><label for="api-url">URL:</label><input name="api-url" value="[API-URL]" type="text" autocomplete="off" required><label for="api-authorization">Authorization:</label><input name="api-authorization" value="[API-AUTHORIZATION]" type="text" autocomplete="off" required><label for="api-data-sending-interval-seconds">Data Sending Interval(s):</label><input name="api-data-sending-interval-seconds" value="[API-DATA-SENDING-INTERVAL-SECONDS]" type="number" min="1" max="86400" required></fieldset><fieldset><legend>WebServer</legend><label for="webserver-port">Port:</label><input name="webserver-port" value="[WEBSERVER-PORT]" type="number" required min="1" max="65535"></fieldset><fieldset><legend>Soft AP</legend><label for="softap-ssid">SSID:</label><input name="softap-ssid" value="[SOFTAP-SSID]" type="text" required autocomplete="off"><label for="softap-psk">PSK:</label><input name="softap-psk" value="[SOFTAP-PSK]" type="password" required autocomplete="off"><label for="softap-local-ip">IP:</label><input name="softap-local-ip" value="[SOFTAP-LOCAL-IP]" type="text" disabled="disabled"></fieldset><input type="submit" value="Save"></form></section><section id="section-about"><h2>About</h2><br>Produced by<strong>Wanderson Gomes da Costa</strong>.<br>For more details click<a href="http://www.github.com/WandersonGomes/temperature-humidity-project-ifce">here</a>.</section></main><script>[JAVASCRIPT]</script></body></html>)=====";
const char CSS[] PROGMEM = R"=====(header,main{box-shadow:1px 1px 5px var(--color-shadow)}.logo,legend{font-weight:700}header,input[type=submit]{background-color:var(--color-primary)}.logo,input[type=submit]{font-size:20px;color:#fff}:root{--color-background:#FFFFFF;--color-primary:#C4161C;--color-secondary:#FFCB08;--color-border-light:#DADCE0;--color-shadow:#80808080}*{margin:0;padding:0;box-sizing:border-box;font-family:Roboto,sans-serif}body{background-color:var(--color-background)}header{display:flex;justify-content:center;align-items:center;padding:15px 5%}main{max-width:1024px;margin:10px auto;padding:20px}h1{font-size:2rem}h2{color:#696868}.box-gauge{display:flex;justify-content:space-between}.gauge-chart{margin:auto;height:200px;width:200px}.gauge-chart svg path{will-change:auto;stroke-width:20px;stroke-miterlimit:round;transition:stroke-dashoffset 850ms ease-in-out;fill:none}.gauge-chart .background{stroke:#E7E7E7}.gauge-chart .striped-background{stroke-dasharray:2 30;stroke:white}.gauge-chart .track{stroke-dasharray:350;stroke-dashoffset:350}.gauge-chart .text-value{font-size:20pt;font-weight:900}fieldset{margin-top:20px;border:1px solid var(--color-shadow);border-radius:2px;padding:10px}input[type=number],input[type=password],input[type=text],select{width:100%;padding:12px 20px;margin:8px 0;display:inline-block;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}input[type=submit]{width:100%;font-weight:500;padding:14px 0;margin:20px 0;border:none;border-radius:4px;cursor:pointer}input[type=submit]:hover{background-color:var(--color-secondary)}#section-about{text-align:center;font-size:.5rem}section{border:1px solid var(--color-border-light);padding:10px;margin:10px})=====";
const char JAVASCRIPT[] PROGMEM = R"=====(function updateTemperatureAndHumidityDisplay(e,t){const n=document.getElementById("track-temperature"),r=document.getElementById("track-humidity");var a=n.getTotalLength()*((50-e)/50),o=r.getTotalLength()*((100-t)/100);n.getBoundingClientRect(),r.getBoundingClientRect(),n.style.strokeDashoffset=Math.max(0,a),r.style.strokeDashoffset=Math.max(0,o),n.style.stroke=e<=17?"#045DC2":e<=34?"#01AA0F":"#C20404",r.style.stroke="#0584EC",document.getElementById("value-temperature").innerHTML=e+" °C",document.getElementById("value-humidity").innerHTML=t+" %"}function main(){var e=window.location.href+"data";fetch(e, {method:'GET', mode: 'no-cors'}).then(e=>{if(!e.ok)throw new Error("Error fetching data");return e.json()}).then(e=>{console.log("Received data: ",e),updateTemperatureAndHumidityDisplay(e.temperature,e.humidity)}).catch(e=>{console.error("Error fetching data: ",e)})}setInterval(main,1e3);)=====";
const char PAGE_404[] PROGMEM = R"=====(<!DOCTYPE html><html lang="pt-br"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Erro 404 - Página não encontrada</title><style>body,html{height:100%;margin:0;padding:0;display:flex;justify-content:center;align-items:center;background-color:#f8f9fa}.error-container{font-family:Arial,sans-serif;text-align:center}.error-number{font-size:10em;color:#dc3545}.error-message{margin-top:20px;font-size:2em;color:#343a40}a{color:#dc3545;font-size:1rem}</style></head><body><div class="error-container"><div class="error-number">404</div><div class="error-message">Página não encontrada.<br><a href="/">Home</a></div></div></body></html>)=====";

class WebServerManager {
    private:
        ESP8266WebServer server;
        DeviceSettingsManager* deviceSettingsManager;
        DHT11Manager* dht11Manager;

        String prepareBuildHTMLPage() {
            String page = (const __FlashStringHelper*) HTML;
            
            page.replace("[CSS]", (const __FlashStringHelper*) CSS);
            page.replace("[JAVASCRIPT]", (const __FlashStringHelper*) JAVASCRIPT);

            return page;
        }

        void populatePageWithDataSetting(String& page) {
            page.replace("[DEVICE-NAME]", deviceSettingsManager->getDeviceName());
            
            page.replace("[WEBSERVER-PORT]", String(deviceSettingsManager->getWebServerPort()));

            page.replace("[WIFI-SSID]", deviceSettingsManager->getWiFiSSID());
            page.replace("[WIFI-PSK]", deviceSettingsManager->getWiFiPSK());
            page.replace("[WIFI-IP]", WiFi.localIP().toString());

            page.replace("[API-URL]", deviceSettingsManager->getAPIURL());
            page.replace("[API-AUTHORIZATION]", deviceSettingsManager->getAPIAuthorization());
            page.replace("[API-DATA-SENDING-INTERVAL-SECONDS]", String(deviceSettingsManager->getAPIDataSendingIntervalSeconds()));

            page.replace("[SOFTAP-SSID]", deviceSettingsManager->getSoftAPSSID());
            page.replace("[SOFTAP-PSK]", deviceSettingsManager->getSoftAPPSK());
            page.replace("[SOFTAP-LOCAL-IP]", WiFi.softAPIP().toString());
        }

        void handleDeviceSettingsPage() {
            String page = prepareBuildHTMLPage();
            populatePageWithDataSetting(page);
            server.send(200, "text/html", page);
        }

        void handleDataDHTJSON() {
            String json = "{\"temperature\": [TEMPERATURE], \"humidity\": [HUMIDITY]}";            

            json.replace("[TEMPERATURE]", String(dht11Manager->getTemperature()));
            json.replace("[HUMIDITY]", String(dht11Manager->getHumidity()));

            server.send(200, "application/json", json);
        }

        void handlePage404() {
            String page = (const __FlashStringHelper*) PAGE_404;

            server.send(200, "text/html", page);
        }

        void handleSaveDeviceSettings() {
            byte numberArgs = server.args();

            for (byte i = 0; i < numberArgs; i++) {
                String arg = server.argName(i);
                String value = server.arg(i);

                if (arg.equals("device-name")) {
                    deviceSettingsManager->setDeviceName(value);                    
                } else if (arg.equals("wifi-ssid")) {
                    deviceSettingsManager->setWiFiSSID(value);
                } else if (arg.equals("wifi-psk")) {
                    deviceSettingsManager->setWiFiPSK(value);
                } else if (arg.equals("api-url")) {
                    deviceSettingsManager->setAPIURL(value);
                } else if (arg.equals("api-authorization")) {
                    deviceSettingsManager->setAPIAuthorization(value);
                } else if (arg.equals("api-data-sending-interval-seconds")) {
                    deviceSettingsManager->setAPIDataSendingIntervalSeconds(value.toInt());
                } else if (arg.equals("webserver-port")) {
                    deviceSettingsManager->setWebServerPort(value.toInt());
                } else if (arg.equals("softap-ssid")) {
                    deviceSettingsManager->setSoftAPSSID(value);
                } else if (arg.equals("softap-psk")) {
                    deviceSettingsManager->setSoftAPPSK(value);
                }
            }

            deviceSettingsManager->saveDataEEPROM();
            delay(2000);
            ESP.reset();
        } 

    public:
        void begin(DeviceSettingsManager* _deviceSettingsManager, DHT11Manager* _dht11Manager) {
            deviceSettingsManager = _deviceSettingsManager;
            dht11Manager = _dht11Manager;

            server.begin(deviceSettingsManager->getWebServerPort());

            server.on("/", HTTP_GET, std::bind(&WebServerManager::handleDeviceSettingsPage, this));
            server.on("/data", HTTP_GET, std::bind(&WebServerManager::handleDataDHTJSON, this));
            server.on("/save-settings", HTTP_POST, std::bind(&WebServerManager::handleSaveDeviceSettings, this));
            server.onNotFound(std::bind(&WebServerManager::handlePage404, this));
        }

        void run() {
            server.handleClient();
        }
};

// ========== SOFTAP ==========
class SoftAPManager {
    private:
        DeviceSettingsManager deviceSettingsManager;

    public:
        void begin(DeviceSettingsManager deviceSettingsManager) {
            this->deviceSettingsManager = deviceSettingsManager;
        }

        void start() {
            if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
                if (WiFi.softAP(deviceSettingsManager.getSoftAPSSID(), deviceSettingsManager.getSoftAPPSK())) {
                    printSerialINFO("SoftAP running...");
                    printSerialINFO("[" + WiFi.softAPIP().toString() + "]");
                    turnOnBlueLed();
                } else {
                    printSerialERROR("Problem start SoftAP...");
                    blinkRedLed(ERROR_CODE_SOFTAP);
                }
            }
        }

        void stop() {
            turnOffBlueLed();
            if (WiFi.softAPIP() != IPAddress(0, 0, 0, 0)) {
                if (WiFi.softAPdisconnect(true)) {
                    printSerialINFO("SoftAP stop running!");
                } else {
                    printSerialERROR("SoftAP not stop!");
                    blinkRedLed(ERROR_CODE_SOFTAP);
                }
            }
        }

        void run() {
            if (digitalRead(PIN_BUTTON)) {
                start();
            } else {
                stop();
            }
        }
};

// ========== WIFI ==========
class WiFiManager {
    private:

    public:
        void start(DeviceSettingsManager deviceSettingsManager) {
            WiFi.begin(deviceSettingsManager.getWiFiSSID(), deviceSettingsManager.getWiFiPSK());
        }

        void run() {
            if (WiFi.isConnected()) {
                printSerialINFO("IP WiFi: " + WiFi.localIP().toString());
            } else {
                printSerialERROR("WiFi not connected!");
                blinkRedLed(ERROR_CODE_WIFI);
            }
        }
};

// ========== API ==========
class APIManager {
    private:
        DeviceSettingsManager* deviceSettingsManager;
        DHT11Manager* dht11Manager;
        unsigned int previousSendingTimeMillis;

        String preparedJSON() {
            String json = "{\"device\": \"[DEVICE-NAME]\", \"temperature\": [TEMPERATURE], \"humidity\": [HUMIDITY]}";

            json.replace("[DEVICE-NAME]", deviceSettingsManager->getDeviceName());
            json.replace("[TEMPERATURE]", String(dht11Manager->getTemperature()));
            json.replace("[HUMIDITY]", String(dht11Manager->getHumidity()));

            return json;
        }

    public:
        void begin(DeviceSettingsManager* _deviceSettingsManager, DHT11Manager* _dht11Manager) {
            deviceSettingsManager = _deviceSettingsManager;
            dht11Manager = _dht11Manager;
            previousSendingTimeMillis = millis();
        }

        void sendingData() {
            if (WiFi.isConnected()) {
                unsigned int currentTimeMillis = millis();
                unsigned int difference = currentTimeMillis - previousSendingTimeMillis;

                // check and handle possible overflows
                if ((currentTimeMillis < previousSendingTimeMillis)) {
                    difference = UINT_MAX - previousSendingTimeMillis + currentTimeMillis + 1;
                }

                if (difference >= deviceSettingsManager->getAPIDataSendingIntervalMillis()) {
                    printSerialINFO("Sending data to API...");

                    WiFiClient clientWiFi;
                    HTTPClient http;

                    String json = preparedJSON();

                    printSerialINFO("API URL: " + String(deviceSettingsManager->getAPIURL()));
                    printSerialINFO("JSON: " + json);

                    http.begin(clientWiFi, String(deviceSettingsManager->getAPIURL()));
                    http.addHeader("Authorization", String(deviceSettingsManager->getAPIAuthorization()));
                    http.addHeader("Content-Type", "application/json");

                    int httpCode = http.POST(json);

                    if (httpCode > 0) {
                        printSerialINFO("HTTP Post Code: " + String(httpCode));
                        
                        if (httpCode == HTTP_CODE_OK) {
                            const String& payload = http.getString();

                            printSerialINFO("Payload: ");
                            println(payload);
                        } else {
                            printSerialERROR("API sending data failed!");
                            blinkRedLed(ERROR_CODE_API);
                        }
                    } else {
                        printSerialERROR("Post failed...");
                        printSerialERROR("Code: " + http.errorToString(httpCode));
                        blinkRedLed(ERROR_CODE_API);
                    }
                    
                    previousSendingTimeMillis = currentTimeMillis;

                    http.end();
                }
            } else {
                printSerialERROR("Connection API failed...");
                blinkRedLed(ERROR_CODE_API);
            }
        }

        void run() {
            sendingData();
        }
};

// ========== INITIAL SETTING ==========
DeviceSettingsManager deviceSettingsManager;
WebServerManager webserverManager;
SoftAPManager softAPManager;
WiFiManager wifiManager;
APIManager apiManager;
DHT11Manager dht11Manager;

void setup() {
    // serial
    Serial.begin(115200);
    Serial.setDebugOutput(false);

    // pin led red
    pinMode(PIN_LED_RED, OUTPUT);

    // signal device connected
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_LED_GREEN, HIGH);

    // dht11
    dht11Manager.begin(PIN_DHT11);

    // load device settings
    deviceSettingsManager.loadDataEEPROM();
    
    // wifi
    wifiManager.start(deviceSettingsManager);

    // webserver
    webserverManager.begin(&deviceSettingsManager, &dht11Manager);

    // softap
    softAPManager.begin(deviceSettingsManager);
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);

    //api
    apiManager.begin(&deviceSettingsManager, &dht11Manager);

    println("\n");
}

// ========== PROGRAM RUNNING ==========
void loop() {
    // webserver
    webserverManager.run();

    // softap
    softAPManager.run();

    // wifi
    wifiManager.run();

    // dht11
    dht11Manager.run();

    // api
    apiManager.run();
}