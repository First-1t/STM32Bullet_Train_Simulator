/*******************************************************************************
 * File Name    : headlight.c
 * Description  : Automatic headlight: reads the LDR (A1) and drives the
 *                yellow LED (PA7)
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "headlight.h"

/* Private includes ----------------------------------------------------------*/
#include "adc.h"
#include "led.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* With the LDR wiring on this shield the ADC value rises when it gets dark:
   ADC above this threshold = dark -> headlight on (calibrated on the board) */
#define HEADLIGHT_ON_THRESHOLD  (1500U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Headlight_Update
 * @brief             - Read the light sensor and switch the headlight
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - Call often (it is called inside every wait loop)
 *********************************************************************/
void Headlight_Update(void)
{
    if (ADC_Read(ADC_CH_LDR) > HEADLIGHT_ON_THRESHOLD) {
        LED_Set(LED_YELLOW, true);
    } else {
        LED_Set(LED_YELLOW, false);
    }
}

/*********************************************************************
 * @fn                - Headlight_IsOn
 * @brief             - Current headlight state
 *
 * @param[in]         - none
 *
 * @return            - true = headlight (yellow LED) on
 *********************************************************************/
bool Headlight_IsOn(void)
{
    return LED_IsOn(LED_YELLOW);
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
