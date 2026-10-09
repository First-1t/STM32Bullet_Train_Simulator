/*******************************************************************************
 * File Name    : timebase.h
 * Description  : Header file for timebase.c (SysTick 1 ms tick and delay)
 * Date         : 2026-10-08
 ******************************************************************************/
#ifndef TIMEBASE_H
#define TIMEBASE_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/
#define MS_PER_SECOND           (1000U)

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void     Timebase_Init(void);
uint32_t Timebase_GetMs(void);
void     Timebase_Sleep(void);
void     Timebase_DelayMs(uint32_t u4t_ms);
void     SysTick_Handler(void);

#endif /* TIMEBASE_H */
