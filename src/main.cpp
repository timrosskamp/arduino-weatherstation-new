#include "settings.h"
#include "TFT_Setup.h"
#include "secrets.h"

#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"

#include <FS.h>
#include <LittleFS.h>
#define FlashFS LittleFS

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <TFT_eSPI.h>
#include <IoAbstraction.h>
#include "OpenWeatherMapOneCall.h"




#define COLOR_BLUE   0x03BF // 0, 120, 255
#define COLOR_YELLOW 0xFD40 // 255, 172, 7
#define COLOR_PURPLE 0x6136 // 104, 38, 183

IoAbstractionRef ioDevice = ioUsingArduino();

TFT_eSPI tft = TFT_eSPI();

OpenWeatherMapOneCallData weather;

time_t now;
tm timeinfo;

enum Screens {
    Progress,
    Weather,
    Sun,
    Forecast
};

Screens screen;



String getIconForWeatherId(int id, bool large) {
    String suffix = large ? "100x.bmp" : "50x.bmp";

    // Clear & Clouds
    if( id >= 800 ){
        if( id == 800 ){
            return "/clear-" + suffix; // clear
        }
        else if( id <= 802 ){
            return "/few-clouds-" + suffix; // light clouds
        }
        else if( id <= 804 ){
            return "/clouds-" + suffix; // heavy clouds
        }
    }
    // Atmosphere 
    else if( id >= 700 ){
        return "/mist-" + suffix;
    }
    // Snow
    else if( id >= 600 ){
        return "/snow-" + suffix;
    }
    // Rain
    else if( id >= 500 ){
        // light rain or moderate rain
        if( id <= 502 ){
            return "/shower-rain-" + suffix;
        }
        // freezing rain
        else if( id == 511 ){
            return "/snow-" + suffix;
        }
        // heavy intensity rain, very heavy rain or shower rain
        else{
            return "/rain-" + suffix;
        }
    }
    // Drizzle
    else if( id >= 300 ){
        // light intensity drizzle, drizzle, light intensity drizzle rain or drizzle rain
        if( id == 300 || id == 301 || id == 310 || id == 311 ){
            return "/shower-rain-" + suffix;
        }
        else{
            return "/rain-" + suffix;
        }
    }
    // Thunderstorm
    else if( id >= 200 ){
        return "/thunder-" + suffix;
    }

    return "";
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc) {
    // For speed use fixed point maths and rounding to permit a power of 2 division
    uint16_t fgR = ((fgc >> 10) & 0x3E) + 1;
    uint16_t fgG = ((fgc >>  4) & 0x7E) + 1;
    uint16_t fgB = ((fgc <<  1) & 0x3E) + 1;

    uint16_t bgR = ((bgc >> 10) & 0x3E) + 1;
    uint16_t bgG = ((bgc >>  4) & 0x7E) + 1;
    uint16_t bgB = ((bgc <<  1) & 0x3E) + 1;

    // Shift right 1 to drop rounding bit and shift right 8 to divide by 256
    uint16_t r = (((fgR * alpha) + (bgR * (255 - alpha))) >> 9);
    uint16_t g = (((fgG * alpha) + (bgG * (255 - alpha))) >> 9);
    uint16_t b = (((fgB * alpha) + (bgB * (255 - alpha))) >> 9);

    // Combine RGB565 colours into 16 bits
    //return ((r&0x18) << 11) | ((g&0x30) << 5) | ((b&0x18) << 0); // 2 bit greyscale
    //return ((r&0x1E) << 11) | ((g&0x3C) << 5) | ((b&0x1E) << 0); // 4 bit greyscale
    return (r << 11) | (g << 5) | (b << 0);
}

