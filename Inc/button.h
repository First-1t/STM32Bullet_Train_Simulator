/*******************************************************************************
 * File Name    : button.h
 * Description  : Header file for button.c (4 push buttons, active-low)
 * Date         : 2026-10-08
 ******************************************************************************/
#ifndef BUTTON_H
#define BUTTON_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/
typedef enum {
    BUTTON_1 = 0,                /* PA10 (D2) */
    BUTTON_2,                    /* PB3  (D3) */
    BUTTON_3,                    /* PB5  (D4) */
    BUTTON_4,                    /* PB4  (D5) */
    BUTTON_COUNT
} button_id_t;

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void Button_Init(void);
bool Button_IsPressed(button_id_t et_button);

#endif /* BUTTON_H */
