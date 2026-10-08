/*******************************************************************************
 * File Name    : adc.c
 * Description  : ADC1 driver: software triggered single conversion
 *                Inputs: PA1 (LDR), PA4 (pot on shield), PC2 (external pot)
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define ADC_ONE_CONVERSION      (0U)     /* SQR1.L = 0 -> 1 conversion */

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - ADC_Init
 * @brief             - Set ADC pins to analog mode and switch ADC1 on
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void ADC_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    GPIO_SetMode(ADC_LDR_PORT, ADC_LDR_PIN, GPIO_MODE_ANALOG);
    GPIO_SetMode(ADC_POT_BOARD_PORT, ADC_POT_BOARD_PIN, GPIO_MODE_ANALOG);
    GPIO_SetMode(ADC_POT_EXT_PORT, ADC_POT_EXT_PIN, GPIO_MODE_ANALOG);

    ADC1->SQR1 = ADC_ONE_CONVERSION;
    ADC1->CR2 |= ADC_CR2_ADON;
}

/*********************************************************************
 * @fn                - ADC_Read
 * @brief             - Convert one channel and return the result
 *
 * @param[in]         - u4t_channel : ADC1 channel number (ADC_CH_xxx)
 *
 * @return            - 0..ADC_MAX_VALUE
 *********************************************************************/
uint16_t ADC_Read(uint32_t u4t_channel)
{
    ADC1->SQR3 = u4t_channel;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
        /* wait for end of conversion */
    }
    return (uint16_t)(ADC1->DR & ADC_MAX_VALUE);
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
