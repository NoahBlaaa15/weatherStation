#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "SinricPro.h"
#include "SinricProTemperaturesensor.h"
#include "DHT.h"
#include "privateConfig.h"


//Minutes between each online update
#define DELAYMS 20
//Comment to not set the esp in to deep-sleep
#define prod

//Also connect a 5K or 10K between this Pin and 3.3V
#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

unsigned long startupMillis;
unsigned long lastMillis;
float oldH;
float oldT;

void startDeepSleep(int minutes){
    ESP.deepSleep(minutes * 6e7);
    yield();
}

void sendTemp(){
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
    }

    if(h != oldH || t != oldT) {
        SinricProTemperaturesensor &mySensor = SinricPro[temperaturDeviceId];
        if(mySensor.sendTemperatureEvent(t, h)){
            Serial.println("Success");
        }
        oldH = h;
        oldT = t;
    }
}

bool startState(const String &deviceId, bool &state) {
    Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state?"on":"off");
    return true;
}

void startWiFi() {
    Serial.printf("\r\n[Wifi]: Connecting");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf(".");
        delay(250);
    }
    IPAddress localIP = WiFi.localIP();
    Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void startSinricPro(){
    SinricProTemperaturesensor &temperaturesensor = SinricPro["624f3044753dc5aab4a44afe"];
    temperaturesensor.onPowerState(startState);

    // setup SinricPro
    SinricPro.onConnected([](){ sendTemp(); });
    SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
    SinricPro.begin(APP_KEY, APP_SECRET);
}


void setup() {
    Serial.begin(9600);

    Serial.println("Startup of Noah's Weather Station");

    startWiFi();
    startSinricPro();

    dht.begin();

    startupMillis = millis();
}

void loop() {
    unsigned long actualMillis = millis();
    SinricPro.handle();
    if(actualMillis - lastMillis > 60000) {
        Serial.println("[" + String(actualMillis) + "] Sending Data!");
        sendTemp();
        lastMillis = actualMillis;
    }

    if(actualMillis - startupMillis > 120100) {
        Serial.println("[" + String(actualMillis) + "] Going to sleep!");
#ifndef prod
        delay(DELAYMS * 1000);
#endif
#ifdef prod
        startDeepSleep(DELAYMS);
#endif
    }
    delay(10);
}