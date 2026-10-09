/*******************************************************************************
 * File Name    : adc.c
 * Description  : ADC1 driver with DMA. ADC1 scans 3 channels continuously and
 *                DMA2 Stream0 (channel 0) copies every result into a circular
 *                buffer, so the latest values are always ready without the
 *                CPU waiting for an end of conversion (no polling).
 *                Scan order: PA1 (LDR, ch1), PC2 (external pot, ch12),
 *                            PA4 (pot on shield, ch4)
 * Date         : 2026-10-09
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* Private includes ----------------------------------------------------------*/
#include "timebase.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define ADC_SCAN_COUNT          (3U)     /* channels in the scan sequence      */
#define ADC_INDEX_LDR           (0U)     /* position in the scan / DMA buffer  */
#define ADC_INDEX_POT_EXT       (1U)
#define ADC_INDEX_POT_BOARD     (2U)
#define ADC_SQR_BITS            (5U)     /* bits per entry in SQRx             */
#define ADC_SMP_84_CYCLES       (4U)     /* SMPx = 100b : 84 ADC clock cycles  */
#define ADC_SMP_BITS            (3U)     /* bits per channel in SMPRx          */
#define ADC_SMPR1_FIRST_CH      (10U)    /* SMPR1 holds channels 10..18        */
#define ADC_STABILIZE_MS        (1U)     /* tSTAB after ADON (a few us needed) */
#define DMA_CHANNEL_ADC1        (0U)     /* DMA2 Stream0 channel 0 = ADC1      */
#define DMA_SIZE_16BIT          (1U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile uint16_t u2g_adc_buf[ADC_SCAN_COUNT];   /* written by DMA */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - ADC_Init
 * @brief             - Analog pins, DMA2 Stream0 (circular) and ADC1 in
 *                      continuous scan mode, then start the conversions
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - Timebase_Init must be called first (stabilisation delay).
 *                      Call once after reset (stream must still be disabled).
 *********************************************************************/
void ADC_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    GPIO_SetMode(ADC_LDR_PORT, ADC_LDR_PIN, GPIO_MODE_ANALOG);
    GPIO_SetMode(ADC_POT_EXT_PORT, ADC_POT_EXT_PIN, GPIO_MODE_ANALOG);
    GPIO_SetMode(ADC_POT_BOARD_PORT, ADC_POT_BOARD_PIN, GPIO_MODE_ANALOG);

    /* DMA2 Stream0 : ADC1->DR -> u2g_adc_buf[], 16-bit, memory increment, circular
       (the stream is disabled after reset, ADC_Init is called once) */
    DMA2_Stream0->PAR  = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)u2g_adc_buf;
    DMA2_Stream0->NDTR = ADC_SCAN_COUNT;
    DMA2_Stream0->CR   = ((DMA_CHANNEL_ADC1 << DMA_SxCR_CHSEL_Pos) |
                          (DMA_SIZE_16BIT << DMA_SxCR_MSIZE_Pos) |
                          (DMA_SIZE_16BIT << DMA_SxCR_PSIZE_Pos) |
                          DMA_SxCR_MINC | DMA_SxCR_CIRC);
    DMA2_Stream0->CR  |= DMA_SxCR_EN;

    /* ADC1 : scan 3 channels, 84-cycle sampling (high impedance sources) */
    ADC1->CR1   = ADC_CR1_SCAN;
    ADC1->SMPR1 = (ADC_SMP_84_CYCLES << ((ADC_CH_POT_EXT - ADC_SMPR1_FIRST_CH) * ADC_SMP_BITS));
    ADC1->SMPR2 = ((ADC_SMP_84_CYCLES << (ADC_CH_LDR * ADC_SMP_BITS)) |
                   (ADC_SMP_84_CYCLES << (ADC_CH_POT_BOARD * ADC_SMP_BITS)));
    ADC1->SQR1  = ((ADC_SCAN_COUNT - 1U) << ADC_SQR1_L_Pos);
    ADC1->SQR3  = ((ADC_CH_LDR << (ADC_INDEX_LDR * ADC_SQR_BITS)) |
                   (ADC_CH_POT_EXT << (ADC_INDEX_POT_EXT * ADC_SQR_BITS)) |
                   (ADC_CH_POT_BOARD << (ADC_INDEX_POT_BOARD * ADC_SQR_BITS)));

    /* continuous conversion, DMA requests keep going (DDS), power on */
    ADC1->CR2 = (ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_ADON);
    Timebase_DelayMs(ADC_STABILIZE_MS);
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

/*********************************************************************
 * @fn                - ADC_Read
 * @brief             - Latest value of one channel (copied there by DMA)
 *
 * @param[in]         - u4t_channel : ADC_CH_LDR, ADC_CH_POT_EXT or ADC_CH_POT_BOARD
 *
 * @return            - 0..ADC_MAX_VALUE (0 for a channel that is not scanned)
 *
 * @Note              - Never waits: the result is read from the DMA buffer
 *********************************************************************/
uint16_t ADC_Read(uint32_t u4t_channel)
{
    uint16_t u2t_value = 0U;

    switch (u4t_channel) {
        case ADC_CH_LDR:
            u2t_value = u2g_adc_buf[ADC_INDEX_LDR];
            break;
        case ADC_CH_POT_EXT:
            u2t_value = u2g_adc_buf[ADC_INDEX_POT_EXT];
            break;
        case ADC_CH_POT_BOARD:
            u2t_value = u2g_adc_buf[ADC_INDEX_POT_BOARD];
            break;
        default:
            /* channel not in the scan sequence */
            break;
    }
    return (uint16_t)(u2t_value & ADC_MAX_VALUE);
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
