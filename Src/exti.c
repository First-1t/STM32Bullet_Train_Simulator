/*******************************************************************************
 * File Name    : exti.c
 * Description  : External interrupt (EXTI) driver.
 *                - Exti_ConfigLine routes a GPIO pin to its EXTI line, sets
 *                  the trigger edge(s), unmasks it and enables the NVIC.
 *                - The EXTI interrupt handlers live here and forward each
 *                  line to the driver that owns it:
 *                    line 3  PB3  button 2      -> Button_ExtiCallback
 *                    line 4  PB4  button 4      -> Button_ExtiCallback
 *                    line 5  PB5  button 3      -> Button_ExtiCallback
 *                    line 8  PB8  RC522 IRQ pin -> RC522_ExtiCallback
 *                    line 10 PA10 button 1      -> Button_ExtiCallback
 * Date         : 2026-10-09
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "exti.h"

/* Private includes ----------------------------------------------------------*/
#include "button.h"
#include "rc522.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define EXTI_LINE_COUNT         (16U)    /* GPIO lines 0..15                 */
#define EXTI_LINE_BIT           (1U)
#define EXTICR_LINES_PER_REG    (4U)     /* 4 lines in each SYSCFG_EXTICRx  */
#define EXTICR_FIELD_BITS       (4U)
#define EXTICR_FIELD_MASK       (0xFU)
#define GPIO_PORT_STRIDE        (0x400U) /* GPIOB_BASE - GPIOA_BASE         */

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/* NVIC interrupt of each EXTI line (lines 5..9 and 10..15 share one) */
static const IRQn_Type etg_exti_irqn[EXTI_LINE_COUNT] = {
    EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
    EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
    EXTI15_10_IRQn
};

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - Exti_ConfigLine
 * @brief             - Connect a GPIO pin to EXTI line "pin", set the
 *                      trigger edge(s), unmask the line, enable the NVIC
 *
 * @param[in]         - pt_port : GPIOA, GPIOB, ...
 * @param[in]         - u4t_pin : pin number 0..15 (= EXTI line)
 * @param[in]         - u4t_trigger : EXTI_TRIGGER_RISING / _FALLING / _BOTH
 *
 * @return            - none
 *
 * @Note              - The pin must already be an input. Only lines 3, 4,
 *                      5..9 and 10..15 have a handler in this file.
 *********************************************************************/
void Exti_ConfigLine(const GPIO_TypeDef *pt_port, uint32_t u4t_pin, uint32_t u4t_trigger)
{
    uint32_t u4t_mask = EXTI_LINE_BIT << u4t_pin;
    uint32_t u4t_reg = u4t_pin / EXTICR_LINES_PER_REG;
    uint32_t u4t_shift = (u4t_pin % EXTICR_LINES_PER_REG) * EXTICR_FIELD_BITS;
    uint32_t u4t_port_code = ((uint32_t)pt_port - GPIOA_BASE) / GPIO_PORT_STRIDE;
    uint32_t u4t_primask;

    if (u4t_pin < EXTI_LINE_COUNT) {
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

        /* other EXTI ISRs change IMR: no interrupt inside these read-modify-writes */
        u4t_primask = __get_PRIMASK();
        __disable_irq();
        SYSCFG->EXTICR[u4t_reg] &= ~(EXTICR_FIELD_MASK << u4t_shift);
        SYSCFG->EXTICR[u4t_reg] |= (u4t_port_code << u4t_shift);
        if ((u4t_trigger & EXTI_TRIGGER_RISING) != 0U) {
            EXTI->RTSR |= u4t_mask;
        } else {
            EXTI->RTSR &= ~u4t_mask;
        }
        if ((u4t_trigger & EXTI_TRIGGER_FALLING) != 0U) {
            EXTI->FTSR |= u4t_mask;
        } else {
            EXTI->FTSR &= ~u4t_mask;
        }
        EXTI->PR = u4t_mask;                /* clear an old request */
        EXTI->IMR |= u4t_mask;
        __set_PRIMASK(u4t_primask);

        NVIC_EnableIRQ(etg_exti_irqn[u4t_pin]);
    } else {
        /* not a GPIO EXTI line */
    }
}

/*********************************************************************
 * @fn                - Exti_TakePending
 * @brief             - Check the pending flag of a line and clear it
 *
 * @param[in]         - u4t_line : EXTI line 0..15
 *
 * @return            - true = the line had an edge (flag now cleared)
 *********************************************************************/
bool Exti_TakePending(uint32_t u4t_line)
{
    uint32_t u4t_mask = EXTI_LINE_BIT << u4t_line;
    bool     bt_pending = false;

    if ((EXTI->PR & u4t_mask) != 0U) {
        EXTI->PR = u4t_mask;                /* write 1 to clear */
        bt_pending = true;
    } else {
        /* No action */
    }
    return bt_pending;
}

/*********************************************************************
 * @fn                - Exti_ClearPending
 * @brief             - Clear the pending flag of a line
 *
 * @param[in]         - u4t_line : EXTI line 0..15
 *
 * @return            - none
 *********************************************************************/
void Exti_ClearPending(uint32_t u4t_line)
{
    EXTI->PR = EXTI_LINE_BIT << u4t_line;
}

/*********************************************************************
 * @fn                - Exti_Mask
 * @brief             - Stop a line from making interrupts
 *
 * @param[in]         - u4t_line : EXTI line 0..15
 *
 * @return            - none
 *
 * @Note              - Called from interrupt handlers only
 *********************************************************************/
void Exti_Mask(uint32_t u4t_line)
{
    EXTI->IMR &= ~(EXTI_LINE_BIT << u4t_line);
}

/*********************************************************************
 * @fn                - Exti_Unmask
 * @brief             - Let a line make interrupts again
 *
 * @param[in]         - u4t_line : EXTI line 0..15
 *
 * @return            - none
 *
 * @Note              - Called from interrupt handlers only
 *********************************************************************/
void Exti_Unmask(uint32_t u4t_line)
{
    EXTI->IMR |= (EXTI_LINE_BIT << u4t_line);
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - EXTI3_IRQHandler
 * @brief             - EXTI line 3 : button 2 (PB3)
 *********************************************************************/
void EXTI3_IRQHandler(void)
{
    Button_ExtiCallback();
}

/*********************************************************************
 * @fn                - EXTI4_IRQHandler
 * @brief             - EXTI line 4 : button 4 (PB4)
 *********************************************************************/
void EXTI4_IRQHandler(void)
{
    Button_ExtiCallback();
}

/*********************************************************************
 * @fn                - EXTI9_5_IRQHandler
 * @brief             - EXTI lines 5..9 : button 3 (PB5), RC522 IRQ (PB8)
 *********************************************************************/
void EXTI9_5_IRQHandler(void)
{
    Button_ExtiCallback();
    RC522_ExtiCallback();
}

/*********************************************************************
 * @fn                - EXTI15_10_IRQHandler
 * @brief             - EXTI lines 10..15 : button 1 (PA10)
 *********************************************************************/
void EXTI15_10_IRQHandler(void)
{
    Button_ExtiCallback();
}

/* Private functions ---------------------------------------------------------*/
