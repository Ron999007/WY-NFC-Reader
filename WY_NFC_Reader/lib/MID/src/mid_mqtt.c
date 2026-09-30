/**
 * @file    mid_mqtt.c
 * @brief   Implementation of the Hardware-independent MQTT 3.1.1 Middleware
 */
#include "mid_mqtt.h"
#include <string.h>

/* ====================================================================
 * Packet Builders Implementation
 * ==================================================================== */

uint16_t mid_mqtt_build_connect(uint8_t *buf, const char *client_id) {
    uint16_t id_len = strlen(client_id);
    uint8_t rem_len = 10 + 2 + id_len; 
    uint16_t idx = 0;
    
    buf[idx++] = 0x10;             
    buf[idx++] = rem_len;          
    
    buf[idx++] = 0x00; buf[idx++] = 0x04;          
    buf[idx++] = 'M';  buf[idx++] = 'Q';           
    buf[idx++] = 'T';  buf[idx++] = 'T';
    buf[idx++] = 0x04;                             
    buf[idx++] = 0x02;                             
    buf[idx++] = 0x00; buf[idx++] = 0x3C;          
    
    buf[idx++] = (id_len >> 8) & 0xFF;             
    buf[idx++] = id_len & 0xFF;                    
    memcpy(&buf[idx], client_id, id_len);          
    idx += id_len;
    
    return idx; 
}

uint16_t mid_mqtt_build_publish(uint8_t *buf, const char *topic, const char *payload) {
    uint16_t topic_len = strlen(topic);
    uint16_t payload_len = strlen(payload);
    uint32_t rem_len = 2 + topic_len + payload_len;
    uint16_t idx = 0;
    
    buf[idx++] = 0x30;
    
    do {
        uint8_t encoded_byte = rem_len % 128;
        rem_len = rem_len / 128;
        if (rem_len > 0) {
            encoded_byte |= 128; 
        }
        buf[idx++] = encoded_byte;
    } while (rem_len > 0);
    
    buf[idx++] = (topic_len >> 8) & 0xFF; 
    buf[idx++] = topic_len & 0xFF;        
    memcpy(&buf[idx], topic, topic_len);
    idx += topic_len;
    
    memcpy(&buf[idx], payload, payload_len);
    idx += payload_len;
    
    return idx;
}

uint16_t mid_mqtt_build_subscribe(uint8_t *buf, const char *topic, uint16_t packet_id) {
    uint16_t topic_len = strlen(topic);
    uint32_t rem_len = 2 + 2 + topic_len + 1;
    uint16_t idx = 0;
    
    buf[idx++] = 0x82;
    
    do {
        uint8_t encoded_byte = rem_len % 128;
        rem_len = rem_len / 128;
        if (rem_len > 0) {
            encoded_byte |= 128;
        }
        buf[idx++] = encoded_byte;
    } while (rem_len > 0);
    
    buf[idx++] = (packet_id >> 8) & 0xFF;
    buf[idx++] = packet_id & 0xFF;
    
    buf[idx++] = (topic_len >> 8) & 0xFF;
    buf[idx++] = topic_len & 0xFF;
    memcpy(&buf[idx], topic, topic_len);
    idx += topic_len;
    buf[idx++] = 0x00; 
    
    return idx;
}

uint16_t mid_mqtt_build_pingreq(uint8_t *buf) {
    buf[0] = 0xC0; 
    buf[1] = 0x00; 
    return 2;
}

/* ====================================================================
 * Packet Parsers Implementation
 * ==================================================================== */

uint8_t mid_mqtt_get_packet_type(const uint8_t *data) {
    return (data[0] & 0xF0);
}

bool mid_mqtt_check_connack(const uint8_t *data, uint16_t len) {
    if (len >= 4 && data[3] == 0x00) {
        return true;
    }
    return false;
}

void mid_mqtt_parse_publish(const uint8_t *data, uint16_t len, mqtt_msg_t *msg_out) {
    msg_out->is_valid = false;
    memset(msg_out->topic, 0, sizeof(msg_out->topic));
    memset(msg_out->payload, 0, sizeof(msg_out->payload));

    uint32_t multiplier = 1;
    uint32_t rem_len = 0;
    uint8_t i = 1;
    uint8_t encoded_byte;
    
    do {
        encoded_byte = data[i++];
        rem_len += (encoded_byte & 127) * multiplier;
        multiplier *= 128;
    } while ((encoded_byte & 128) != 0);

    uint16_t topic_len = (data[i] << 8) | data[i + 1];
    i += 2;
    
    uint16_t t_len = (topic_len < sizeof(msg_out->topic) - 1) ? topic_len : sizeof(msg_out->topic) - 1;
    memcpy(msg_out->topic, &data[i], t_len);
    i += topic_len;

    uint16_t payload_len = rem_len - topic_len - 2;
    uint16_t p_len = (payload_len < sizeof(msg_out->payload) - 1) ? payload_len : sizeof(msg_out->payload) - 1;
    memcpy(msg_out->payload, &data[i], p_len);

    msg_out->is_valid = true;
}