// Bodmer's streamlined x2 faster "no seek" version
void drawBmp(String filename, uint16_t x, uint16_t y, TFT_eSPI *_tft) {
    if( (x >= _tft->width()) || (y >= _tft->height())) return;

    fs::File bmpFS;

    // Check file exists and open it
    Serial.println(filename);

    if( !LittleFS.exists(filename) ) filename = "/missing.bmp";

    if( !LittleFS.exists(filename) ) return; // all is lost, if not even the missing.bmp exists

    // Open requested file
    bmpFS = LittleFS.open(filename, "r");

    uint32_t seekOffset;
    uint16_t w, h, row, col;
    uint8_t  r, g, b;

    if (read16(bmpFS) == 0x4D42)
    {
        read32(bmpFS);
        read32(bmpFS);
        seekOffset = read32(bmpFS);
        read32(bmpFS);
        w = read32(bmpFS);
        h = read32(bmpFS);

        if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
        {
            y += h - 1;

            _tft->setSwapBytes(true);
            bmpFS.seek(seekOffset);

            // Calculate padding to avoid seek
            uint16_t padding = (4 - ((w * 3) & 3)) & 3;
            uint8_t lineBuffer[w * 3 + padding];

            for (row = 0; row < h; row++) {
                
                bmpFS.read(lineBuffer, sizeof(lineBuffer));
                uint8_t*  bptr = lineBuffer;
                uint16_t* tptr = (uint16_t*)lineBuffer;
                // Convert 24 to 16 bit colours using the same line buffer for results
                for (col = 0; col < w; col++)
                {
                    b = *bptr++;
                    g = *bptr++;
                    r = *bptr++;
                    *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }

                // Push the pixel row to screen, pushImage will crop the line if needed
                // y is decremented as the BMP image is drawn bottom up
                _tft->pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
            }
        }
        else Serial.println("BMP format not recognized.");
    }
    bmpFS.close();
}

String lastProgressText;
uint8_t lastProgressPercentage;

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
    if( screen != Progress ){
        tft.fillScreen(TFT_BLACK);
    }
    
    if( lastProgressText != text ){
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(BC_DATUM);
        tft.loadFont(AA_FONT_SMALL, LittleFS);
        tft.fillRect(0, 144, 240, 16, TFT_BLACK);
        tft.drawString(text, 120, 160);
        tft.unloadFont();
        lastProgressText = text;
    }

    if( lastProgressPercentage > percentage ){
        tft.fillRect(12, 170, 216, 11, TFT_BLACK); // Clear Bar
    }

    tft.drawRoundRect(10, 168, 220, 15, 4, TFT_WHITE);
    tft.fillRoundRect(12, 170, 216 * percentage / 100.0, 11, 2, COLOR_BLUE);

    lastProgressPercentage = percentage;
    
    screen = Progress;
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
        drawProgress(i, "Connecting to WiFi...");
        i += 10;
        Serial.print(".");
    }

    drawProgress(100, "Connected to WiFi.");

    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    delay(1000);
}

void updateWeatherData() {
    OpenWeatherMapOneCall *oneCallClient = new OpenWeatherMapOneCall();
    oneCallClient->setMetric(true);
    oneCallClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
    oneCallClient->update(&weather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
}

void updateData() {
    drawProgress(50, "Updating weather...");

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

    for( int8_t i = 0; i < 4; i++ ){
        for( int8_t j = 0; j < 2 * (i + 1); j++ ){
            if( quality > i * 25 || j == 0 ){
                // gfx.setPixel(228 + 2 * i, 15 - j);
                tft.drawPixel(228 + 2 * i, 15 - j, TFT_WHITE);
            }
        }
    }
}

void drawCurrentWeather() {
    // gfx.setTransparentColor(COLOR_BLACK);
    // gfx.drawPalettedBitmapFromPgm(0, 85, getMeteoconIconFromProgmem(weather.current.weatherIcon));
    drawBmp(getIconForWeatherId(weather.current.weatherId, true), 10, 76, &tft);

    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COLOR_BLUE, TFT_BLACK);
    tft.drawString(DISPLAYED_CITY_NAME, 220, 95);

    tft.loadFont(AA_FONT_LARGE, LittleFS);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(weather.current.temp, 1) + " C", 220, 114);

    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(weather.current.weatherDescription, 220, 148);

    tft.unloadFont();
}

