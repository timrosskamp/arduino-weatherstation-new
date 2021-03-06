#pragma once
#include <Arduino.h>

typedef struct OneCallDataWeather {
    // Weather condition id
    uint16_t id;

    // Group of weather parameters (Rain, Snow, Extreme etc.)
    char main[20];

    // Weather condition within the group
    char description[30];

    // Weather icon id
    char icon[10];
} OneCallDataWeather;

typedef struct OneCallDataTemperature {
    // Morning temperature.
    float morn;
    
    // Day temperature.
    float day;
    
    // Evening temperature.
    float eve;
    
    // Night temperature.
    float night;
} OneCallDataTemperature;

typedef struct OneCallDataHourly {
    // Time of the forecasted data, Unix, UTC
    uint32_t dt;

    // Temperature
    float temp;

    // Temperature. This temperature parameter accounts for the human perception
    // of weather.
    float feels_like;

    // Atmospheric pressure on the sea level, hPa
    uint16_t pressure;

    // Humidity, %
    uint8_t humidity;

    // Current UV index
    float uvi;

    // Cloudiness, %
    uint8_t clouds;

    // Average visibility, metres
    uint16_t visibility;

    // Wind speed.
    float wind_speed;

    // Wind direction, degrees (meteorological)
    float wind_deg;

    // Probability of precipitation
    float pop;

    // Rain volume for last hour, mm (where available)
    float rain;

    // Snow volume for last hour, mm (where available)
    float snow;

    OneCallDataWeather weather;
} OneCallDataHourly;

typedef struct OneCallDataDaily {
    // Time of the forecasted data, Unix, UTC
    uint32_t dt;

    // Sunrise time, Unix, UTC
    uint32_t sunrise;

    // Sunset time, Unix, UTC
    uint32_t sunset;

    OneCallDataTemperature temp;

    /* OneCallDataTemperature feels_like; */

    // Atmospheric pressure on the sea level, hPa
    uint16_t pressure;

    // Humidity, %
    uint8_t humidity;

    // Wind speed.
    float wind_speed;

    // Wind direction, degrees (meteorological)
    float wind_deg;

    // Cloudiness, %
    uint8_t clouds;

    // Current UV index
    float uvi;

    // Probability of precipitation
    float pop;

    // Rain volume for last hour, mm (where available)
    float rain;

    // Snow volume for last hour, mm (where available)
    float snow;

    OneCallDataWeather weather;
} OneCallDataDaily;

typedef struct OneCallDataCurrent {
    // Current time, Unix, UTC
    uint32_t dt;

    // Sunrise time, Unix, UTC
    uint32_t sunrise;

    // Sunset time, Unix, UTC
    uint32_t sunset;

    // Temperature
    float temp;

    // Temperature. This temperature parameter accounts for the human perception
    // of weather.
    float feels_like;

    // Atmospheric pressure on the sea level, hPa
    uint16_t pressure;

    // Humidity, %
    uint8_t humidity;

    // Cloudiness, %
    uint8_t clouds;

    // Current UV index
    float uvi;

    // Average visibility, metres
    uint16_t visibility;

    // Wind speed.
    float wind_speed;

    // Wind direction, degrees (meteorological)
    float wind_deg;

    OneCallDataWeather weather;
} OneCallDataCurrent;

typedef struct OneCallData {
    OneCallDataCurrent current;

    OneCallDataHourly hourly[25];

    OneCallDataDaily daily[8];
} OneCallData;