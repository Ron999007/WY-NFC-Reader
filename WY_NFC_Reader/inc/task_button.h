#ifndef __TASK_BUTTON_H__
#define __TASK_BUTTON_H__

#include "macro_utils.h"
#include "drv_button.h"

/* Hardware Pin Definitions */
#define BTN_SW PB9


/* Public API for the Button Task */
void Task_Button_Init(void);
void Task_Button(void);

#endif /* __TASK_BUTTON_H__ */