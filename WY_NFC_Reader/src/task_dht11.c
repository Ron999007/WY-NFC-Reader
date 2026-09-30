#include "MyApplication.h"

DHT11_Device_t my_dht11;

static Task_DHT11_State_t app_state = TASK_DHT11_IDLE;
static uint32_t start_pulse_timestamp_us = 0;
static uint32_t last_read_timestamp_us = 0;
static uint32_t dht11_debug_edge_count = 0;

/* Read interval (2,000,000 us = 2 seconds) */
#define DHT11_READ_INTERVAL_US 2000000 

/* Helper function to handle 24-bit timer rollover safely */
static uint32_t get_time_diff(uint32_t start, uint32_t end) {
    return (end - start) & 0xFFFFFF;
}

/* =========================================
 * M480 Hardware Wrappers (PE.4 & TIMER1)
 * ========================================= */

static void m480_pin_mode_out(void) {
    /* Set PE.4 to Standard Push-Pull Output Mode. 
     * Note: This eliminates the need for an external pull-up resistor, 
     * but carries a slight risk of brief short-circuit during bus handover.
     */
    GPIO_SetMode(PE, BIT4, GPIO_MODE_OUTPUT);
    GPIO_DisableInt(PE, 4);
}
 
static void m480_pin_mode_in_int(void) {
    GPIO_SetMode(PE, BIT4, GPIO_MODE_INPUT);
    GPIO_EnableInt(PE, 4, GPIO_INT_BOTH_EDGE);
    GPIO_DISABLE_DEBOUNCE(PE, BIT4); 
    
    /* Highest priority (0) to prevent UART/WIFI from dropping DHT11 edges */
    NVIC_SetPriority(GPE_IRQn, 0);
    NVIC_EnableIRQ(GPE_IRQn);
}

static void m480_pin_write(uint8_t level) {
    PE4 = level ? 1 : 0; 
}

static uint8_t m480_pin_read(void) {
    return PE4;
}

static uint32_t m480_get_time_us(void) {
    return TIMER_GetCounter(TIMER1);
}

/* =========================================
 * EXTI Interrupt Service Routine (Port E)
 * ========================================= */

void GPE_IRQHandler(void) {
    if(GPIO_GET_INT_FLAG(PE, BIT4)) {
        GPIO_CLR_INT_FLAG(PE, BIT4);
        
        dht11_debug_edge_count++;
        DHT11_Handle_External_Interrupt(&my_dht11);
    } else {
        PE->INTSRC = PE->INTSRC;
    }
}

/* =========================================
 * Application Task Initialization
 * ========================================= */

void Task_DHT11_Init(void) {
    
    TIMER_Open(TIMER1, TIMER_CONTINUOUS_MODE, 1);
    /* (12MHz / 12 = 1MHz) */
    TIMER_SET_PRESCALE_VALUE(TIMER1, 11);
    TIMER_Start(TIMER1);                   

    /* Bind hardware wrappers */
    DHT11_Interface_t m480_intf = {
        .pin_mode_out    = m480_pin_mode_out,
        .pin_mode_in_int = m480_pin_mode_in_int,
        .pin_write       = m480_pin_write,
        .pin_read        = m480_pin_read,
        .get_time_us     = m480_get_time_us
    };
    
    DHT11_Init(&my_dht11, &m480_intf);
    
    app_state = TASK_DHT11_IDLE;
    last_read_timestamp_us = m480_get_time_us();
}

/* =========================================
 * Main Non-blocking Run Loop
 * ========================================= */

void Task_DHT11(void) {
    uint32_t current_time = m480_get_time_us();

    switch (app_state) {
        case TASK_DHT11_IDLE:
            if (get_time_diff(last_read_timestamp_us, current_time) >= DHT11_READ_INTERVAL_US) {
                last_read_timestamp_us = current_time;
                dht11_debug_edge_count = 0;
                
                DHT11_Start_Trigger(&my_dht11);
                
                start_pulse_timestamp_us = current_time;
                app_state = TASK_DHT11_WAITING_START_PULSE;
            }
            break;

        case TASK_DHT11_WAITING_START_PULSE:
            if (get_time_diff(start_pulse_timestamp_us, current_time) >= 20000) {
                m480_pin_write(1);
                m480_pin_mode_in_int();
                my_dht11.state = DHT11_STATE_WAIT_RESPONSE_LOW;
                my_dht11.last_edge_us = m480_get_time_us(); 
                app_state = TASK_DHT11_WAITING_DATA;
            }
            break;

        case TASK_DHT11_WAITING_DATA:
            if (my_dht11.state == DHT11_STATE_DATA_READY) {
                printf("[DHT11] SUCCESS -> Hum: %d %%, Temp: %d C\n", 
                my_dht11.data.humidity, my_dht11.data.temperature);
                
                /* ---------------------------------------------------
                 * NEW: Format data as JSON and publish via MQTT 
                 * Format: {"temp":25,"humi":60}
                 * --------------------------------------------------- */
                char mqtt_payload[64];
                snprintf(mqtt_payload, sizeof(mqtt_payload), "{\"temp\":%d,\"humi\":%d}", 
                         my_dht11.data.temperature, my_dht11.data.humidity);
                
                /* Publish to the specified broker topic */
                Task_Wifi_Publish_MQTT("nfc_reader/DHT11", mqtt_payload);
                /* --------------------------------------------------- */
                        
                m480_pin_mode_out();
                m480_pin_write(1);
                my_dht11.state = DHT11_STATE_IDLE;
                app_state = TASK_DHT11_IDLE;
                
            } else if (my_dht11.state == DHT11_STATE_ERROR) {
                printf("[DHT11] ERROR: Checksum/Protocol (Edges: %u)\n", dht11_debug_edge_count);
                
                m480_pin_mode_out();
                m480_pin_write(1);
                my_dht11.state = DHT11_STATE_IDLE;
                app_state = TASK_DHT11_IDLE;
                
            } else if (get_time_diff(start_pulse_timestamp_us, current_time) > 50000) {
                printf("[DHT11] TIMEOUT: Sensor not responding (Edges: %u)\n", dht11_debug_edge_count);
                
                m480_pin_mode_out();
                m480_pin_write(1);
                my_dht11.state = DHT11_STATE_IDLE;
                app_state = TASK_DHT11_IDLE;
            }
            break;
            
        default:
            app_state = TASK_DHT11_IDLE;
            break;
    }
}