/*******************************************************************************
 * File Name    : uart.c
 * Description  : USART2 driver (PA2 TX / PA3 RX, 115200 8N1), interrupt driven.
 *                TX : bytes go into a ring buffer, the TXE interrupt sends them.
 *                RX : the RXNE interrupt stores received bytes in a ring buffer.
 *                The CPU never waits on a UART status flag (no polling).
 * Date         : 2026-10-09
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
#define UART_TX_BUF_SIZE        (1024U)  /* must be a power of 2 */
#define UART_TX_BUF_MASK        (UART_TX_BUF_SIZE - 1U)
#define UART_RX_BUF_SIZE        (64U)    /* must be a power of 2 */
#define UART_RX_BUF_MASK        (UART_RX_BUF_SIZE - 1U)
#define DEC_BASE                (10U)
#define UINT32_MAX_DIGITS       (10U)
#define HEX_NIBBLE_BITS         (4U)
#define HEX_NIBBLE_MASK         (0x0FU)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const char ctg_hex_digits[] = "0123456789ABCDEF";

/* Private variables ---------------------------------------------------------*/
/* TX ring buffer : head written by the main program, tail written by the ISR */
static volatile uint8_t  u1g_tx_buf[UART_TX_BUF_SIZE];
static volatile uint32_t u4g_tx_head = 0U;
static volatile uint32_t u4g_tx_tail = 0U;
/* RX ring buffer : head written by the ISR, tail written by the main program */
static volatile uint8_t  u1g_rx_buf[UART_RX_BUF_SIZE];
static volatile uint32_t u4g_rx_head = 0U;
static volatile uint32_t u4g_rx_tail = 0U;

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - UART_Init
 * @brief             - Configure PA2/PA3 as AF7, USART2 as 115200 8N1 and
 *                      enable the USART2 interrupt (RXNE now, TXE on demand)
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
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);

    NVIC_EnableIRQ(USART2_IRQn);
}

/*********************************************************************
 * @fn                - UART_SendChar
 * @brief             - Queue one byte for sending (sent by the TXE interrupt)
 *
 * @param[in]         - ct_ch : byte to send
 *
 * @return            - none
 *
 * @Note              - Only waits (CPU asleep) when the 1 KB TX buffer is full
 *********************************************************************/
void UART_SendChar(char ct_ch)
{
    uint32_t u4t_next = (u4g_tx_head + 1U) & UART_TX_BUF_MASK;

    while (u4t_next == u4g_tx_tail) {
        __WFI();                            /* buffer full: sleep until the TXE interrupt sends a byte */
    }
    u1g_tx_buf[u4g_tx_head] = (uint8_t)ct_ch;
    u4g_tx_head = u4t_next;
    USART2->CR1 |= USART_CR1_TXEIE;        /* start / keep the TX interrupt running */
}

/*********************************************************************
 * @fn                - UART_SendString
 * @brief             - Queue a null-terminated string (UTF-8 is sent as bytes)
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
 * @brief             - Queue an unsigned number in decimal
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
 * @brief             - Queue one byte as 2 hex digits
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
 * @brief             - Take one received byte from the RX buffer (non-blocking)
 *
 * @param[out]        - pt_ch : received byte
 *
 * @return            - true = a byte was available, false = buffer empty
 *********************************************************************/
bool UART_ReadChar(char *pt_ch)
{
    bool bt_received = false;

    if (u4g_rx_tail != u4g_rx_head) {
        *pt_ch = (char)u1g_rx_buf[u4g_rx_tail];
        u4g_rx_tail = (u4g_rx_tail + 1U) & UART_RX_BUF_MASK;
        bt_received = true;
    } else {
        /* nothing received */
    }
    return bt_received;
}

/*********************************************************************
 * @fn                - UART_FlushRx
 * @brief             - Drop every byte waiting in the RX buffer
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void UART_FlushRx(void)
{
    u4g_rx_tail = u4g_rx_head;
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - USART2_IRQHandler
 * @brief             - USART2 interrupt: store a received byte (RXNE) and
 *                      send the next queued byte (TXE)
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - TXE interrupt is switched off when the TX buffer is empty
 *********************************************************************/
void USART2_IRQHandler(void)
{
    uint32_t u4t_status = USART2->SR;
    uint32_t u4t_next;
    uint8_t  u1t_data;

    if ((u4t_status & USART_SR_RXNE) != 0U) {
        u1t_data = (uint8_t)(USART2->DR & UART_DATA_MASK);   /* also clears RXNE / ORE */
        u4t_next = (u4g_rx_head + 1U) & UART_RX_BUF_MASK;
        if (u4t_next != u4g_rx_tail) {
            u1g_rx_buf[u4g_rx_head] = u1t_data;
            u4g_rx_head = u4t_next;
        } else {
            /* RX buffer full: byte dropped */
        }
    } else {
        /* No action */
    }

    if (((USART2->CR1 & USART_CR1_TXEIE) != 0U) && ((u4t_status & USART_SR_TXE) != 0U)) {
        if (u4g_tx_tail != u4g_tx_head) {
            USART2->DR = (uint32_t)u1g_tx_buf[u4g_tx_tail];
            u4g_tx_tail = (u4g_tx_tail + 1U) & UART_TX_BUF_MASK;
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;   /* nothing left to send */
        }
    } else {
        /* No action */
    }
}

/* Private functions ---------------------------------------------------------*/
