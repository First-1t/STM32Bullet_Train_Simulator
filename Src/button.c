/*******************************************************************************
 * File Name    : button.c
 * Description  : Push button driver (active-low with internal pull-up)
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "button.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/
typedef struct {
    GPIO_TypeDef *pt_port;
    uint32_t      u4_pin;
} button_pin_t;

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const button_pin_t stg_button_pins[BUTTON_COUNT] = {
    { BUTTON1_PORT, BUTTON1_PIN },
    { BUTTON2_PORT, BUTTON2_PIN },
    { BUTTON3_PORT, BUTTON3_PIN },
    { BUTTON4_PORT, BUTTON4_PIN }
};

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Button_Init
 * @brief             - Set the 4 button pins as inputs with pull-up
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - PB4 is JTAG NJTRST after reset, it must be set to
 *                      input here before the button can be read
 *********************************************************************/
void Button_Init(void)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < (uint32_t)BUTTON_COUNT; u4t_i++) {
        GPIO_SetMode(stg_button_pins[u4t_i].pt_port, stg_button_pins[u4t_i].u4_pin, GPIO_MODE_INPUT);
        GPIO_SetPullUp(stg_button_pins[u4t_i].pt_port, stg_button_pins[u4t_i].u4_pin);
    }
}

/*********************************************************************
 * @fn                - Button_IsPressed
 * @brief             - Read one button
 *
 * @param[in]         - et_button : button id
 *
 * @return            - true = pressed (pin low), false = released
 *********************************************************************/
bool Button_IsPressed(button_id_t et_button)
{
    bool bt_pressed = false;

    if (et_button < BUTTON_COUNT) {
        bt_pressed = (GPIO_ReadPin(stg_button_pins[et_button].pt_port,
                                   stg_button_pins[et_button].u4_pin) == false);
    } else {
        /* invalid id: report released */
    }
    return bt_pressed;
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
