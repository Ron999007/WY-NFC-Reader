/**
 * @file    task_wifi.h
 * @brief   Wi-Fi TCP Server Application Task with Async FSM.
 * @details Application layer that initializes hardware interfaces, binds them 
 * to the ESP8266 driver, and runs the MQTT state machine.
 */
#ifndef __TASK_WIFI_H__
#define __TASK_WIFI_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enumeration of Wi-Fi and MQTT State Machine phases.
 */
typedef enum {
    WIFI_STATE_INIT = 0,
    WIFI_STATE_CHECK_ALIVE,
    WIFI_STATE_DISABLE_ECHO,
    WIFI_STATE_SET_MODE,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_GET_IP,
    WIFI_STATE_TCP_CONNECT,
    WIFI_STATE_TCP_DELAY,
    WIFI_STATE_MQTT_CONNECT,
    WIFI_STATE_MQTT_SUBSCRIBE, 
    WIFI_STATE_MQTT_CONNECTED,
    WIFI_STATE_ERROR
} wifi_fsm_state_e;

/**
 * @brief Initializes the Wi-Fi task, RingBuffers, hardware, and binds the driver object.
 */
void Task_wifi_Init(void);

/**
 * @brief Main non-blocking routine for the Wi-Fi task. Must be called repeatedly in the main loop.
 */
void Task_wifi(void);

/**
 * @brief API for other application tasks to publish data to the MQTT Broker.
 * @param topic   The target MQTT Topic string.
 * @param payload The data string to send.
 */
void Task_Wifi_Publish_MQTT(const char *topic, const char *payload);

/**
 * @brief Retrieves the current state of the Wi-Fi State Machine.
 * @return Current state enumeration value.
 */
wifi_fsm_state_e Task_wifi_Get_State(void);

/**
 * @brief Forces a hardware reset of the Wi-Fi module and attempts to reconnect to a new AP.
 * @param ssid The SSID of the new Access Point.
 * @param pwd  The password of the new Access Point.
 */
void Task_Wifi_Reconnect(const char *ssid, const char *pwd);

#endif /* __TASK_WIFI_H__ */