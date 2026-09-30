/**
 * @file    task_wifi.c
 * @brief   Wi-Fi TCP Server Application Task Implementation (Async FSM).
 */
#include "MyApplication.h" 
#include "mid_mqtt.h"        
#include "drv_esp8266.h"     
#include "periph_uart.h"     
#include "uart_io.h"         
#include <string.h>
#include <stdio.h>

/* ====================================================================
 * Debug Configuration
 * ==================================================================== */
#define WIFI_DEBUG 1

#if WIFI_DEBUG
    #define WIFI_LOG(...) printf(__VA_ARGS__)
#else
    #define WIFI_LOG(...)
#endif

/* Dynamic Wi-Fi Credentials */
static char current_wifi_ssid[32] = "WHAYU-AP";
static char current_wifi_pwd[64]  = "4725047000";

#define WIFI_RX_BUF_SIZE 512
#define WIFI_TX_BUF_SIZE 256

/* Hardware Memory Pools & Ring Buffers */
static uint8_t wifi_rx_mem[WIFI_RX_BUF_SIZE];
static uint8_t wifi_tx_mem[WIFI_TX_BUF_SIZE];
static Utils_RB_t wifi_rx_rb;
static Utils_RB_t wifi_tx_rb;

/* Driver Objects */
static periph_uart_obj_t uart_wifi_obj; /* M487 UART hardware instance */
static esp8266_obj_t esp01s_module;     /* ESP8266 driver instance */

/* FSM Control Variables */
static wifi_fsm_state_e current_wifi_state = WIFI_STATE_INIT;
static bool is_req_sent = false;  
static uint32_t state_timer = 0;
static uint8_t retry_count = 0;
#define MAX_RETRY_COUNT 3

/* ====================================================================
 * Debug Utility: Raw Packet Hex Dumper
 * ==================================================================== */
static void debug_print_mqtt_raw(const char *direction, const uint8_t *data, uint16_t len) {
#if WIFI_DEBUG
    WIFI_LOG("\r\n[%s_RAW] (%d bytes): ", direction, len);
    for (uint16_t i = 0; i < len; i++) {
        WIFI_LOG("%02X ", data[i]);
    }
    WIFI_LOG("\r\n");
#endif
}

static void send_mqtt_tcp_with_dump(uint8_t *data, uint16_t len) {
    debug_print_mqtt_raw("TX", data, len);
    drv_esp8266_async_send_tcp(&esp01s_module, 0, data, len);
}

/* ====================================================================
 * Application Logic: MQTT Packet Parser 
 * ==================================================================== */
static void app_on_tcp_data_received(uint8_t link_id, uint8_t *data, uint16_t len) {
    if (len == 0) return;
    
    debug_print_mqtt_raw("RX", data, len);

    /* Extract the MQTT Packet Type using Middleware */
    uint8_t pkt_type = mid_mqtt_get_packet_type(data);

    switch (pkt_type) {
        case MQTT_PKT_CONNACK:
            if (mid_mqtt_check_connack(data, len)) {
                WIFI_LOG("\r\n=========================================\r\n");
                WIFI_LOG(" [MQTT BROKER ACCEPTED CONNECTION!] \r\n");
                WIFI_LOG(" -> Connection Return Code: 0x00 (Success)\r\n");
                WIFI_LOG("=========================================\r\n\r\n");
                
                /* Trigger the FSM to subscribe to topics */
                current_wifi_state = WIFI_STATE_MQTT_SUBSCRIBE; 
            } else {
                WIFI_LOG("\r\n[MQTT_APP] Broker refused connection.\r\n");
            }
            break;
            
        case MQTT_PKT_PUBLISH:
        {
            mqtt_msg_t incoming_msg;
            mid_mqtt_parse_publish(data, len, &incoming_msg);
            
            if (incoming_msg.is_valid) {
                WIFI_LOG("\r\n=========================================\r\n");
                WIFI_LOG(" [CLOUD COMMAND RECEIVED] \r\n");
                WIFI_LOG(" -> Topic  : %s\r\n", incoming_msg.topic);
                WIFI_LOG(" -> Payload: %s\r\n", incoming_msg.payload);
                WIFI_LOG("=========================================\r\n> ");
                
                /* Note: Add application-specific routing logic here */
            }
            break;
        }

        case MQTT_PKT_SUBACK:
            WIFI_LOG("\r\n[MQTT_APP] Subscription Confirmed! Listening for commands...\r\n> ");
            break;

        case MQTT_PKT_PINGRESP:
            /* Heartbeat acknowledged, ignore to keep console clean */
            break;
    }
}

