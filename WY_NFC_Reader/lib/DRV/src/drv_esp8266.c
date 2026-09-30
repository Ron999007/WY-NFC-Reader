/**
 * @file    drv_esp8266.c
 * @brief   Object-oriented ESP8266 AT Command Driver Implementation.
 */
#include "drv_esp8266.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ESP8266_DEBUG 1
#if ESP8266_DEBUG
    #define ESP_DBG(...) printf("[ESP8266] " __VA_ARGS__)
#else
    #define ESP_DBG(...)
#endif

/**
 * @brief Internal helper to parse a completed text line received from the module.
 */
static void parse_text_line(esp8266_obj_t *obj, char *line) {
    ESP_DBG("RX: %s\r\n", line); 

    if (strstr(line, "OK") != NULL) {
        obj->flag_ok = true;
    } else if (strstr(line, "ERROR") != NULL || strstr(line, "FAIL") != NULL) {
        obj->flag_error = true;
    } else if (strncmp(line, "SEND OK", 7) == 0) {
        obj->flag_ok = true;
    } else if (strncmp(line, "+CIFSR:STAIP,\"", 14) == 0) {
        char *end_quote = strchr(line + 14, '"');
        if (end_quote != NULL) {
            *end_quote = '\0';
            strncpy(obj->local_ip, line + 14, sizeof(obj->local_ip) - 1);
        }
    } else if (strstr(line, ",CONNECT") != NULL) {
        printf("\r\n[WIFI_APP] >>> TCP Client Connected! [%s] <<<\r\n", line);
    } else if (strstr(line, ",CLOSED") != NULL) {
        printf("\r\n[WIFI_APP] <<< TCP Client Disconnected! [%s] >>>\r\n", line);
    } else if (strncmp(line, "+IPD,", 5) == 0) {
        char *comma_ptr = strchr(line + 5, ',');
        char *colon_ptr = strchr(line + 5, ':');
        if (comma_ptr != NULL && colon_ptr != NULL) {
            *comma_ptr = '\0'; *colon_ptr = '\0';
            obj->ipd_link_id = atoi(line + 5);
            obj->ipd_target_len = atoi(comma_ptr + 1);
            obj->ipd_read_len = 0;
            obj->rx_state = ESP_STATE_READ_IPD_DATA; 
        }
    }
}

/**
 * @brief Internal helper to ingest and process raw bytes from the abstract interface.
 */
static void process_rx_buffer(esp8266_obj_t *obj) {
    uint8_t ch;
    static uint8_t ipd_buffer[512]; 

    /* Read all available bytes from the interface */
    while (obj->io->available(obj->io->user_data) && obj->io->read(obj->io->user_data, &ch)) {
        
        /* Globally check for CIPSEND prompt character */
        if (ch == '>') obj->flag_prompt = true;

        switch (obj->rx_state) {
            case ESP_STATE_IDLE:
            case ESP_STATE_READ_LINE:
                if (ch == '\n') {
                    obj->line_buf[obj->line_idx] = '\0';
                    parse_text_line(obj, obj->line_buf);
                    obj->line_idx = 0; 
                    if (obj->rx_state != ESP_STATE_READ_IPD_DATA) obj->rx_state = ESP_STATE_IDLE;
                } else if (ch != '\r' && ch != '>') {
                    if (obj->line_idx < sizeof(obj->line_buf) - 1) {
                        obj->line_buf[obj->line_idx++] = ch;
                        obj->rx_state = ESP_STATE_READ_LINE;
                        
                        /* Fast-track for +IPD header parsing to prevent missing data payload */
                        if (ch == ':' && strncmp(obj->line_buf, "+IPD,", 5) == 0) {
                            obj->line_buf[obj->line_idx] = '\0';
                            parse_text_line(obj, obj->line_buf); 
                            obj->line_idx = 0; 
                        }
                    }
                }
                break;
                
            case ESP_STATE_READ_IPD_DATA:
                /* In this state, raw payload drops directly into the buffer */
                if (obj->ipd_read_len < sizeof(ipd_buffer)) {
                    ipd_buffer[obj->ipd_read_len] = ch;
                }
                obj->ipd_read_len++;
                
                /* Check if the expected payload length has been reached */
                if (obj->ipd_read_len >= obj->ipd_target_len) {
                    if (obj->on_tcp_data_received != NULL) {
                        uint16_t pass_len = (obj->ipd_target_len > sizeof(ipd_buffer)) ? sizeof(ipd_buffer) : obj->ipd_target_len;
                        obj->on_tcp_data_received(obj->ipd_link_id, ipd_buffer, pass_len);
                    }
                    obj->rx_state = ESP_STATE_IDLE;
                }
                break;
                
            default:
                obj->rx_state = ESP_STATE_IDLE;
                break;
        }
    }
}

