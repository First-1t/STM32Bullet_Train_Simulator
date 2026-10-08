/*******************************************************************************
 * File Name    : board.c
 * Description  : Board level helpers: GPIO clock enable, GPIO pin configuration
 *                and SystemInit (called by startup code)
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define GPIO_FIELD2_BITS        (2U)     /* MODER / PUPDR : 2 bits per pin   */
#define GPIO_FIELD2_MASK        (3U)
#define GPIO_PULL_UP            (1U)
#define GPIO_AF_BITS            (4U)     /* AFR : 4 bits per pin             */
#define GPIO_AF_MASK            (0xFU)
#define GPIO_AFR_PINS           (8U)     /* pins per AFR register            */
#define GPIO_BSRR_RESET_SHIFT   (16U)    /* BSRR upper half = reset bits     */
#define GPIO_PIN_BIT            (1UL)
#define FPU_CP10_CP11_FULL      ((3UL << 20U) | (3UL << 22U))

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Board_Init
 * @brief             - Enable the clocks of GPIO ports A, B and C
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - Call before any driver that uses GPIO
 *********************************************************************/
void Board_Init(void)
{
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN);
}

/*********************************************************************
 * @fn                - GPIO_SetMode
 * @brief             - Set the mode of one GPIO pin (input/output/AF/analog)
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 * @param[in]         - u4t_mode : GPIO_MODE_xxx
 *
 * @return            - none
 *********************************************************************/
void GPIO_SetMode(GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_mode)
{
    uint32_t u4t_shift = u4t_pin * GPIO_FIELD2_BITS;

    pt_port->MODER &= ~(GPIO_FIELD2_MASK << u4t_shift);
    pt_port->MODER |= ((u4t_mode & GPIO_FIELD2_MASK) << u4t_shift);
}

/*********************************************************************
 * @fn                - GPIO_SetPullUp
 * @brief             - Enable the internal pull-up resistor of one pin
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 *
 * @return            - none
 *********************************************************************/
void GPIO_SetPullUp(GPIO_TypeDef *pt_port, uint32_t u4t_pin)
{
    uint32_t u4t_shift = u4t_pin * GPIO_FIELD2_BITS;

    pt_port->PUPDR &= ~(GPIO_FIELD2_MASK << u4t_shift);
    pt_port->PUPDR |= (GPIO_PULL_UP << u4t_shift);
}

/*********************************************************************
 * @fn                - GPIO_SetAltFunc
 * @brief             - Select the alternate function number of one pin
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 * @param[in]         - u4t_af : alternate function number 0..15
 *
 * @return            - none
 *
 * @Note              - Pin must also be set to GPIO_MODE_AF
 *********************************************************************/
void GPIO_SetAltFunc(GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_af)
{
    uint32_t u4t_index = u4t_pin / GPIO_AFR_PINS;
    uint32_t u4t_shift = (u4t_pin % GPIO_AFR_PINS) * GPIO_AF_BITS;

    pt_port->AFR[u4t_index] &= ~(GPIO_AF_MASK << u4t_shift);
    pt_port->AFR[u4t_index] |= ((u4t_af & GPIO_AF_MASK) << u4t_shift);
}

/*********************************************************************
 * @fn                - GPIO_WritePin
 * @brief             - Drive one output pin high or low (atomic via BSRR)
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 * @param[in]         - bt_high : true = high, false = low
 *
 * @return            - none
 *********************************************************************/
void GPIO_WritePin(GPIO_TypeDef *pt_port, uint32_t u4t_pin, bool bt_high)
{
    if (bt_high == true) {
        pt_port->BSRR = (GPIO_PIN_BIT << u4t_pin);
    } else {
        pt_port->BSRR = (GPIO_PIN_BIT << (u4t_pin + GPIO_BSRR_RESET_SHIFT));
    }
}

/*********************************************************************
 * @fn                - GPIO_ReadPin
 * @brief             - Read the input level of one pin
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 *
 * @return            - true = high, false = low
 *********************************************************************/
bool GPIO_ReadPin(const GPIO_TypeDef *pt_port, uint32_t u4t_pin)
{
    return ((pt_port->IDR & (GPIO_PIN_BIT << u4t_pin)) != 0U);
}

/*********************************************************************
 * @fn                - GPIO_ReadOutputPin
 * @brief             - Read back the output latch (ODR) of one pin
 *
 * @param[in]         - pt_port : GPIO port base address
 * @param[in]         - u4t_pin : pin number 0..15
 *
 * @return            - true = driven high, false = driven low
 *********************************************************************/
bool GPIO_ReadOutputPin(const GPIO_TypeDef *pt_port, uint32_t u4t_pin)
{
    return ((pt_port->ODR & (GPIO_PIN_BIT << u4t_pin)) != 0U);
}

/*********************************************************************
 * @fn                - SystemInit
 * @brief             - Called by startup code before main(); enables the FPU
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - The project has no system_stm32f4xx.c, startup declares
 *                      SystemInit as .weak without a fallback, so it must be
 *                      defined here. The core keeps running on HSI 16 MHz.
 *********************************************************************/
void SystemInit(void)
{
#if (defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)) && \
    (defined(__FPU_USED)    && (__FPU_USED    == 1U))
    SCB->CPACR |= FPU_CP10_CP11_FULL;
#endif
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
