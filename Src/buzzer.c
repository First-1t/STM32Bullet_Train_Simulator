/*******************************************************************************
 * File Name    : buzzer.c
 * Description  : Buzzer driver. Generates a square wave of about 2-3 kHz,
 *                works with both active and passive buzzers.
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "buzzer.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define BUZZER_HALF_PERIOD_LOOPS    (600U)   /* busy-loop count for half a period */

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void buzzer_half_period(void);

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Buzzer_Init
 * @brief             - Set the buzzer pin as output (off)
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void Buzzer_Init(void)
{
    GPIO_SetMode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
    GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, false);
}

/*********************************************************************
 * @fn                - Buzzer_Beep
 * @brief             - Beep for a number of square-wave cycles (blocking)
 *
 * @param[in]         - u4t_cycles : number of cycles (100 cycles ~ 40-50 ms)
 *
 * @return            - none
 *
 * @Note              - Buzzer is off when the function returns
 *********************************************************************/
void Buzzer_Beep(uint32_t u4t_cycles)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < u4t_cycles; u4t_i++) {
        GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, true);
        buzzer_half_period();
        GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, false);
        buzzer_half_period();
    }
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*********************************************************************
 * @fn                - buzzer_half_period
 * @brief             - Busy wait for half a square-wave period
 *********************************************************************/
static void buzzer_half_period(void)
{
    volatile uint32_t u4t_loop;

    for (u4t_loop = 0U; u4t_loop < BUZZER_HALF_PERIOD_LOOPS; u4t_loop++) {
        /* wait */
    }
}
