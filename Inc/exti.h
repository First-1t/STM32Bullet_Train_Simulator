/*******************************************************************************
 * File Name    : exti.h
 * Description  : Header file for exti.c (EXTI line setup + EXTI interrupt
 *                handlers that forward each line to its driver)
 * Date         : 2026-10-09
 ******************************************************************************/
#ifndef EXTI_H
#define EXTI_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/
#define EXTI_TRIGGER_RISING     (0x1U)
#define EXTI_TRIGGER_FALLING    (0x2U)
#define EXTI_TRIGGER_BOTH       (EXTI_TRIGGER_RISING | EXTI_TRIGGER_FALLING)

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void Exti_ConfigLine(const GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_trigger);
bool Exti_TakePending(uint32_t u4t_line);
void Exti_ClearPending(uint32_t u4t_line);
void Exti_Mask(uint32_t u4t_line);
void Exti_Unmask(uint32_t u4t_line);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

#endif /* EXTI_H */