/* ====================================================================
 * Adapter Layer: Convert M487 hardware interfaces to standard contract
 * ==================================================================== */

/** @brief Adapter: Get available bytes */
static uint32_t io_uart_available(void *data) { 
    return periph_uart_available((periph_uart_obj_t*)data); 
}

/** @brief Adapter: Read one byte */
static bool io_uart_read(void *data, uint8_t *byte) { 
    return periph_uart_read((periph_uart_obj_t*)data, byte); 
}

/** @brief Adapter: Write byte array */
static void io_uart_write(void *data, const uint8_t *buf, uint32_t len) { 
    periph_uart_write((periph_uart_obj_t*)data, buf, len); 
}

/** @brief Adapter: Get system tick */
static uint32_t io_get_tick(void) { 
    return Get_TickCount(); 
}

/** @brief Adapter: Control hardware reset pin */
static void io_hw_reset(void *data, bool state) {
    (void)data; /* Prevent unused variable warning */
    if (!state) {
        PB9 = 0; 
        Delay_ms(20); 
        PB9 = 1; 
        Delay_ms(500);
    }
}

/* Instantiate the IO contract representing the physical link */
static uart_io_interface_t wifi_io_contract = {
    .user_data = &uart_wifi_obj, /* Bind M487 UART2 as the hardware context */
    .available = io_uart_available,
    .read = io_uart_read,
    .write = io_uart_write,
    .get_tick = io_get_tick,
    .set_reset_pin = io_hw_reset
};


/* ====================================================================
 * FSM Task Implementation
 * ==================================================================== */

/**
 * @brief Initializes the Wi-Fi task, RingBuffers, hardware, and binds the driver object.
 */
void Task_wifi_Init(void) {
    WIFI_LOG("\r\n[TASK_WIFI] Initializing ESP-01S (Non-blocking Mode)...\r\n");
    
    /* 1. Hardware Initialization */
    GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);
    PB9 = 1; 
    
    /* 2. UART & Ring Buffer Initialization */
    Utils_RB_Init(&wifi_rx_rb, wifi_rx_mem, WIFI_RX_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&wifi_tx_rb, wifi_tx_mem, WIFI_TX_BUF_SIZE, sizeof(uint8_t));
    periph_uart_init(&uart_wifi_obj, UART2, 115200, &wifi_rx_rb, &wifi_tx_rb);
    
    /* 3. ESP8266 Driver Binding (Dependency Injection) */
    drv_esp8266_init(&esp01s_module, &wifi_io_contract);
    esp01s_module.on_tcp_data_received = app_on_tcp_data_received;
    
    /* 4. Enable UART Interrupt */
    NVIC_EnableIRQ(UART2_IRQn);
    
    current_wifi_state = WIFI_STATE_INIT;
    is_req_sent = false;
    retry_count = 0;
}

/**
 * @brief Main non-blocking routine for the Wi-Fi task. Must be called repeatedly in the main loop.
 */
