/*******************************************************************************
 * File Name    : rc522.h
 * Description  : Header file for rc522.c (MFRC522 RFID reader, bit-bang SPI,
 *                IRQ pin on EXTI8)
 * Date         : 2026-10-09
 ******************************************************************************/
#ifndef RC522_H
#define RC522_H

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported includes ---------------------------------------------------------*/

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/
#define RC522_UID_LEN           (4U)     /* bytes of a 4-byte (single size) UID */

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void    RC522_Init(void);
uint8_t RC522_GetVersion(void);
bool    RC522_IsAlive(void);
bool    RC522_ReadUid(uint8_t *pt_uid);
bool    RC522_IsIrqPinOk(void);
void    RC522_ExtiCallback(void);

#endif /* RC522_H */
