#include "settings.h"
#include "secrets.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MiniGrafx.h>
#include <ILI9341_SPI.h>
#include <simpleDSTadjust.h>
#include "OpenWeatherMapOneCall.h"

#include "ArialRounded.h"
#include "weathericons.h"




#define COLOR_BLACK  0
#define COLOR_WHITE  1
#define COLOR_YELLOW 2
#define COLOR_BLUE   3

// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {
    0x0000,  // BLACK
    0xFFFF,  // WHITE
    0xED84,  // #f0b429 - YELLOW
    0x5D7C   // #62B0E8 - BLUE
};

int SCREEN_WIDTH = 240;
int SCREEN_HEIGHT = 320;

// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 = 4 colors

time_t dstOffset = 0;


ADC_MODE(ADC_VCC);




ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);

OpenWeatherMapOneCallData weather;
simpleDSTadjust dstAdjusted(StartRule, EndRule);




int screen = 0;
int screenCount = 2;

long lastDownloadUpdate = millis();

unsigned long lastPressed;
int wait = 200;




// Progress bar helper
void writeProgress(uint8_t percentage, String text) {
    gfx.fillBuffer(COLOR_BLACK);
    gfx.setFont(ArialRoundedMTBold_14);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.setColor(COLOR_YELLOW);
    gfx.drawString(120, 146, text);
    gfx.setColor(COLOR_WHITE);
    gfx.drawRect(10, 168, 220, 15);
    gfx.setColor(COLOR_BLUE);
    gfx.fillRect(12, 170, 216 * percentage / 100, 11);

    gfx.commit();
}

void connectWifi() {
    if( WiFi.status() == WL_CONNECTED ) return;
    
    Serial.print("Connecting");

    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.hostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int i = 0;

    while( WiFi.status() != WL_CONNECTED ){
        delay(500);
        if( i > 80 ) i = 0;
        writeProgress(i, "Connecting to WiFi...");
        i += 10;
        Serial.print(".");
    }

    writeProgress(100, "Connected to WiFi.");

    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    delay(1000);
}

void updateTimeData() {
    time_t now;

    configTime(UTC_OFFSET * 3600, 0, NTP_SERVERS);

    while( (now = time(nullptr)) < NTP_MIN_VALID_EPOCH ){
        delay(300);
    }
    
    Serial.printf("Current time: %d\n", now);

    // calculate for time calculation how much the dst class adds.
    dstOffset = UTC_OFFSET * 3600 + dstAdjusted.time(nullptr) - now;
    Serial.printf("Time difference for DST: %d\n", dstOffset);
}

