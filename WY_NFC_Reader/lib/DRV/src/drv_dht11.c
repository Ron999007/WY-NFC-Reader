#include "drv_dht11.h"
#include <stddef.h>

void DHT11_Init(DHT11_Device_t *dev, DHT11_Interface_t *interface) {
    if (dev != NULL && interface != NULL) {
        dev->hw = *interface;
        dev->state = DHT11_STATE_IDLE;
        
        dev->hw.pin_mode_out();
        dev->hw.pin_write(1);
    }
}

void DHT11_Start_Trigger(DHT11_Device_t *dev) {
    dev->hw.pin_mode_out();
    dev->hw.pin_write(0);
    dev->state = DHT11_STATE_IDLE; 
}

void DHT11_Handle_External_Interrupt(DHT11_Device_t *dev) {
    uint32_t current_time = dev->hw.get_time_us();
    uint32_t delta = (current_time - dev->last_edge_us) & 0xFFFFFF;
    uint8_t pin_level = dev->hw.pin_read();
    
    dev->last_edge_us = current_time;

    switch (dev->state) {
        case DHT11_STATE_WAIT_RESPONSE_LOW:
            if (pin_level == 0) {
                dev->state = DHT11_STATE_WAIT_RESPONSE_HIGH;
            }
            break;

        case DHT11_STATE_WAIT_RESPONSE_HIGH:
            if (pin_level == 1) {
                /* Sensor is pulling high for 80us ACK. Move to ACK_END. */
                dev->state = DHT11_STATE_ACK_END;
            }
            break;

        case DHT11_STATE_ACK_END:
            if (pin_level == 0) {
                /* The 80us ACK high pulse is over. NOW we are ready for data. */
                dev->state = DHT11_STATE_RECEIVE_DATA;
                dev->bit_index = 0;
                for(int i = 0; i < 5; i++) dev->raw_data[i] = 0;
            }
            break;

        case DHT11_STATE_RECEIVE_DATA:
            /* Measure High pulse duration. pin_level == 0 means High pulse just ended */
            if (pin_level == 0) {
                uint8_t byte_idx = dev->bit_index / 8;
                dev->raw_data[byte_idx] <<= 1;
                
                /* Bit 0 is ~28us, Bit 1 is ~70us. Threshold is ~45us */
                if (delta > 45 && delta < 100) {
                    dev->raw_data[byte_idx] |= 0x01;
                }
                
                dev->bit_index++;
                
                if (dev->bit_index >= 40) {
                    uint8_t checksum = dev->raw_data[0] + dev->raw_data[1] + 
                                       dev->raw_data[2] + dev->raw_data[3];
                    if (checksum == dev->raw_data[4]) {
                        dev->data.humidity = dev->raw_data[0];
                        dev->data.temperature = dev->raw_data[2];
                        dev->state = DHT11_STATE_DATA_READY;
                    } else {
                        dev->state = DHT11_STATE_ERROR;
                    }
                }
            }
            break;

        default:
            break;
    }
}