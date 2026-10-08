/*******************************************************************************
 * File Name    : led.h
 * Description  : Header file for led.c (4 LEDs on the training shield)
 * Date         : 2026-10-08
 ******************************************************************************/
#ifndef LED_H
#define LED_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/
/* Order = gauge order (green -> yellow -> red -> blue) */
typedef enum {
    LED_GREEN = 0,               /* PB6 : driving               */
    LED_YELLOW,                  /* PA7 : headlight             */
    LED_RED,                     /* PA6 : alarm                 */
    LED_BLUE,                    /* PA5 : near station          */
    LED_COUNT
} led_id_t;

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void LED_Init(void);
void LED_Set(led_id_t et_led, bool bt_on);
bool LED_IsOn(led_id_t et_led);
void LED_SetAll(bool bt_on);
void LED_BlinkAll(uint32_t u4t_times, uint32_t u4t_half_period_ms);

#endif /* LED_H */
