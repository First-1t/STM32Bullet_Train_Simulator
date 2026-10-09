/*******************************************************************************
 * File Name    : board.h
 * Description  : Board definitions for NUCLEO-F411RE + NEXTY Training Shield 1
 *                (device header, pin map, GPIO helper functions)
 * Date         : 2026-10-08
 ******************************************************************************/
#ifndef BOARD_H
#define BOARD_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported includes ---------------------------------------------------------*/
#ifndef STM32F411xE
#define STM32F411xE
#endif
#include "stm32f4xx.h"

/* Exported typedef ----------------------------------------------------------*/

/* Exported enum -------------------------------------------------------------*/

/* Exported struct -----------------------------------------------------------*/

/* Exported union ------------------------------------------------------------*/

/* Exported define -----------------------------------------------------------*/
/* System clock (HSI default, no PLL) */
#define CORE_CLOCK_HZ           (16000000U)

/* GPIO mode values (MODER) */
#define GPIO_MODE_INPUT         (0U)
#define GPIO_MODE_OUTPUT        (1U)
#define GPIO_MODE_AF            (2U)
#define GPIO_MODE_ANALOG        (3U)

/* ---- Pin map -------------------------------------------------------------*/
/* LED (D13..D10) : PA5 = blue, PA6 = red, PA7 = yellow (headlight), PB6 = green */
#define LED_GREEN_PORT          (GPIOB)
#define LED_GREEN_PIN           (6U)
#define LED_YELLOW_PORT         (GPIOA)
#define LED_YELLOW_PIN          (7U)
#define LED_RED_PORT            (GPIOA)
#define LED_RED_PIN             (6U)
#define LED_BLUE_PORT           (GPIOA)
#define LED_BLUE_PIN            (5U)

/* Buttons (active-low, internal pull-up) : PA10, PB3, PB5, PB4 */
#define BUTTON1_PORT            (GPIOA)
#define BUTTON1_PIN             (10U)
#define BUTTON2_PORT            (GPIOB)
#define BUTTON2_PIN             (3U)
#define BUTTON3_PORT            (GPIOB)
#define BUTTON3_PIN             (5U)
#define BUTTON4_PORT            (GPIOB)
#define BUTTON4_PIN             (4U)

/* 7-segment BCD decoder inputs : 2^0 = PC7, 2^1 = PA8, 2^2 = PB10, 2^3 = PA9 */
#define SEG7_BIT0_PORT          (GPIOC)
#define SEG7_BIT0_PIN           (7U)
#define SEG7_BIT1_PORT          (GPIOA)
#define SEG7_BIT1_PIN           (8U)
#define SEG7_BIT2_PORT          (GPIOB)
#define SEG7_BIT2_PIN           (10U)
#define SEG7_BIT3_PORT          (GPIOA)
#define SEG7_BIT3_PIN           (9U)

/* Buzzer : + -> PC3 (CN7-37), - -> GND */
#define BUZZER_PORT             (GPIOC)
#define BUZZER_PIN              (3U)

/* USART2 (ST-LINK virtual COM port) : PA2 = TX, PA3 = RX, AF7 */
#define UART_TX_PORT            (GPIOA)
#define UART_TX_PIN             (2U)
#define UART_RX_PORT            (GPIOA)
#define UART_RX_PIN             (3U)
#define UART_GPIO_AF            (7U)

/* ADC1 inputs */
#define ADC_LDR_PORT            (GPIOA)          /* A1 : light sensor (LDR)      */
#define ADC_LDR_PIN             (1U)
#define ADC_CH_LDR              (1U)
#define ADC_POT_BOARD_PORT      (GPIOA)          /* A2 : potentiometer on shield */
#define ADC_POT_BOARD_PIN       (4U)
#define ADC_CH_POT_BOARD        (4U)
#define ADC_POT_EXT_PORT        (GPIOC)          /* PC2 : external potentiometer */
#define ADC_POT_EXT_PIN         (2U)
#define ADC_CH_POT_EXT          (12U)

/* RC522 RFID (bit-bang SPI) : SDA/CS = A0, SCK = A3, MOSI = A4, MISO = A5,
   IRQ = D15 (PB8, EXTI8), RST -> 3V3 , 3.3V -> 3V3 , GND -> GND */
#define RC522_CS_PORT           (GPIOA)
#define RC522_CS_PIN            (0U)
#define RC522_SCK_PORT          (GPIOB)
#define RC522_SCK_PIN           (0U)
#define RC522_MOSI_PORT         (GPIOC)
#define RC522_MOSI_PIN          (1U)
#define RC522_MISO_PORT         (GPIOC)
#define RC522_MISO_PIN          (0U)
#define RC522_IRQ_PORT          (GPIOB)
#define RC522_IRQ_PIN           (8U)

/* Exported macro ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void Board_Init(void);
void GPIO_SetMode(GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_mode);
void GPIO_SetPullUp(GPIO_TypeDef *pt_port, uint32_t u4t_pin);
void GPIO_SetAltFunc(GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_af);
void GPIO_WritePin(GPIO_TypeDef *pt_port, uint32_t u4t_pin, bool bt_high);
bool GPIO_ReadPin(const GPIO_TypeDef *pt_port, uint32_t u4t_pin);
bool GPIO_ReadOutputPin(const GPIO_TypeDef *pt_port, uint32_t u4t_pin);

#endif /* BOARD_H */
