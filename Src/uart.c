/*******************************************************************************
 * File Name    : uart.c
 * Description  : USART2 driver (PA2 TX / PA3 RX, 115200 8N1), polling mode
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "uart.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define UART_BRR_115200         (139U)   /* 16 MHz / 115200 = 138.9 */
#define UART_DATA_MASK          (0xFFU)
#define DEC_BASE                (10U)
#define UINT32_MAX_DIGITS       (10U)
#define HEX_NIBBLE_BITS         (4U)
#define HEX_NIBBLE_MASK         (0x0FU)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const char ctg_hex_digits[] = "0123456789ABCDEF";

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - UART_Init
 * @brief             - Configure PA2/PA3 as AF7 and USART2 as 115200 8N1
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - USART2 is connected to the ST-LINK virtual COM port
 *********************************************************************/
void UART_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    GPIO_SetMode(UART_TX_PORT, UART_TX_PIN, GPIO_MODE_AF);
    GPIO_SetMode(UART_RX_PORT, UART_RX_PIN, GPIO_MODE_AF);
    GPIO_SetAltFunc(UART_TX_PORT, UART_TX_PIN, UART_GPIO_AF);
    GPIO_SetAltFunc(UART_RX_PORT, UART_RX_PIN, UART_GPIO_AF);

    USART2->CR1 |= USART_CR1_UE;
    USART2->CR1 &= ~USART_CR1_M;           /* 8 data bits */
    USART2->CR2 &= ~USART_CR2_STOP;        /* 1 stop bit  */
    USART2->BRR = UART_BRR_115200;
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
}

/*********************************************************************
 * @fn                - UART_SendChar
 * @brief             - Send one byte (waits until TX register is empty)
 *
 * @param[in]         - ct_ch : byte to send
 *
 * @return            - none
 *********************************************************************/
void UART_SendChar(char ct_ch)
{
    while ((USART2->SR & USART_SR_TXE) == 0U) {
        /* wait */
    }
    USART2->DR = (uint32_t)((uint8_t)ct_ch);
}

/*********************************************************************
 * @fn                - UART_SendString
 * @brief             - Send a null-terminated string (UTF-8 is sent as bytes)
 *
 * @param[in]         - pt_str : string to send
 *
 * @return            - none
 *********************************************************************/
void UART_SendString(const char *pt_str)
{
    const char *pt_ch = pt_str;

    while (*pt_ch != '\0') {
        UART_SendChar(*pt_ch);
        pt_ch++;
    }
}

/*********************************************************************
 * @fn                - UART_SendUint
 * @brief             - Send an unsigned number in decimal
 *
 * @param[in]         - u4t_value : number to send
 *
 * @return            - none
 *********************************************************************/
void UART_SendUint(uint32_t u4t_value)
{
    char     ct_buf[UINT32_MAX_DIGITS];
    uint32_t u4t_len = 0U;
    uint32_t u4t_rest = u4t_value;

    do {
        ct_buf[u4t_len] = (char)((uint32_t)'0' + (u4t_rest % DEC_BASE));
        u4t_len++;
        u4t_rest = u4t_rest / DEC_BASE;
    } while (u4t_rest != 0U);

    while (u4t_len > 0U) {
        u4t_len--;
        UART_SendChar(ct_buf[u4t_len]);
    }
}

/*********************************************************************
 * @fn                - UART_SendHex8
 * @brief             - Send one byte as 2 hex digits
 *
 * @param[in]         - u1t_value : byte to send
 *
 * @return            - none
 *********************************************************************/
void UART_SendHex8(uint8_t u1t_value)
{
    UART_SendChar(ctg_hex_digits[((uint32_t)u1t_value >> HEX_NIBBLE_BITS) & HEX_NIBBLE_MASK]);
    UART_SendChar(ctg_hex_digits[(uint32_t)u1t_value & HEX_NIBBLE_MASK]);
}

/*********************************************************************
 * @fn                - UART_ReadChar
 * @brief             - Read one received byte if available (non-blocking)
 *
 * @param[out]        - pt_ch : received byte
 *
 * @return            - true = a byte was received, false = nothing received
 *********************************************************************/
bool UART_ReadChar(char *pt_ch)
{
    bool bt_received = false;

    if ((USART2->SR & USART_SR_RXNE) != 0U) {
        *pt_ch = (char)(USART2->DR & UART_DATA_MASK);
        bt_received = true;
    } else {
        /* No action */
    }
    return bt_received;
}

/*********************************************************************
 * @fn                - UART_FlushRx
 * @brief             - Drop any byte that is waiting in the receiver
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void UART_FlushRx(void)
{
    char ct_dummy = '\0';

    while (UART_ReadChar(&ct_dummy) == true) {
        /* discard */
    }
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
