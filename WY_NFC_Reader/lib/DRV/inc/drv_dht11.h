#ifndef _DRV_DHT11_H_
#define _DRV_DHT11_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DHT11_STATE_IDLE = 0,
    DHT11_STATE_WAIT_RESPONSE_LOW,  
    DHT11_STATE_WAIT_RESPONSE_HIGH, 
    DHT11_STATE_ACK_END,            /* NEW: Wait for the 80us ACK high pulse to finish */
    DHT11_STATE_RECEIVE_DATA,       
    DHT11_STATE_DATA_READY,         
    DHT11_STATE_ERROR               
} DHT11_State_t;

typedef struct {
    uint8_t temperature;
    uint8_t humidity;
} DHT11_Data_t;

typedef struct {
    void (*pin_mode_out)(void);
    void (*pin_mode_in_int)(void); 
    void (*pin_write)(uint8_t level);
    uint8_t (*pin_read)(void);
    uint32_t (*get_time_us)(void); 
} DHT11_Interface_t;

typedef struct {
    DHT11_Interface_t hw;
    DHT11_State_t state;
    DHT11_Data_t data;
    
    uint32_t last_edge_us;
    uint8_t bit_index;
    uint8_t raw_data[5];
} DHT11_Device_t;

void DHT11_Init(DHT11_Device_t *dev, DHT11_Interface_t *interface);
void DHT11_Start_Trigger(DHT11_Device_t *dev);
void DHT11_Handle_External_Interrupt(DHT11_Device_t *dev);

#endif /* _DRV_DHT11_H_ */