void drawClock() {
    char time_str[11];

    tft.setTextDatum(TC_DATUM);
    tft.loadFont(AA_FONT_LARGE, LittleFS);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    sprintf(time_str, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    tft.fillRect(60, 36, 120, 38, TFT_BLACK);
    tft.drawString(time_str, 120, 37);

    tft.unloadFont();
}

void drawTime() {
    time(&now);
    localtime_r(&now, &timeinfo);

    tft.setTextDatum(TC_DATUM);
    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String date = WDAY_NAMES[timeinfo.tm_wday] + ", " + String(timeinfo.tm_mday) + ". " + MONTH_NAMES[timeinfo.tm_mon] + " " + String(1900 + timeinfo.tm_year);
    tft.drawString(date, 120, 21);

    tft.unloadFont();

    drawClock();
}

void updateTime() {
    time(&now);
    localtime_r(&now, &timeinfo);

    drawClock();
}

void drawForecastColumn(uint16_t x, uint16_t y, uint8_t dayIndex) {
    OpenWeatherMapOneCallDailyData day = weather.daily[dayIndex];

    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    time_t time = day.dt;
    struct tm *timeinfo = localtime(&time);
    tft.drawString(WDAY_NAMES[timeinfo->tm_wday], x + 25, y - 15);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(String(day.tempDay, 0) + " C", x + 25, y + 5);
    tft.setTextColor(COLOR_BLUE, TFT_BLACK);
    tft.drawString(String(day.tempNight, 0) + " C", x + 25, y + 20);

    drawBmp(getIconForWeatherId(day.weatherId, false), x, y + 35, &tft);

    tft.setTextColor(COLOR_BLUE, TFT_BLACK);

    if( day.snow ){
        tft.drawString(String(day.snow, 1) + "mm", x + 25, y + 80);
    }else if( day.rain ){
        tft.drawString(String(day.rain, 1) + "mm", x + 25, y + 80);
    }

    tft.unloadFont();
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
    float tscale = min(3.0F, 40 / tdelta);

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

            float y = interpolate(y0, y1, y2, y3, mu);

            double y_floor;
            double y_decimal = modf((double) y, &y_floor);
            tft.drawPixel(x, y_floor, alphaBlend((1 - y_decimal) * 255, COLOR_BLUE, TFT_BLACK));
            tft.drawLine(x, y_floor + 1, x, 320, COLOR_BLUE);
        }
    }

    tft.loadFont(AA_FONT_SMALL, LittleFS);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    for( int i = 0; i < 6; i++ ){
        // index in the forecast array
        int j = i * 4 + 2;
        float t = weather.hourly[j].temp;
        // minimum y-coord in current, last and next temperature
        int tymin = min(min(tycoords[j], tycoords[j+1]), tycoords[j-1]);
        int x = i * 40 + 20;

        tft.drawString(String(t, 0), x, tymin - 10);
    }

    tft.setTextColor(TFT_WHITE, COLOR_BLUE);

    char time_str[11];

    for( int i = 0; i < 3; i++ ){
        int j = i * 8 + 4;
        time_t time = weather.hourly[j].dt;
        struct tm *timeinfo = localtime(&time);
        int x = i * 80 + 40;

        sprintf(time_str, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        tft.drawString(time_str, x, 310);
    }

    tft.unloadFont();
}

// Sinuskurve mit Nullstellen bei x=0, x=1 und Hochpunkt bei (0.5|1).
float sineHill(float x) {
    return 0.5 * sin(2 * PI * x - HALF_PI) + 0.5;
}

void drawSunForecast() {
    const int G_y = 220;
    const int G_h = 80;
    const int G_horz_isect_x = 240 * 0.2;
    const int G_horz_isect_h = sineHill(((float) G_horz_isect_x) / 240) * G_h;

    time(&now);
    localtime_r(&now, &timeinfo);

    time_t sunrise = weather.current.sunrise;
    tm sunriseinfo;
    localtime_r(&sunrise, &sunriseinfo);

    time_t sunset = weather.current.sunset;
    tm sunsetinfo;
    localtime_r(&sunset, &sunsetinfo);

    char time_str[11];

    tft.loadFont(AA_FONT_SMALL);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.setTextDatum(BL_DATUM);
    sprintf(time_str, "%02d:%02d", sunriseinfo.tm_hour, sunriseinfo.tm_min);
    tft.drawString(time_str, 10, G_y - G_h + 10);

    tft.setTextDatum(BR_DATUM);
    sprintf(time_str, "%02d:%02d", sunsetinfo.tm_hour, sunsetinfo.tm_min);
    tft.drawString(time_str, 230, G_y - G_h + 10);

    float x_sun;

    if( sunrise > now ){
        // sun is yet to rise
        float prog = 1 - (float)(sunrise - now) / (float)(sunriseinfo.tm_hour * 3600 + sunriseinfo.tm_min * 60 + sunriseinfo.tm_sec);
        x_sun = G_horz_isect_x * prog;
    }else if( sunset > now ) {
        // sun has risen, is yet to set
        float prog_day = (float)(now - sunrise) / (float)(sunset - sunrise);
        x_sun = G_horz_isect_x + (240 - G_horz_isect_x * 2) * prog_day;
    }else{
        // sun has set
        float prog = (float)(now - sunset) / (float)((24 - sunsetinfo.tm_hour) * 3600 + (60 - sunsetinfo.tm_min) * 60 + 60 - sunsetinfo.tm_sec);
        x_sun = 240 - G_horz_isect_x + G_horz_isect_x * prog;
    }
    
    // draw sunline
    for( int x = 0; x < 240; x++ ){
        float y = G_y - sineHill(((float) x) / 240) * G_h;
        double y_floor;
        double y_decimal = modf((double) y, &y_floor);
        bool gtHorizon = x > G_horz_isect_x && x < 240 - G_horz_isect_x;

        if( x < x_sun ){
            int color;

            if( gtHorizon ){
                color = COLOR_BLUE;
            }else{
                color = COLOR_PURPLE;
            }

            tft.drawLine(x, G_y - G_horz_isect_h, x, y_floor, color);

            if( gtHorizon ){
                tft.drawPixel(x, y_floor, alphaBlend((1 - y_decimal) * 255, TFT_WHITE, TFT_BLACK));
                tft.drawPixel(x, y_floor + 1, alphaBlend(y_decimal * 255, TFT_WHITE, color));
            }else{
                tft.drawPixel(x, y_floor, alphaBlend((1 - y_decimal) * 255, TFT_WHITE, color));
                tft.drawPixel(x, y_floor + 1, alphaBlend(y_decimal * 255, TFT_WHITE, TFT_BLACK));
            }
        }else{
            tft.drawPixel(x, y_floor, alphaBlend((1 - y_decimal) * 255, TFT_WHITE, TFT_BLACK));
            tft.drawPixel(x, y_floor + 1, alphaBlend(y_decimal * 255, TFT_WHITE, TFT_BLACK));
        }
    }

    // draw horizon
    tft.drawLine(0, G_y - G_horz_isect_h, 240, G_y - G_horz_isect_h, TFT_WHITE);

    // draw sun
    tft.fillCircle(x_sun, G_y - sineHill(((float) x_sun) / 240) * G_h, 8, COLOR_YELLOW);
}

