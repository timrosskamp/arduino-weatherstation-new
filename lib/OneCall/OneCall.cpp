#include "OneCall.h"
#include "OneCallData.h"
#include <OneCallListener.h>
#include <WiFiClientSecure.h>
#include <JsonStreamingParser2.h>

OneCall::OneCall(char *appId) {
    this->appId = appId;
}

bool OneCall::update(OneCallData *data, float lat, float lng) {
    WiFiClientSecure client;
    JsonStreamingParser parser;
    auto listener = new OneCallListener(data);
    unsigned long started = millis();
    char path[160];
    char request[260];
    char c;

    sprintf(path, "/data/2.5/onecall?lat=%f&lon=%f&exclude=minutely,alerts&units=metric&lang=de&appid=%s", lat, lng, appId);
    sprintf(request, "GET %s HTTP/1.1\r\nHOST: %s\r\nConnection: close\r\n\r\n", path, host);

    parser.setHandler(listener);

    client.setInsecure();

    if( !client.connect(host, port) ){
        Serial.println("[HTTP] failed to connect to host");
        return false;
    }

    Serial.println("[HTTP] connected, now getting data");
    client.print(request);

    while( client.connected() ){
        String line = client.readStringUntil('\n');
        if( line == "\r" ){
            // End of headers
            break;
        }

        if( (millis() - started) > 10000 ){
            Serial.println("[HTTP] client timeout during headers");
            client.stop();
            return false;
        }
    }

    while( client.connected() || client.available() > 0 ) {
        while( client.available() > 0 ){
            c = client.read();

            parser.parse(c);
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

    Serial.println("[HTTP] done");

    return true;
}