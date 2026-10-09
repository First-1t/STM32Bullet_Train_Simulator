/*******************************************************************************
 * File Name    : timebase.c
 * Description  : SysTick based 1 ms time base (interrupt) and blocking delay
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "timebase.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define SYSTICK_LOAD_1MS        ((CORE_CLOCK_HZ / MS_PER_SECOND) - 1U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint32_t u4g_ms = 0U;      /* milliseconds since Timebase_Init */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Timebase_Init
 * @brief             - Start SysTick with an interrupt every 1 ms
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - SysTick is a core exception, no NVIC enable needed.
 *                      DBG_SLEEP keeps the debugger (SWD) working while
 *                      the CPU sleeps in Timebase_Sleep.
 *********************************************************************/
void Timebase_Init(void)
{
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;
    SysTick->LOAD = SYSTICK_LOAD_1MS;
    SysTick->VAL  = 0U;
    SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk);
}

/*********************************************************************
 * @fn                - Timebase_GetMs
 * @brief             - Get the millisecond counter
 *
 * @param[in]         - none
 *
 * @return            - milliseconds since Timebase_Init (wraps after ~49 days)
 *********************************************************************/
uint32_t Timebase_GetMs(void)
{
    return u4g_ms;
}

/*********************************************************************
 * @fn                - Timebase_Sleep
 * @brief             - Sleep (WFI) until the next interrupt wakes the CPU
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - SysTick wakes the CPU at least every 1 ms; UART,
 *                      EXTI and timer interrupts wake it earlier
 *********************************************************************/
void Timebase_Sleep(void)
{
    __WFI();
}

/*********************************************************************
 * @fn                - Timebase_DelayMs
 * @brief             - Delay driven by the SysTick interrupt: the CPU
 *                      sleeps between ticks instead of busy-waiting
 *
 * @param[in]         - u4t_ms : delay time in milliseconds
 *
 * @return            - none
 *********************************************************************/
void Timebase_DelayMs(uint32_t u4t_ms)
{
    uint32_t u4t_start = u4g_ms;

    while ((u4g_ms - u4t_start) < u4t_ms) {
        Timebase_Sleep();
    }
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - SysTick_Handler
 * @brief             - SysTick interrupt: count 1 ms
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - Overrides the weak handler in the startup file
 *********************************************************************/
void SysTick_Handler(void)
{
    u4g_ms++;
}

/* Private functions ---------------------------------------------------------*/