void drawScreen() {
    tft.fillScreen(TFT_BLACK);

    if( screen == Weather ){
        drawWifiQuality();
        drawTime();
        drawCurrentWeather();
        drawHourlyForecastGraph();
    }else if( screen == Sun ) {
        drawWifiQuality();
        drawTime();
        drawSunForecast();
    }else if( screen == Forecast ){
        drawWifiQuality();
        drawForecastColumn(10, 60, 1);
        drawForecastColumn(95, 60, 2);
        drawForecastColumn(180, 60, 3);
        drawForecastColumn(10, 200, 4);
        drawForecastColumn(95, 200, 5);
        drawForecastColumn(180, 200, 6);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    if( !LittleFS.begin() ){
        Serial.println("Flash FS initialisation failed!");
        while(1) yield(); // Stay here twiddling thumbs waiting
    }
    Serial.println("Flash FS available!");

    if( !LittleFS.exists("/NotoSansBold15.vlw") ||
        !LittleFS.exists("/NotoSansBold36.vlw") ){
        Serial.println("Fonts missing in Flash FS, did you upload it?");
        while(1) yield(); // Stay here twiddling thumbs waiting
    }
    else Serial.println("Fonts found OK.");

    pinMode(D1, OUTPUT);
    digitalWrite(D1, HIGH);

    // ioDevicePinMode(ioDevice, D1, OUTPUT);
    switches.initialise(ioDevice, true);

    // Turn on the background LED
    // ioDeviceDigitalWriteS(ioDevice, D1, HIGH);

    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    configTime(LOCAL_TIMEZONE, NTP_SERVERS);

    connectWifi();

    // wait until time is valid
    while( time(&now) < NTP_MIN_VALID_EPOCH ){
        delay(300);
    }

    Serial.println("Current time: " + String(now));

    updateData();
    screen = Sun;
    drawScreen();

    switches.addSwitch(D4, [](uint8_t pin, bool held){
        if( held ){
            ESP.deepSleep(0);
        }else{
            if( screen == Weather ){
                screen = Sun;
                drawScreen();
            }else if( screen == Sun ){
                screen = Forecast;
                drawScreen();
            }else if( screen == Forecast ){
                screen = Weather;
                drawScreen();
            }
        }
    });

    time(&now);
    localtime_r(&now, &timeinfo);

    // Wait until a minute just passed.
    taskManager.scheduleOnce((unsigned int) 62 - timeinfo.tm_sec, []{
        if( screen == Weather || screen == Sun ){
            updateTime();
        }
        taskManager.scheduleFixedRate(60, []{
            if( screen == Weather || screen == Sun ){
                updateTime();
            }
        }, TIME_SECONDS);
    }, TIME_SECONDS);

    taskManager.scheduleFixedRate(UPDATE_INTERVAL_SECS, []{
        auto lastScreen = screen;
		updateData();
        screen = lastScreen;
        drawScreen();
	}, TIME_SECONDS);
}

void loop() {
    taskManager.runLoop();
}