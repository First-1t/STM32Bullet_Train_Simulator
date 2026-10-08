/*******************************************************************************
 * File Name    : uart.h
 * Description  : Header file for uart.c (USART2, 115200 8N1, polling)
 * Date         : 2026-10-08
 ******************************************************************************/
#ifndef UART_H
#define UART_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void UART_Init(void);
void UART_SendChar(char ct_ch);
void UART_SendString(const char *pt_str);
void UART_SendUint(uint32_t u4t_value);
void UART_SendHex8(uint8_t u1t_value);
bool UART_ReadChar(char *pt_ch);
void UART_FlushRx(void);

#endif /* UART_H */
