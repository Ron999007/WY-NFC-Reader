/**
 * @file    mid_mqtt.h
 * @brief   Hardware-independent MQTT 3.1.1 Protocol Builder & Parser Middleware
 */
#ifndef __MID_MQTT_H__
#define __MID_MQTT_H__

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * Macros & Definitions
 * ==================================================================== */
#define MQTT_PKT_CONNACK    0x20
#define MQTT_PKT_PUBLISH    0x30
#define MQTT_PKT_SUBACK     0x90
#define MQTT_PKT_PINGRESP   0xD0

/* ====================================================================
 * Data Structures
 * ==================================================================== */
/**
 * @brief Structure to hold a parsed MQTT PUBLISH message.
 */
typedef struct {
    bool is_valid;       /* True if parsing was successful */
    char topic[64];      /* Extracted Topic string */
    char payload[128];   /* Extracted Payload string */
} mqtt_msg_t;

/* ====================================================================
 * Public API Prototypes
 * ==================================================================== */

/* Packet Builders */
uint16_t mid_mqtt_build_connect(uint8_t *buf, const char *client_id);
uint16_t mid_mqtt_build_publish(uint8_t *buf, const char *topic, const char *payload);
uint16_t mid_mqtt_build_subscribe(uint8_t *buf, const char *topic, uint16_t packet_id);
uint16_t mid_mqtt_build_pingreq(uint8_t *buf);

/* Packet Parsers */
uint8_t  mid_mqtt_get_packet_type(const uint8_t *data);
bool     mid_mqtt_check_connack(const uint8_t *data, uint16_t len);
void     mid_mqtt_parse_publish(const uint8_t *data, uint16_t len, mqtt_msg_t *msg_out);

#endif /* __MID_MQTT_H__ */