void Task_wifi(void) {
    esp8266_res_e res = drv_esp8266_async_poll(&esp01s_module);
    static char cmd_buffer[128];

    switch (current_wifi_state) {
        case WIFI_STATE_INIT:
            wifi_io_contract.set_reset_pin(wifi_io_contract.user_data, false);
            current_wifi_state = WIFI_STATE_CHECK_ALIVE;
            is_req_sent = false;
            break;
            
        case WIFI_STATE_CHECK_ALIVE:
            if (!is_req_sent) {
                drv_esp8266_async_send_at(&esp01s_module, "AT\r\n", 1000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                current_wifi_state = WIFI_STATE_DISABLE_ECHO; 
                is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; is_req_sent = false;
            }
            break;

        case WIFI_STATE_DISABLE_ECHO:
            if (!is_req_sent) {
                drv_esp8266_async_send_at(&esp01s_module, "ATE0\r\n", 1000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                current_wifi_state = WIFI_STATE_SET_MODE; is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; is_req_sent = false;
            }
            break;
            
        case WIFI_STATE_SET_MODE:
            if (!is_req_sent) {
                drv_esp8266_async_send_at(&esp01s_module, "AT+CWMODE=1\r\n", 1000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                current_wifi_state = WIFI_STATE_CONNECTING; is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; is_req_sent = false;
            }
            break;
            
        case WIFI_STATE_CONNECTING:
            if (!is_req_sent) {
                snprintf(cmd_buffer, sizeof(cmd_buffer), "AT+CWJAP=\"%s\",\"%s\"\r\n", current_wifi_ssid, current_wifi_pwd);
                drv_esp8266_async_send_at(&esp01s_module, cmd_buffer, 15000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                current_wifi_state = WIFI_STATE_GET_IP; is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; is_req_sent = false;
            }
            break;
            
        case WIFI_STATE_GET_IP:
            if (!is_req_sent) {
                drv_esp8266_async_send_at(&esp01s_module, "AT+CIFSR\r\n", 2000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                drv_esp8266_async_send_at(&esp01s_module, "AT+CIPMUX=1\r\n", 1000); 
                current_wifi_state = WIFI_STATE_TCP_CONNECT; 
                is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; 
                is_req_sent = false;
            }
            break;

        case WIFI_STATE_TCP_CONNECT:
            if (!is_req_sent) {
                WIFI_LOG("\r\n[WIFI_FSM] Opening TCP connection to broker.hivemq.com:1883...\r\n");
                drv_esp8266_async_send_at(&esp01s_module, "AT+CIPSTART=0,\"TCP\",\"broker.hivemq.com\",1883\r\n", 10000);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                state_timer = Get_TickCount();
                current_wifi_state = WIFI_STATE_TCP_DELAY; 
                is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; 
                is_req_sent = false;
            }
            break;

        case WIFI_STATE_TCP_DELAY:
            if ((Get_TickCount() - state_timer) >= 1000) {
                current_wifi_state = WIFI_STATE_MQTT_CONNECT;
            }
            break;

        case WIFI_STATE_MQTT_CONNECT:
            if (!is_req_sent) {
                WIFI_LOG("[WIFI_FSM] TCP Connected! Sending MQTT CONNECT packet...\r\n");
                static uint8_t mqtt_conn_buf[64]; 
                
                uint16_t pkt_len = mid_mqtt_build_connect(mqtt_conn_buf, "MyNuvoton_Node_001");
                send_mqtt_tcp_with_dump(mqtt_conn_buf, pkt_len);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                WIFI_LOG("\r\n=========================================\r\n");
                WIFI_LOG(" [MQTT PROTOCOL CONNECTED] \r\n");
                WIFI_LOG(" -> Broker : broker.hivemq.com\r\n");
                WIFI_LOG(" -> Wait for CONNACK...\r\n");
                WIFI_LOG("=========================================\r\n\r\n");
                
                current_wifi_state = WIFI_STATE_MQTT_CONNECTED; 
                is_req_sent = false;
                retry_count = 0;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; 
                is_req_sent = false;
            }
            break;
            
        case WIFI_STATE_MQTT_SUBSCRIBE:
            if (!is_req_sent) {
                WIFI_LOG("[WIFI_FSM] Subscribing to topic [nuvoton/cmd]...\r\n");
                static uint8_t mqtt_sub_buf[64]; 
                
                uint16_t pkt_len = mid_mqtt_build_subscribe(mqtt_sub_buf, "nfc_reader/cmd", 1);
                send_mqtt_tcp_with_dump(mqtt_sub_buf, pkt_len);
                is_req_sent = true;
            } else if (res == ESP_RES_OK) {
                current_wifi_state = WIFI_STATE_MQTT_CONNECTED; 
                state_timer = Get_TickCount(); 
                is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; 
                is_req_sent = false;
            }
            break;

        case WIFI_STATE_MQTT_CONNECTED:
            if (!is_req_sent) {
                if ((Get_TickCount() - state_timer) >= 45000) {
                    static uint8_t ping_buf[2];
                    
                    uint16_t pkt_len = mid_mqtt_build_pingreq(ping_buf);
                    send_mqtt_tcp_with_dump(ping_buf, pkt_len);
                    is_req_sent = true;
                }
            } else if (res == ESP_RES_OK) {
                state_timer = Get_TickCount(); 
                is_req_sent = false;
            } else if (res == ESP_RES_ERROR || res == ESP_RES_TIMEOUT) {
                current_wifi_state = WIFI_STATE_ERROR; 
                is_req_sent = false;
            }
            break;
            
        case WIFI_STATE_ERROR:
            if (!is_req_sent) {
                retry_count++;
                if (retry_count >= MAX_RETRY_COUNT) { 
                    current_wifi_state = WIFI_STATE_INIT; 
                    retry_count = 0; 
                } else { 
                    state_timer = Get_TickCount(); 
                    is_req_sent = true; 
                }
            } else {
                if ((Get_TickCount() - state_timer) >= 5000) {
                    current_wifi_state = WIFI_STATE_CHECK_ALIVE; 
                    is_req_sent = false;
                }
            }
            break;
    }
}

/* ====================================================================
 * Public Application Interfaces
 * ==================================================================== */

/**
 * @brief API for other application tasks to publish data to the MQTT Broker.
 */
void Task_Wifi_Publish_MQTT(const char *topic, const char *payload) {
    if (current_wifi_state != WIFI_STATE_MQTT_CONNECTED) {
        WIFI_LOG("[WIFI_APP] Warning: Cannot publish, MQTT not connected.\r\n");
        return;
    }

    static uint8_t pub_buf[256]; 
    WIFI_LOG("\r\n[WIFI_APP] Publishing to [%s]: %s\r\n", topic, payload);
    
    uint16_t pkt_len = mid_mqtt_build_publish(pub_buf, topic, payload);
    send_mqtt_tcp_with_dump(pub_buf, pkt_len);
}

/**
 * @brief Retrieves the current state of the Wi-Fi State Machine.
 */
wifi_fsm_state_e Task_wifi_Get_State(void) { 
    return current_wifi_state; 
}

/**
 * @brief Forces a hardware reset of the Wi-Fi module and attempts to reconnect to a new AP.
 */
void Task_Wifi_Reconnect(const char *ssid, const char *pwd) {
    WIFI_LOG("\r\n[WIFI_APP] Forcing reconnection to SSID: %s\r\n", ssid);
    
    strncpy(current_wifi_ssid, ssid, sizeof(current_wifi_ssid) - 1);
    current_wifi_ssid[sizeof(current_wifi_ssid) - 1] = '\0';
    
    strncpy(current_wifi_pwd, pwd, sizeof(current_wifi_pwd) - 1);
    current_wifi_pwd[sizeof(current_wifi_pwd) - 1] = '\0';

    current_wifi_state = WIFI_STATE_INIT;
    is_req_sent = false;
}

/**
 * @brief Bridge M487 UART IRQ to our Software Object.
 */
void UART2_IRQHandler(void) { 
    periph_uart_irq_process(&uart_wifi_obj); 
}