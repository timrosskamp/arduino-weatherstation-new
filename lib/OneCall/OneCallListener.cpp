#include "OneCallListener.h"

OneCallListener::OneCallListener(OneCallData *data) {
    this->data = data;
}

void OneCallListener::endArray(ElementPath path){}
void OneCallListener::endDocument(){}
void OneCallListener::endObject(ElementPath path){}
void OneCallListener::startArray(ElementPath path){}
void OneCallListener::startDocument(){}
void OneCallListener::startObject(ElementPath path){}

void OneCallListener::value(ElementPath path, ElementValue value){
    const char *root = path.get(0)->getKey();

    if( strcmp(root, "current") == 0 ){
        const char *current = path.get(1)->getKey();

        if( strcmp(current, "dt") == 0 )         data->current.dt = value.getInt();
        if( strcmp(current, "sunrise") == 0 )    data->current.sunrise = value.getInt();
        if( strcmp(current, "sunset") == 0 )     data->current.sunset = value.getInt();
        if( strcmp(current, "temp") == 0 )       data->current.temp = value.getFloat();
        if( strcmp(current, "feels_like") == 0 ) data->current.feels_like = value.getFloat();
        if( strcmp(current, "pressure") == 0 )   data->current.pressure = value.getInt();
        if( strcmp(current, "humidity") == 0 )   data->current.humidity = value.getInt();
        if( strcmp(current, "clouds") == 0 )     data->current.clouds = value.getInt();
        if( strcmp(current, "uvi") == 0 )        data->current.uvi = value.getFloat();
        if( strcmp(current, "visibility") == 0 ) data->current.visibility = value.getInt();
        if( strcmp(current, "wind_speed") == 0 ) data->current.wind_speed = value.getFloat();
        if( strcmp(current, "wind_deg") == 0 )   data->current.wind_deg = value.getFloat();

        if( strcmp(current, "weather") == 0 && path.get(2)->getIndex() == 0 ){
            const char *weather = path.get(3)->getKey();

            if( strcmp(weather, "id") == 0 ) data->current.weather.id = value.getInt();
            if( strcmp(weather, "main") == 0 ){
                strncpy(data->current.weather.main, value.getString(), sizeof(data->current.weather.main));
            }
            if( strcmp(weather, "description") == 0 ){
                strncpy(data->current.weather.description, value.getString(), sizeof(data->current.weather.description));
            }
            if( strcmp(weather, "icon") == 0 ){
                strncpy(data->current.weather.icon, value.getString(), sizeof(data->current.weather.icon));
            }
        }
    }

    if( strcmp(root, "hourly") == 0 ){
        int index = path.get(1)->getIndex();

        if( index <= 24 ){
            const char *hourly = path.get(2)->getKey();

            if( strcmp(hourly, "dt") == 0 )         data->hourly[index].dt = value.getInt();
            if( strcmp(hourly, "temp") == 0 )       data->hourly[index].temp = value.getFloat();
            if( strcmp(hourly, "feels_like") == 0 ) data->hourly[index].feels_like = value.getFloat();
            if( strcmp(hourly, "pressure") == 0 )   data->hourly[index].pressure = value.getInt();
            if( strcmp(hourly, "humidity") == 0 )   data->hourly[index].humidity = value.getInt();
            if( strcmp(hourly, "clouds") == 0 )     data->hourly[index].clouds = value.getInt();
            if( strcmp(hourly, "uvi") == 0 )        data->hourly[index].uvi = value.getFloat();
            if( strcmp(hourly, "visibility") == 0 ) data->hourly[index].visibility = value.getInt();
            if( strcmp(hourly, "wind_speed") == 0 ) data->hourly[index].wind_speed = value.getFloat();
            if( strcmp(hourly, "wind_deg") == 0 )   data->hourly[index].wind_deg = value.getFloat();
            if( strcmp(hourly, "pop") == 0 )        data->hourly[index].pop = value.getFloat();
            if( strcmp(hourly, "rain") == 0 )       data->hourly[index].rain = value.getFloat();
            if( strcmp(hourly, "snow") == 0 )       data->hourly[index].snow = value.getFloat();

            if( strcmp(hourly, "weather") == 0 && path.get(3)->getIndex() == 0 ){
                const char *weather = path.get(4)->getKey();

                if( strcmp(weather, "id") == 0 ) data->hourly[index].weather.id = value.getInt();
                if( strcmp(weather, "main") == 0 ){
                    strncpy(data->hourly[index].weather.main, value.getString(), sizeof(data->hourly[index].weather.main));
                }
                if( strcmp(weather, "description") == 0 ){
                    strncpy(data->hourly[index].weather.description, value.getString(), sizeof(data->hourly[index].weather.description));
                }
                if( strcmp(weather, "icon") == 0 ){
                    strncpy(data->hourly[index].weather.icon, value.getString(), sizeof(data->hourly[index].weather.icon));
                }
            }
        }
    }

    if( strcmp(root, "daily") == 0 ){
        int index = path.get(1)->getIndex();

        if( index <= 8 ){
            const char *daily = path.get(2)->getKey();

            if( strcmp(daily, "dt") == 0 )         data->daily[index].dt = value.getInt();
            if( strcmp(daily, "pressure") == 0 )   data->daily[index].pressure = value.getInt();
            if( strcmp(daily, "humidity") == 0 )   data->daily[index].humidity = value.getInt();
            if( strcmp(daily, "clouds") == 0 )     data->daily[index].clouds = value.getInt();
            if( strcmp(daily, "uvi") == 0 )        data->daily[index].uvi = value.getFloat();
            if( strcmp(daily, "wind_speed") == 0 ) data->daily[index].wind_speed = value.getFloat();
            if( strcmp(daily, "wind_deg") == 0 )   data->daily[index].wind_deg = value.getFloat();
            if( strcmp(daily, "pop") == 0 )        data->daily[index].pop = value.getFloat();
            if( strcmp(daily, "rain") == 0 )       data->daily[index].rain = value.getFloat();
            if( strcmp(daily, "snow") == 0 )       data->daily[index].snow = value.getFloat();

            if( strcmp(daily, "temp") == 0 ){
                const char *temp = path.get(3)->getKey();

                if( strcmp(temp, "morn") == 0 )  data->daily[index].temp.morn = value.getFloat();
                if( strcmp(temp, "day") == 0 )   data->daily[index].temp.day = value.getFloat();
                if( strcmp(temp, "eve") == 0 )   data->daily[index].temp.eve = value.getFloat();
                if( strcmp(temp, "night") == 0 ) data->daily[index].temp.night = value.getFloat();
            }

            if( strcmp(daily, "weather") == 0 && path.get(3)->getIndex() == 0 ){
                const char *weather = path.get(4)->getKey();

                if( strcmp(weather, "id") == 0 ) data->daily[index].weather.id = value.getInt();
                if( strcmp(weather, "main") == 0 ){
                    strncpy(data->daily[index].weather.main, value.getString(), sizeof(data->daily[index].weather.main));
                }
                if( strcmp(weather, "description") == 0 ){
                    strncpy(data->daily[index].weather.description, value.getString(), sizeof(data->daily[index].weather.description));
                }
                if( strcmp(weather, "icon") == 0 ){
                    strncpy(data->daily[index].weather.icon, value.getString(), sizeof(data->daily[index].weather.icon));
                }
            }
        }
    }
};

void OneCallListener::whitespace(char c){}