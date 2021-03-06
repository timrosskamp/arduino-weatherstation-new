#pragma once

class OneCall
{
    public:
        OneCall(const char* appId);
        bool update(float lat, float lng);
    private:
        const char* host = "api.openweathermap.org";
        const unsigned short port = 443;
        const char* appId;
};
