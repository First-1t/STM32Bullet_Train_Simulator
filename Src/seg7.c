/*******************************************************************************
 * File Name    : seg7.c
 * Description  : 7-segment display driver. The shield has a BCD-to-7-segment
 *                decoder, the digit is written as 4-bit binary on 4 pins.
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "seg7.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef *pt_port;
    uint32_t      u4_pin;
} seg7_pin_t;

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SEG7_BCD_BITS           (4U)
#define SEG7_BIT_MASK           (1U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/* index = BCD bit number (2^0 .. 2^3) */
static const seg7_pin_t stg_seg7_pins[SEG7_BCD_BITS] = {
    { SEG7_BIT0_PORT, SEG7_BIT0_PIN },
    { SEG7_BIT1_PORT, SEG7_BIT1_PIN },
    { SEG7_BIT2_PORT, SEG7_BIT2_PIN },
    { SEG7_BIT3_PORT, SEG7_BIT3_PIN }
};

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Seg7_Init
 * @brief             - Set the 4 BCD pins as outputs and show 0
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void Seg7_Init(void)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < SEG7_BCD_BITS; u4t_i++) {
        GPIO_SetMode(stg_seg7_pins[u4t_i].pt_port, stg_seg7_pins[u4t_i].u4_pin, GPIO_MODE_OUTPUT);
    }
    Seg7_Show(0U);
}

/*********************************************************************
 * @fn                - Seg7_Show
 * @brief             - Show one digit
 *
 * @param[in]         - u1t_digit : 0..9 (values above 9 are limited to 9)
 *
 * @return            - none
 *********************************************************************/
void Seg7_Show(uint8_t u1t_digit)
{
    uint32_t u4t_value = (uint32_t)u1t_digit;
    uint32_t u4t_i;

    if (u4t_value > SEG7_MAX_DIGIT) {
        u4t_value = SEG7_MAX_DIGIT;
    } else {
        /* No action */
    }

    for (u4t_i = 0U; u4t_i < SEG7_BCD_BITS; u4t_i++) {
        GPIO_WritePin(stg_seg7_pins[u4t_i].pt_port, stg_seg7_pins[u4t_i].u4_pin,
                      ((u4t_value >> u4t_i) & SEG7_BIT_MASK) != 0U);
    }
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
