/**
 * @file    drv_esp8266.h
 * @brief   Object-oriented ESP8266 AT Command Driver (100% Non-blocking Async).
 * @details Fully decoupled from MCU hardware using dependency injection (uart_io_interface_t).
 */
#ifndef __DRV_ESP8266_H__
#define __DRV_ESP8266_H__

#include <stdint.h>
#include <stdbool.h>
#include "uart_io.h"

/**
 * @brief Internal states for RX line parsing.
 */
typedef enum {
    ESP_STATE_IDLE = 0,
    ESP_STATE_READ_LINE,
    ESP_STATE_READ_IPD_LEN,
    ESP_STATE_READ_IPD_DATA
} esp8266_rx_state_e;

/**
 * @brief Internal states for Asynchronous Command tracking.
 */
typedef enum {
    ESP_CMD_IDLE = 0,
    ESP_CMD_WAIT_OK,
    ESP_CMD_WAIT_PROMPT,
    ESP_CMD_WAIT_SEND_OK
} esp8266_cmd_state_e;

/**
 * @brief Execution results for the polling engine.
 */
typedef enum {
    ESP_RES_IDLE = 0,    /* No operation in progress */
    ESP_RES_PENDING,     /* Waiting for device response */
    ESP_RES_OK,          /* Operation finished successfully */
    ESP_RES_ERROR,       /* Device returned ERROR or FAIL */
    ESP_RES_TIMEOUT      /* Device failed to respond within the given time */
} esp8266_res_e;

/**
 * @brief ESP8266 Object Structure representing a single module instance.
 */
typedef struct {
    /* Hardware Abstraction Interface */
    uart_io_interface_t *io;
    
    /* RX Parsing Variables */
    esp8266_rx_state_e rx_state;
    char line_buf[128];
    uint16_t line_idx;
    uint8_t  ipd_link_id;     
    uint16_t ipd_target_len;  
    uint16_t ipd_read_len;    
    char local_ip[16];        
    
    /* Response Flags updated by the parser */
    volatile bool flag_ok;
    volatile bool flag_error;
    volatile bool flag_prompt; 
    
    /* Async Command Tracking */
    esp8266_cmd_state_e cmd_state;
    uint32_t cmd_start_tick;
    uint32_t cmd_timeout_ms;
    const uint8_t *tcp_payload_ptr; 
    uint16_t tcp_payload_len;
    
    /**
     * @brief Application Callback for incoming TCP data.
     * @param link_id The connection ID of the TCP client.
     * @param data    Pointer to the received binary payload.
     * @param len     Length of the received binary payload.
     */
    void (*on_tcp_data_received)(uint8_t link_id, uint8_t *data, uint16_t len);
    
} esp8266_obj_t;

/**
 * @brief Initializes the ESP8266 driver object by injecting the IO interface.
 * @param obj Pointer to the ESP8266 driver object instance.
 * @param io  Pointer to the implemented UART IO interface contract.
 */
void drv_esp8266_init(esp8266_obj_t *obj, uart_io_interface_t *io);

/**
 * @brief Sends an AT command asynchronously. Returns immediately.
 * @param obj        Pointer to the ESP8266 driver object instance.
 * @param cmd        Null-terminated string containing the AT command.
 * @param timeout_ms Maximum time allowed for the command to finish.
 */
void drv_esp8266_async_send_at(esp8266_obj_t *obj, const char *cmd, uint32_t timeout_ms);

/**
 * @brief Sends TCP data asynchronously. Automatically handles the ">" prompt.
 * @param obj     Pointer to the ESP8266 driver object instance.
 * @param link_id TCP Connection ID (0-4).
 * @param data    Pointer to the payload (MUST be static or globally accessible).
 * @param len     Payload length in bytes.
 */
void drv_esp8266_async_send_tcp(esp8266_obj_t *obj, uint8_t link_id, const uint8_t *data, uint16_t len);

/**
 * @brief Core polling engine. Processes RX data and updates the async state machine.
 * @param obj Pointer to the ESP8266 driver object instance.
 * @return Status of the currently executing async command.
 */
esp8266_res_e drv_esp8266_async_poll(esp8266_obj_t *obj);

#endif /* __DRV_ESP8266_H__ */