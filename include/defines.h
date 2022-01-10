#ifndef DEFINES_H
#define DEFINES_H

#define MQTT_CUSTOMER "viktak"
#define MQTT_PROJECT  "wio-terminal"

#define HARDWARE_ID "Wio Terminal"
#define HARDWARE_VERSION "1.0"
#define SOFTWARE_ID "Hello World"

#define SerialMon Serial

#define DEBUG_SPEED 921600

#define JSON_SETTINGS_SIZE (JSON_OBJECT_SIZE(10) + 200)
#define JSON_MQTT_COMMAND_SIZE 512

#define CONNECTION_STATUS_LED_GPIO 0
#define MODE_BUTTON_GPIO 2

#define DEFAULT_HEARTBEAT_INTERVAL 300

#endif