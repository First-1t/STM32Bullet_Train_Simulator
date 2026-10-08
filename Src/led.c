/*******************************************************************************
 * File Name    : led.c
 * Description  : LED driver for the 4 LEDs on the training shield
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "led.h"

/* Private includes ----------------------------------------------------------*/
#include "timebase.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef *pt_port;
    uint32_t      u4_pin;
} led_pin_t;

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const led_pin_t stg_led_pins[LED_COUNT] = {
    { LED_GREEN_PORT,  LED_GREEN_PIN  },
    { LED_YELLOW_PORT, LED_YELLOW_PIN },
    { LED_RED_PORT,    LED_RED_PIN    },
    { LED_BLUE_PORT,   LED_BLUE_PIN   }
};

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - LED_Init
 * @brief             - Set the 4 LED pins as outputs and switch them off
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void LED_Init(void)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < (uint32_t)LED_COUNT; u4t_i++) {
        GPIO_SetMode(stg_led_pins[u4t_i].pt_port, stg_led_pins[u4t_i].u4_pin, GPIO_MODE_OUTPUT);
    }
    LED_SetAll(false);
}

/*********************************************************************
 * @fn                - LED_Set
 * @brief             - Switch one LED on or off
 *
 * @param[in]         - et_led : LED id
 * @param[in]         - bt_on : true = on, false = off
 *
 * @return            - none
 *********************************************************************/
void LED_Set(led_id_t et_led, bool bt_on)
{
    if (et_led < LED_COUNT) {
        GPIO_WritePin(stg_led_pins[et_led].pt_port, stg_led_pins[et_led].u4_pin, bt_on);
    } else {
        /* invalid id: no action */
    }
}

/*********************************************************************
 * @fn                - LED_IsOn
 * @brief             - Read back whether an LED is on
 *
 * @param[in]         - et_led : LED id
 *
 * @return            - true = on, false = off (or invalid id)
 *********************************************************************/
bool LED_IsOn(led_id_t et_led)
{
    bool bt_on = false;

    if (et_led < LED_COUNT) {
        bt_on = GPIO_ReadOutputPin(stg_led_pins[et_led].pt_port, stg_led_pins[et_led].u4_pin);
    } else {
        /* invalid id: report off */
    }
    return bt_on;
}

/*********************************************************************
 * @fn                - LED_SetAll
 * @brief             - Switch all 4 LEDs on or off
 *
 * @param[in]         - bt_on : true = on, false = off
 *
 * @return            - none
 *********************************************************************/
void LED_SetAll(bool bt_on)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < (uint32_t)LED_COUNT; u4t_i++) {
        GPIO_WritePin(stg_led_pins[u4t_i].pt_port, stg_led_pins[u4t_i].u4_pin, bt_on);
    }
}

/*********************************************************************
 * @fn                - LED_BlinkAll
 * @brief             - Blink all 4 LEDs together (blocking)
 *
 * @param[in]         - u4t_times : number of blinks
 * @param[in]         - u4t_half_period_ms : on time = off time (ms)
 *
 * @return            - none
 *
 * @Note              - LEDs are off when the function returns
 *********************************************************************/
void LED_BlinkAll(uint32_t u4t_times, uint32_t u4t_half_period_ms)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < u4t_times; u4t_i++) {
        LED_SetAll(true);
        Timebase_DelayMs(u4t_half_period_ms);
        LED_SetAll(false);
        Timebase_DelayMs(u4t_half_period_ms);
    }
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
