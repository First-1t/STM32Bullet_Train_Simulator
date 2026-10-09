/*******************************************************************************
 * File Name    : buzzer.c
 * Description  : Buzzer driver using TIM2 update interrupt.
 *                TIM2 runs at 1 MHz and overflows every 200 us; each update
 *                interrupt toggles the buzzer pin, giving a 2.5 kHz square
 *                wave (works with both active and passive buzzers).
 *                Buzzer_Beep() only starts the timer and returns at once,
 *                the interrupt stops the timer when the beep is finished.
 * Date         : 2026-10-09
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "buzzer.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define BUZZER_TIMER_HZ         (1000000U)   /* TIM2 counter clock               */
#define BUZZER_TOGGLE_HZ        (5000U)      /* update rate = 2 x tone (2.5 kHz) */
#define BUZZER_PSC              ((CORE_CLOCK_HZ / BUZZER_TIMER_HZ) - 1U)
#define BUZZER_ARR              ((BUZZER_TIMER_HZ / BUZZER_TOGGLE_HZ) - 1U)
#define BUZZER_TOGGLES_PER_CYCLE (2U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t u4g_toggles_left = 0U;   /* decremented by TIM2_IRQHandler */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Buzzer_Init
 * @brief             - Buzzer pin as output (off), TIM2 at 5 kHz update rate
 *                      with update interrupt enabled in the NVIC
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - TIM2 stays stopped until Buzzer_Beep is called
 *********************************************************************/
void Buzzer_Init(void)
{
    GPIO_SetMode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
    GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, false);

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->CR1 = 0U;
    TIM2->PSC = BUZZER_PSC;
    TIM2->ARR = BUZZER_ARR;
    TIM2->EGR = TIM_EGR_UG;                 /* load PSC / ARR now */
    TIM2->SR = 0U;                          /* drop the flag made by UG */
    TIM2->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
}

/*********************************************************************
 * @fn                - Buzzer_Beep
 * @brief             - Start a beep of a number of square-wave cycles
 *
 * @param[in]         - u4t_cycles : number of cycles (100 cycles = 40 ms)
 *
 * @return            - none
 *
 * @Note              - Non-blocking: returns at once, TIM2 interrupt makes
 *                      the sound. A new call restarts the beep length.
 *********************************************************************/
void Buzzer_Beep(uint32_t u4t_cycles)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
    u4g_toggles_left = u4t_cycles * BUZZER_TOGGLES_PER_CYCLE;
    GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, false);
    TIM2->CNT = 0U;
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - TIM2_IRQHandler
 * @brief             - TIM2 update interrupt: toggle the buzzer pin, stop the
 *                      timer when the requested cycles are done
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U) {
        TIM2->SR = ~TIM_SR_UIF;             /* rc_w0: write 0 to clear */
        if (u4g_toggles_left > 0U) {
            GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
                          (GPIO_ReadOutputPin(BUZZER_PORT, BUZZER_PIN) == false));
            u4g_toggles_left--;
        } else {
            TIM2->CR1 &= ~TIM_CR1_CEN;      /* beep finished */
            GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, false);
        }
    } else {
        /* No action */
    }
}

/* Private functions ---------------------------------------------------------*/
