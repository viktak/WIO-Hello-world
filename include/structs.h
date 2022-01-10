struct config{
    char ssid[22];
    char password[32];

    char friendlyName[30];
    u_int32_t heartbeatInterval;

    u_int32_t timeZone;

    char mqttServer[64];
    u_int16_t mqttPort;
    char mqttTopic[32];

    bool dst;

    u_int8_t mode;
};


