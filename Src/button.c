/*******************************************************************************
 * File Name    : button.c
 * Description  : Push button driver (active-low with internal pull-up),
 *                fully interrupt driven - the pins are never polled.
 *                EXTI : every button raises its EXTI line on both edges
 *                       (PA10 = line 10, PB3 = line 3, PB5 = line 5,
 *                        PB4 = line 4). Button_ExtiCallback (called by the
 *                       EXTI handlers in exti.c) masks the line (ignore
 *                       contact bounce) and starts the debounce timer.
 *                TIM3 : one-pulse 20 ms timer. Its update interrupt reads
 *                       the settled level, updates the pressed state and
 *                       counts a new press, then unmasks the EXTI line.
 * Date         : 2026-10-09
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "button.h"

/* Private includes ----------------------------------------------------------*/
#include "exti.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef *pt_port;
    uint32_t      u4_pin;        /* pin number = EXTI line number */
} button_pin_t;

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define BUTTON_DEBOUNCE_MS      (20U)
#define DEBOUNCE_TIMER_HZ       (1000U)  /* TIM3 counts milliseconds        */
#define DEBOUNCE_PSC            ((CORE_CLOCK_HZ / DEBOUNCE_TIMER_HZ) - 1U)
#define DEBOUNCE_ARR            (BUTTON_DEBOUNCE_MS - 1U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const button_pin_t stg_button_pins[BUTTON_COUNT] = {
    { BUTTON1_PORT, BUTTON1_PIN },
    { BUTTON2_PORT, BUTTON2_PIN },
    { BUTTON3_PORT, BUTTON3_PIN },
    { BUTTON4_PORT, BUTTON4_PIN }
};

/* Private variables ---------------------------------------------------------*/
static volatile bool     bg_pressed[BUTTON_COUNT];      /* debounced state        */
static volatile uint32_t u4g_presses[BUTTON_COUNT];     /* presses not taken yet  */
static volatile bool     bg_debouncing[BUTTON_COUNT];   /* line masked, wait TIM3 */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static bool button_pin_low(uint32_t u4t_index);
static void button_start_debounce(void);
static void button_settle(uint32_t u4t_index);

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Button_Init
 * @brief             - TIM3 as debounce timer, button pins as inputs with
 *                      pull-up and an EXTI line (both edges) for every button
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - PB4 is JTAG NJTRST after reset, it must be set to
 *                      input here before the button can be used
 *********************************************************************/
void Button_Init(void)
{
    uint32_t u4t_i;

    /* TIM3 : 1 kHz count, stops by itself after BUTTON_DEBOUNCE_MS (one-pulse) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->CR1 = TIM_CR1_OPM;
    TIM3->PSC = DEBOUNCE_PSC;
    TIM3->ARR = DEBOUNCE_ARR;
    TIM3->EGR = TIM_EGR_UG;                 /* load PSC / ARR now */
    TIM3->SR = 0U;                          /* drop the flag made by UG */
    TIM3->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM3_IRQn);

    for (u4t_i = 0U; u4t_i < (uint32_t)BUTTON_COUNT; u4t_i++) {
        GPIO_SetMode(stg_button_pins[u4t_i].pt_port, stg_button_pins[u4t_i].u4_pin, GPIO_MODE_INPUT);
        GPIO_SetPullUp(stg_button_pins[u4t_i].pt_port, stg_button_pins[u4t_i].u4_pin);
        bg_pressed[u4t_i] = button_pin_low(u4t_i);
        u4g_presses[u4t_i] = 0U;
        bg_debouncing[u4t_i] = false;
        /* press = falling edge, release = rising edge */
        Exti_ConfigLine(stg_button_pins[u4t_i].pt_port, stg_button_pins[u4t_i].u4_pin, EXTI_TRIGGER_BOTH);
    }
}

/*********************************************************************
 * @fn                - Button_IsPressed
 * @brief             - Debounced state of one button (kept by the interrupts)
 *
 * @param[in]         - et_button : button id
 *
 * @return            - true = pressed, false = released
 *********************************************************************/
bool Button_IsPressed(button_id_t et_button)
{
    bool bt_pressed = false;

    if (et_button < BUTTON_COUNT) {
        bt_pressed = bg_pressed[et_button];
    } else {
        /* invalid id: report released */
    }
    return bt_pressed;
}

