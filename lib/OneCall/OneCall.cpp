#include "OneCall.h"
#include <WiFiClientSecure.h>

OneCall::OneCall(const char* appId) {
    this->appId = appId;
}

bool OneCall::update(float lat, float lng) {
    WiFiClientSecure client;
    unsigned long started = millis();
    char path[160];
    char request[260];
    char c;

    sprintf(path, "/data/2.5/onecall?lat=%f&lon=%f&exclude=minutely,alerts&units=metric&lang=de&appid=%s", lat, lng, appId);
    sprintf(request, "GET %s HTTP/1.1\r\nHOST: %s\r\nConnection: close\r\n\r\n", path, host);

    client.setInsecure();

    if( !client.connect(host, port) ){
        Serial.println("[HTTP] failed to connect to host");
        return false;
    }

    Serial.println("[HTTP] connected, now getting data");
    client.print(request);

    Serial.println("[HTTP] Headers:");

    while( client.connected() ){
        String line = client.readStringUntil('\n');
        if( line == "\r" ){
            // End of headers
            break;
        }

        Serial.println(line);

        if( (millis() - started) > 10000 ){
            Serial.println("[HTTP] client timeout during headers");
            client.stop();
            return false;
        }
    }

    Serial.println();

    while( client.connected() || client.available() > 0 ) {
        while( client.available() > 0 ){
            c = client.read();

            Serial.print(c);
        }

        if( (millis() - started) > 10000 ){
            Serial.println("[HTTP] client timeout during JSON parse");
            client.stop();
            return false;
        }

        // give WiFi and TCP/IP libraries a chance to handle pending events
        yield();
    }

    client.stop();

    return true;
}