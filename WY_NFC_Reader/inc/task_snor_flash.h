#ifndef __TASK_SNOR_FLASH_H__
#define __TASK_SNOR_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include "drv_snor_flash.h"

extern drv_snor_t ext_flash;

/**
 * @brief Current state of the SNOR Flash background job.
 */
typedef enum {
    SNOR_JOB_IDLE = 0,      /* Ready for a new operation. */
    SNOR_JOB_BUSY_ERASE,    /* Hardware is performing internal erase operation. */
    SNOR_JOB_BUSY_WRITE,    /* Hardware is performing internal program operation. */
    SNOR_JOB_DONE,          /* The last asynchronous job finished successfully. */
    SNOR_JOB_ERROR          /* The last job failed due to timeout or hardware error. */
} snor_job_state_t;

/**
 * @brief Initialize the SNOR Flash hardware and the internal driver framework.
 * @note This function is synchronous and should be called once during system startup.
 */
void Task_Snor_Flash_Init(void);

/**
 * @brief Main execution engine for the SNOR Flash state machine.
 * @note MUST be called constantly within the main while(1) loop. 
 * It handles background polling of the Flash WIP bit and manages multi-page writes.
 */
void Task_Snor_Flash(void);

/**
 * @brief Retrieves the current state of the background job.
 * @return Current state defined in snor_job_state_t.
 */
snor_job_state_t Task_Snor_Get_State(void);

/**
 * @brief Resets the state from SNOR_JOB_DONE or SNOR_JOB_ERROR back to SNOR_JOB_IDLE.
 * @note Call this after you have acknowledged the completion of a job.
 */
void Task_Snor_Clear_State(void);

/**
 * @brief Performs a high-speed synchronous read from Flash.
 * @param[in]  addr    The target physical address in Flash memory.
 * @param[out] buffer  The SRAM buffer to store the read data.
 * @param[in]  length  Number of bytes to read.
 * @return true if read was successful; false if Flash is currently busy with an async job.
 * @note DMA read is extremely fast, so this remains synchronous for simplicity.
 */
bool Task_Snor_Read(uint32_t addr, uint8_t *buffer, uint32_t length);

/**
 * @brief Starts an asynchronous sector erase operation (4KB).
 * @param[in]  addr    The target physical address in Flash (must be sector-aligned).
 * @return true if the job was successfully started; false if the Flash is busy.
 * @note This function returns immediately. Check Task_Snor_Get_State() for completion.
 */
bool Task_Snor_Erase_Async(uint32_t addr);

/**
 * @brief Starts an asynchronous multi-page write operation.
 * @param[in]  addr    The target physical address in Flash memory.
 * @param[in]  buffer  The SRAM buffer containing data to be written. 
 * Note: The buffer must remain valid until the job is DONE.
 * @param[in]  length  Number of bytes to write.
 * @return true if the job was successfully queued; false if the Flash is busy.
 * @note This handles cross-page boundaries automatically in the background.
 */
bool Task_Snor_Write_Async(uint32_t addr, const uint8_t *buffer, uint32_t length);

#endif /* __TASK_SNOR_FLASH_H__ */