/*********************************************************************
 * @fn                - Button_TakePresses
 * @brief             - Number of new presses since the last call, then
 *                      reset the count of that button
 *
 * @param[in]         - et_button : button id
 *
 * @return            - number of presses (0 = none)
 *********************************************************************/
uint32_t Button_TakePresses(button_id_t et_button)
{
    uint32_t u4t_count = 0U;
    uint32_t u4t_primask;

    if (et_button < BUTTON_COUNT) {
        u4t_primask = __get_PRIMASK();
        __disable_irq();                    /* read + clear without losing a press */
        u4t_count = u4g_presses[et_button];
        u4g_presses[et_button] = 0U;
        __set_PRIMASK(u4t_primask);
    } else {
        /* invalid id: no presses */
    }
    return u4t_count;
}

/*********************************************************************
 * @fn                - Button_ClearPresses
 * @brief             - Forget the presses of every button (pressed in an
 *                      earlier part of the game)
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void Button_ClearPresses(void)
{
    uint32_t u4t_i;
    uint32_t u4t_primask = __get_PRIMASK();

    __disable_irq();
    for (u4t_i = 0U; u4t_i < (uint32_t)BUTTON_COUNT; u4t_i++) {
        u4g_presses[u4t_i] = 0U;
    }
    __set_PRIMASK(u4t_primask);
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - Button_ExtiCallback
 * @brief             - Called by the EXTI handlers (exti.c): for every
 *                      button line with an edge, clear it, mask it and
 *                      start the debounce timer
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void Button_ExtiCallback(void)
{
    uint32_t u4t_i;
    uint32_t u4t_line;

    for (u4t_i = 0U; u4t_i < (uint32_t)BUTTON_COUNT; u4t_i++) {
        u4t_line = stg_button_pins[u4t_i].u4_pin;
        if (Exti_TakePending(u4t_line) == true) {
            Exti_Mask(u4t_line);            /* ignore bounce until TIM3 fires */
            bg_debouncing[u4t_i] = true;
            button_start_debounce();
        } else {
            /* No action */
        }
    }
}

/*********************************************************************
 * @fn                - TIM3_IRQHandler
 * @brief             - Debounce time is over: settle every button that
 *                      is waiting
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void TIM3_IRQHandler(void)
{
    uint32_t u4t_i;

    if ((TIM3->SR & TIM_SR_UIF) != 0U) {
        TIM3->SR = ~TIM_SR_UIF;             /* rc_w0: write 0 to clear */
        for (u4t_i = 0U; u4t_i < (uint32_t)BUTTON_COUNT; u4t_i++) {
            if (bg_debouncing[u4t_i] == true) {
                button_settle(u4t_i);
            } else {
                /* No action */
            }
        }
    } else {
        /* No action */
    }
}

/* Private functions ---------------------------------------------------------*/
/*********************************************************************
 * @fn                - button_pin_low
 * @brief             - Level of a button pin (low = pressed)
 *********************************************************************/
static bool button_pin_low(uint32_t u4t_index)
{
    return (GPIO_ReadPin(stg_button_pins[u4t_index].pt_port,
                         stg_button_pins[u4t_index].u4_pin) == false);
}

/*********************************************************************
 * @fn                - button_start_debounce
 * @brief             - (Re)start the one-pulse debounce timer from 0
 *********************************************************************/
static void button_start_debounce(void)
{
    TIM3->CNT = 0U;
    TIM3->CR1 |= TIM_CR1_CEN;
}

/*********************************************************************
 * @fn                - button_settle
 * @brief             - Take the settled level of one button after the
 *                      debounce time, count a new press, unmask its line
 *
 * @Note              - If the level changed again while the line was
 *                      masked, the line stays masked and the timer restarts
 *********************************************************************/
static void button_settle(uint32_t u4t_index)
{
    uint32_t u4t_line = stg_button_pins[u4t_index].u4_pin;
    bool     bt_low = button_pin_low(u4t_index);

    if (bt_low != bg_pressed[u4t_index]) {
        bg_pressed[u4t_index] = bt_low;
        if (bt_low == true) {
            u4g_presses[u4t_index]++;
        } else {
            /* released */
        }
    } else {
        /* only bounce, no change */
    }

    Exti_ClearPending(u4t_line);
    Exti_Unmask(u4t_line);
    if (button_pin_low(u4t_index) != bg_pressed[u4t_index]) {
        Exti_Mask(u4t_line);                /* changed again: wait once more */
        button_start_debounce();
    } else {
        bg_debouncing[u4t_index] = false;
    }
}