/**
 * @brief Initializes the ESP8266 driver object by injecting the IO interface.
 */
void drv_esp8266_init(esp8266_obj_t *obj, uart_io_interface_t *io) {
    if (obj == NULL || io == NULL) return;
    obj->io = io;
    obj->rx_state = ESP_STATE_IDLE;
    obj->line_idx = 0;
    obj->flag_ok = false;
    obj->flag_error = false;
    obj->flag_prompt = false;
    obj->on_tcp_data_received = NULL;
    memset(obj->local_ip, 0, sizeof(obj->local_ip));
    obj->cmd_state = ESP_CMD_IDLE;
    ESP_DBG("Driver Initialized in Decoupled Async Mode.\r\n");
}

/**
 * @brief Sends an AT command asynchronously.
 */
void drv_esp8266_async_send_at(esp8266_obj_t *obj, const char *cmd, uint32_t timeout_ms) {
    obj->flag_ok = false;
    obj->flag_error = false;
    
    if (cmd[0] != '\0') {
        ESP_DBG("TX: %s", cmd);
        obj->io->write(obj->io->user_data, (const uint8_t*)cmd, strlen(cmd));
    }
    
    obj->cmd_start_tick = obj->io->get_tick();
    obj->cmd_timeout_ms = timeout_ms;
    obj->cmd_state = ESP_CMD_WAIT_OK;
}

/**
 * @brief Sends TCP data asynchronously. Automatically handles the ">" prompt.
 */
void drv_esp8266_async_send_tcp(esp8266_obj_t *obj, uint8_t link_id, const uint8_t *data, uint16_t len) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", link_id, len);
    
    obj->flag_prompt = false;
    obj->flag_error = false;
    obj->tcp_payload_ptr = data;
    obj->tcp_payload_len = len;
    
    ESP_DBG("TX: %s", cmd);
    obj->io->write(obj->io->user_data, (const uint8_t*)cmd, strlen(cmd));
    
    obj->cmd_start_tick = obj->io->get_tick();
    obj->cmd_timeout_ms = 2000; 
    obj->cmd_state = ESP_CMD_WAIT_PROMPT;
}

/**
 * @brief Core polling engine. Processes RX data and updates the async state machine.
 */
esp8266_res_e drv_esp8266_async_poll(esp8266_obj_t *obj) {
    if (obj == NULL || obj->io == NULL) return ESP_RES_IDLE;
    
    /* Process incoming data */
    process_rx_buffer(obj);
    
    /* Return early if no command is pending */
    if (obj->cmd_state == ESP_CMD_IDLE) return ESP_RES_IDLE;
    
    uint32_t elapsed = obj->io->get_tick() - obj->cmd_start_tick;
    
    switch (obj->cmd_state) {
        case ESP_CMD_WAIT_OK:
            if (obj->flag_ok) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_OK; }
            if (obj->flag_error) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_ERROR; }
            if (elapsed >= obj->cmd_timeout_ms) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_TIMEOUT; }
            return ESP_RES_PENDING;
            
        case ESP_CMD_WAIT_PROMPT:
            if (obj->flag_prompt) {
                ESP_DBG("Prompt '>' received, sending TCP Payload...\r\n");
                obj->flag_ok = false;
                obj->flag_error = false;
                /* Send the actual payload */
                obj->io->write(obj->io->user_data, obj->tcp_payload_ptr, obj->tcp_payload_len);
                obj->cmd_start_tick = obj->io->get_tick();
                obj->cmd_timeout_ms = 3000;
                obj->cmd_state = ESP_CMD_WAIT_SEND_OK;
                return ESP_RES_PENDING;
            }
            if (obj->flag_error) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_ERROR; }
            if (elapsed >= obj->cmd_timeout_ms) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_TIMEOUT; }
            return ESP_RES_PENDING;
            
        case ESP_CMD_WAIT_SEND_OK:
            if (obj->flag_ok) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_OK; }
            if (obj->flag_error) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_ERROR; }
            if (elapsed >= obj->cmd_timeout_ms) { obj->cmd_state = ESP_CMD_IDLE; return ESP_RES_TIMEOUT; }
            return ESP_RES_PENDING;
            
        default:
            obj->cmd_state = ESP_CMD_IDLE; 
            return ESP_RES_IDLE;
    }
}