void updateWeatherData() {
    OpenWeatherMapOneCall *oneCallClient = new OpenWeatherMapOneCall();
    oneCallClient->setMetric(IS_METRIC);
    oneCallClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
    oneCallClient->update(&weather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
}

void updateData() {
    writeProgress(10, "Updating time...");

    updateTimeData();

    writeProgress(50, "Updating weather...");

    updateWeatherData();
}

int8_t getWifiQuality() {
    int32_t dbm = WiFi.RSSI();
    if( dbm <= -100 ){
        return 0;
    }else if( dbm >= -50 ){
        return 100;
    }else{
        return 2 * (dbm + 100);
    }
}

void drawWifiQuality() {
    int8_t quality = getWifiQuality();

    gfx.setColor(COLOR_WHITE);
    // gfx.setFont(ArialMT_Plain_10);
    // gfx.setTextAlignment(TEXT_ALIGN_RIGHT);  
    // gfx.drawString(226, 6, String(quality) + "%");

    for( int8_t i = 0; i < 4; i++ ){
        for( int8_t j = 0; j < 2 * (i + 1); j++ ){
            if( quality > i * 25 || j == 0 ){
                gfx.setPixel(228 + 2 * i, 15 - j);
            }
        }
    }
}

void drawCurrentWeather() {
    gfx.setTransparentColor(COLOR_BLACK);
    gfx.drawPalettedBitmapFromPgm(0, 85, getMeteoconIconFromProgmem(weather.current.weatherIcon));
    
    gfx.setFont(ArialRoundedMTBold_14);
    gfx.setColor(COLOR_BLUE);
    gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
    gfx.drawString(220, 95, DISPLAYED_CITY_NAME);

    gfx.setFont(ArialRoundedMTBold_36);
    gfx.setColor(COLOR_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
    gfx.drawString(220, 108, String(weather.current.temp, 1) + (IS_METRIC ? "°C" : "°F"));

    gfx.setFont(ArialRoundedMTBold_14);
    gfx.setColor(COLOR_YELLOW);
    gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
    gfx.drawStringMaxWidth(220, 148, 120, weather.current.weatherDescription);
}

// draws the clock
void drawTime() {
  char time_str[11];
  char *dstAbbrev;
  time_t now = dstAdjusted.time(&dstAbbrev);
  struct tm *timeinfo = localtime(&now);

  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setColor(COLOR_WHITE);
  String date = WDAY_NAMES[timeinfo->tm_wday] + " " + MONTH_NAMES[timeinfo->tm_mon] + " " + String(timeinfo->tm_mday) + " " + String(1900 + timeinfo->tm_year);
  gfx.drawString(120, 21, date);

  gfx.setFont(ArialRoundedMTBold_36);
  sprintf(time_str, "%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min);
  gfx.drawString(120, 35, time_str);
}

void drawForecastColumn(uint16_t x, uint16_t y, uint8_t dayIndex) {
    OpenWeatherMapOneCallDailyData day = weather.daily[dayIndex];

    gfx.setColor(COLOR_YELLOW);
    gfx.setFont(ArialRoundedMTBold_14);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    time_t time = day.dt;
    struct tm *timeinfo = localtime(&time);
    gfx.drawString(x + 25, y - 15, WDAY_NAMES[timeinfo->tm_wday]);

    gfx.setColor(COLOR_WHITE);
    gfx.drawString(x + 25, y + 5, String(day.tempDay, 0) + (IS_METRIC ? "°C" : "°F"));
    gfx.setColor(COLOR_BLUE);
    gfx.drawString(x + 25, y + 20, String(day.tempNight, 0) + (IS_METRIC ? "°C" : "°F"));

    gfx.drawPalettedBitmapFromPgm(x, y + 35, getMiniMeteoconIconFromProgmem(day.weatherIcon));
    gfx.setColor(COLOR_BLUE);

    if( day.snow ){
        gfx.drawString(x + 25, y + 80, String(day.snow, 1) + (IS_METRIC ? "mm" : "in"));
    }else if( day.rain ){
        gfx.drawString(x + 25, y + 80, String(day.rain, 1) + (IS_METRIC ? "mm" : "in"));
    }
}

/**
 * Smooth interpolation between y1 and y2.
 * 
 * http://paulbourke.net/miscellaneous/interpolation/
 */
float interpolate(float y0, float y1, float y2, float y3, float mu) {
   float mu2 = mu * mu;
   float a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
   float a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
   float a2 = -0.5 * y0 + 0.5 * y2;
   float a3 = y1;

   return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

void drawHourlyForecastGraph() {
    gfx.setColor(COLOR_BLUE);

    float tmin = weather.hourly[0].temp;
    float tmax = weather.hourly[0].temp;

    for( int b = 1; b < 25; b++ ){
        float t = weather.hourly[b].temp;

        if( tmin > t ){
            tmin = t;
        }

        if( tmax < t ){
            tmax = t;
        }
    }

    // temperature delta
    float tdelta = fabs(tmax - tmin);
    // stretch factor for the graph. 1° = 3px, but 40px is max height.
    float tscale = min(3, 40 / tdelta);

    // y-coord for every temperature each hour.
    float tycoords[25] = {};

    for( int b = 0; b < 25; b++ ){
        float t = weather.hourly[b].temp;
        tycoords[b] = 240 - (t - tmin) * tscale;
    }

    for( int b = 0; b < 24; b++ ){
        for( int l = 0; l < 10; l++ ){
            int x = b * 10 + l;
            float mu = (float) l / 9;
            float y0 = tycoords[max(0, b - 1)];
            float y1 = tycoords[b];
            float y2 = tycoords[b + 1];
            float y3 = tycoords[min(23, b + 2)];

            int y = interpolate(y0, y1, y2, y3, mu);

            gfx.drawVerticalLine(x, y, 320 - y);
        }
    }

    gfx.setColor(COLOR_WHITE);
    gfx.setFont(ArialRoundedMTBold_14);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);

    for( int i = 0; i < 6; i++ ){
        // index in the forecast array
        int j = i * 4 + 2;
        float t = weather.hourly[j].temp;
        // minimum y-coord in current, last and next temperature
        int tymin = min(min(tycoords[j], tycoords[j+1]), tycoords[j-1]);
        int x = i * 40 + 20;

        gfx.drawString(x, tymin - 20, String(t, 0));
    }

    gfx.setTransparentColor(COLOR_BLUE);
    gfx.setColor(COLOR_BLACK);

    char time_str[11];

    for( int i = 0; i < 3; i++ ){
        int j = i * 8 + 4;
        time_t time = weather.hourly[j].dt;
        struct tm *timeinfo = localtime(&time);
        int x = i * 80 + 40;

        sprintf(time_str, "%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min);
        gfx.drawString(x, 300, time_str);
    }

    gfx.setTransparentColor(COLOR_BLACK);
}

void buttonPressed() {
    screen = (screen + 1) % screenCount;
}

void ICACHE_RAM_ATTR handleInterrupt() {
    unsigned long now = millis();
    
    if( lastPressed + wait < now ){
        buttonPressed();
        lastPressed = now;
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(D4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(D4), handleInterrupt, FALLING);

    // Turn on the background LED
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH); // HIGH to Turn on;

    gfx.init();
    gfx.fillBuffer(COLOR_BLACK);
    gfx.commit();

    connectWifi();

    updateData();
}

void loop() {
    gfx.fillBuffer(COLOR_BLACK);

    if( screen == 0 ){
        drawWifiQuality();
        drawTime();
        drawCurrentWeather();
        drawHourlyForecastGraph();
    }else if( screen == 1 ){
        drawWifiQuality();
        drawForecastColumn(10, 60, 1);
        drawForecastColumn(95, 60, 2);
        drawForecastColumn(180, 60, 3);
        drawForecastColumn(10, 200, 4);
        drawForecastColumn(95, 200, 5);
        drawForecastColumn(180, 200, 6);
    }

    gfx.commit();

    // Check if we should update weather information
    if( millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS ){
        updateData();
        lastDownloadUpdate = millis();
    }
}