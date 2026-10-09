/*******************************************************************************
 * File Name    : rc522.c
 * Description  : MFRC522 (RC522) RFID reader driver over bit-bang SPI mode 0.
 *                Reads the 4-byte UID of an ISO14443A card (WUPA + anticollision).
 *                Interrupt driven: the RC522 IRQ pin (PB8, EXTI8, falling edge)
 *                tells the MCU that the card answered (RxIRq) or that the
 *                RC522 timer ran out (TimerIRq = no card). While waiting the
 *                CPU sleeps, the status register is never polled.
 * Date         : 2026-10-09
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "rc522.h"

/* Private includes ----------------------------------------------------------*/
#include "timebase.h"
#include "exti.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/

/* Private struct ------------------------------------------------------------*/

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* MFRC522 registers */
#define REG_COMMAND             (0x01U)
#define REG_COM_IEN             (0x02U)
#define REG_COM_IRQ             (0x04U)
#define REG_ERROR               (0x06U)
#define REG_FIFO_DATA           (0x09U)
#define REG_FIFO_LEVEL          (0x0AU)
#define REG_CONTROL             (0x0CU)
#define REG_BIT_FRAMING         (0x0DU)
#define REG_MODE                (0x11U)
#define REG_TX_CONTROL          (0x14U)
#define REG_TX_ASK              (0x15U)
#define REG_T_MODE              (0x2AU)
#define REG_T_PRESCALER         (0x2BU)
#define REG_T_RELOAD_H          (0x2CU)
#define REG_T_RELOAD_L          (0x2DU)
#define REG_VERSION             (0x37U)

/* MFRC522 commands */
#define CMD_IDLE                (0x00U)
#define CMD_TRANSCEIVE          (0x0CU)
#define CMD_SOFT_RESET          (0x0FU)

/* Card (PICC) commands */
#define PICC_WUPA               (0x52U)  /* wake up cards in any state */
#define PICC_ANTICOLL_CL1       (0x93U)
#define PICC_ANTICOLL_NVB       (0x20U)
#define WUPA_FRAME_LEN          (1U)
#define ANTICOLL_FRAME_LEN      (2U)

/* Register values used by RC522_Init */
#define T_MODE_AUTO             (0x8DU)
#define T_PRESCALER_VALUE       (0x3EU)
#define T_RELOAD_H_VALUE        (0x00U)
#define T_RELOAD_L_VALUE        (0x1EU)
#define TX_ASK_100_PERCENT      (0x40U)
#define MODE_CRC_PRESET_6363    (0x3DU)
#define TX_ANTENNA_ON           (0x03U)

/* Bits used by transceive */
#define IRQ_ENABLE_RX_IDLE_TIMER (0xB1U) /* IRqInv (pin low = IRQ) + RxIEn + IdleIEn + TimerIEn */
#define COM_IRQ_SET1            (0x80U)
#define FIFO_FLUSH              (0x80U)
#define BIT_FRAMING_START_SEND  (0x80U)
#define BIT_FRAMING_7_BITS      (0x07U)  /* short frame for WUPA */
#define BIT_FRAMING_8_BITS      (0x00U)
#define IRQ_TIMER               (0x01U)
#define IRQ_RX_OR_IDLE          (0x30U)
#define ERROR_MASK              (0x1BU)  /* BufferOvfl, ColErr, ParityErr, ProtocolErr */
#define RX_LAST_BITS_MASK       (0x07U)
#define FIFO_MAX_BYTES          (16U)
#define TRANSCEIVE_TIMEOUT_MS   (30U)    /* RC522 timer gives up after 15 ms  */
#define ATQA_BITS               (16U)

/* SPI address byte */
#define ADDR_READ_FLAG          (0x80U)
#define ADDR_MASK               (0x7EU)
#define ADDR_SHIFT              (1U)

/* Bit-bang SPI */
#define BITS_PER_BYTE           (8U)
#define SPI_MSB_MASK            (0x80U)
#define SPI_LSB_MASK            (0x01U)
#define RESET_DELAY_MS          (50U)

/* VersionReg values that mean "no module answering" */
#define VERSION_BUS_LOW         (0x00U)
#define VERSION_BUS_HIGH        (0xFFU)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static volatile bool bg_irq_flag = false;   /* set by RC522_ExtiCallback            */
static bool          bg_irq_pin_ok = true;  /* false = RC522 IRQ seen, no EXTI edge */

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static uint8_t spi_transfer(uint8_t u1t_out);
static void    reg_write(uint8_t u1t_reg, uint8_t u1t_value);
static uint8_t reg_read(uint8_t u1t_reg);
static void    reg_set_bits(uint8_t u1t_reg, uint8_t u1t_mask);
static void    reg_clear_bits(uint8_t u1t_reg, uint8_t u1t_mask);
static bool    transceive(const uint8_t *pt_send, uint32_t u4t_send_len,
                          uint8_t *pt_back, uint32_t *pt_back_bits);

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - RC522_Init
 * @brief             - Configure the SPI pins, soft-reset the MFRC522,
 *                      set its timer and switch the antenna on
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - RST of the module is tied to 3V3; reset is done by
 *                      the SoftReset command. Can be called again at any time.
 *                      Also sets up the IRQ pin (PB8) on EXTI8, falling edge.
 *                      RC522 IRQ stays open-drain (reset value): the MCU
 *                      pull-up makes the high level, D15 is shared with I2C.
 *********************************************************************/
void RC522_Init(void)
{
    GPIO_SetMode(RC522_CS_PORT, RC522_CS_PIN, GPIO_MODE_OUTPUT);
    GPIO_SetMode(RC522_SCK_PORT, RC522_SCK_PIN, GPIO_MODE_OUTPUT);
    GPIO_SetMode(RC522_MOSI_PORT, RC522_MOSI_PIN, GPIO_MODE_OUTPUT);
    GPIO_SetMode(RC522_MISO_PORT, RC522_MISO_PIN, GPIO_MODE_INPUT);
    GPIO_SetPullUp(RC522_MISO_PORT, RC522_MISO_PIN);
    GPIO_SetMode(RC522_IRQ_PORT, RC522_IRQ_PIN, GPIO_MODE_INPUT);
    GPIO_SetPullUp(RC522_IRQ_PORT, RC522_IRQ_PIN);
    Exti_ConfigLine(RC522_IRQ_PORT, RC522_IRQ_PIN, EXTI_TRIGGER_FALLING);

    GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, true);
    GPIO_WritePin(RC522_SCK_PORT, RC522_SCK_PIN, false);
    GPIO_WritePin(RC522_MOSI_PORT, RC522_MOSI_PIN, false);
    Timebase_DelayMs(RESET_DELAY_MS);

    reg_write(REG_COMMAND, CMD_SOFT_RESET);
    Timebase_DelayMs(RESET_DELAY_MS);

    reg_write(REG_T_MODE, T_MODE_AUTO);
    reg_write(REG_T_PRESCALER, T_PRESCALER_VALUE);
    reg_write(REG_T_RELOAD_L, T_RELOAD_L_VALUE);
    reg_write(REG_T_RELOAD_H, T_RELOAD_H_VALUE);
    reg_write(REG_TX_ASK, TX_ASK_100_PERCENT);
    reg_write(REG_MODE, MODE_CRC_PRESET_6363);

    if ((reg_read(REG_TX_CONTROL) & TX_ANTENNA_ON) == 0U) {
        reg_set_bits(REG_TX_CONTROL, TX_ANTENNA_ON);
    } else {
        /* antenna already on */
    }
}

/*********************************************************************
 * @fn                - RC522_GetVersion
 * @brief             - Read VersionReg (0x91 / 0x92 for a genuine MFRC522)
 *
 * @param[in]         - none
 *
 * @return            - VersionReg value
 *********************************************************************/
uint8_t RC522_GetVersion(void)
{
    return reg_read(REG_VERSION);
}

/*********************************************************************
 * @fn                - RC522_IsAlive
 * @brief             - Check that the module answers on the SPI bus
 *
 * @param[in]         - none
 *
 * @return            - true = module answers, false = 0x00/0xFF (no module)
 *********************************************************************/
bool RC522_IsAlive(void)
{
    uint8_t u1t_version = reg_read(REG_VERSION);

    return ((u1t_version != VERSION_BUS_LOW) && (u1t_version != VERSION_BUS_HIGH));
}

/*********************************************************************
 * @fn                - RC522_ReadUid
 * @brief             - Try to read the UID of a card on the reader
 *
 * @param[out]        - pt_uid : buffer of RC522_UID_LEN bytes
 *
 * @return            - true = UID read and BCC correct, false = no card
 *********************************************************************/
bool RC522_ReadUid(uint8_t *pt_uid)
{
    uint8_t  u1t_buf[FIFO_MAX_BYTES];
    uint32_t u4t_bits = 0U;
    uint8_t  u1t_bcc = 0U;
    uint32_t u4t_i;
    bool     bt_ok = false;

    /* 1) WUPA (7-bit short frame) -> card answers ATQA (16 bits) */
    reg_write(REG_BIT_FRAMING, BIT_FRAMING_7_BITS);
    u1t_buf[0] = PICC_WUPA;
    if ((transceive(u1t_buf, WUPA_FRAME_LEN, u1t_buf, &u4t_bits) == true) &&
        (u4t_bits == ATQA_BITS)) {
        /* 2) anticollision cascade level 1 -> 4 UID bytes + BCC */
        reg_write(REG_BIT_FRAMING, BIT_FRAMING_8_BITS);
        u1t_buf[0] = PICC_ANTICOLL_CL1;
        u1t_buf[1] = PICC_ANTICOLL_NVB;
        if (transceive(u1t_buf, ANTICOLL_FRAME_LEN, u1t_buf, &u4t_bits) == true) {
            for (u4t_i = 0U; u4t_i < RC522_UID_LEN; u4t_i++) {
                pt_uid[u4t_i] = u1t_buf[u4t_i];
                u1t_bcc = (uint8_t)(u1t_bcc ^ u1t_buf[u4t_i]);
            }
            if (u1t_bcc == u1t_buf[RC522_UID_LEN]) {
                bt_ok = true;
            } else {
                /* BCC mismatch: treat as no card */
            }
        } else {
            /* no answer to anticollision */
        }
    } else {
        /* no card */
    }
    return bt_ok;
}

/*********************************************************************
 * @fn                - RC522_IsIrqPinOk
 * @brief             - Check the wiring of the RC522 IRQ pin
 *
 * @param[in]         - none
 *
 * @return            - false = the RC522 raised an interrupt but no EXTI
 *                      edge came (IRQ wire to D15 missing), true = OK
 *********************************************************************/
bool RC522_IsIrqPinOk(void)
{
    return bg_irq_pin_ok;
}

/* Callback functions --------------------------------------------------------*/
/*********************************************************************
 * @fn                - RC522_ExtiCallback
 * @brief             - Called by EXTI9_5_IRQHandler (exti.c): the RC522
 *                      IRQ pin went low (card answered or RC522 timeout)
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void RC522_ExtiCallback(void)
{
    if (Exti_TakePending(RC522_IRQ_PIN) == true) {
        bg_irq_flag = true;
    } else {
        /* No action */
    }
}

/* Private functions ---------------------------------------------------------*/

/*********************************************************************
 * @fn                - spi_transfer
 * @brief             - Exchange one byte, SPI mode 0 (CPOL = 0, CPHA = 0),
 *                      MSB first (clock about 500 kHz, set by the code speed)
 *
 * @param[in]         - u1t_out : byte to send on MOSI
 *
 * @return            - byte received on MISO
 *********************************************************************/
static uint8_t spi_transfer(uint8_t u1t_out)
{
    uint8_t  u1t_data = u1t_out;
    uint8_t  u1t_in = 0U;
    uint32_t u4t_bit;

    for (u4t_bit = 0U; u4t_bit < BITS_PER_BYTE; u4t_bit++) {
        GPIO_WritePin(RC522_MOSI_PORT, RC522_MOSI_PIN, ((u1t_data & SPI_MSB_MASK) != 0U));
        u1t_data = (uint8_t)(u1t_data << 1U);
        GPIO_WritePin(RC522_SCK_PORT, RC522_SCK_PIN, true);
        u1t_in = (uint8_t)(u1t_in << 1U);
        if (GPIO_ReadPin(RC522_MISO_PORT, RC522_MISO_PIN) == true) {
            u1t_in = (uint8_t)(u1t_in | SPI_LSB_MASK);
        } else {
            /* bit = 0 */
        }
        GPIO_WritePin(RC522_SCK_PORT, RC522_SCK_PIN, false);
    }
    return u1t_in;
}

/*********************************************************************
 * @fn                - reg_write
 * @brief             - Write one MFRC522 register
 *********************************************************************/
static void reg_write(uint8_t u1t_reg, uint8_t u1t_value)
{
    GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, false);
    (void)spi_transfer((uint8_t)((u1t_reg << ADDR_SHIFT) & ADDR_MASK));
    (void)spi_transfer(u1t_value);
    GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, true);
}

/*********************************************************************
 * @fn                - reg_read
 * @brief             - Read one MFRC522 register
 *********************************************************************/
static uint8_t reg_read(uint8_t u1t_reg)
{
    uint8_t u1t_value;

    GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, false);
    (void)spi_transfer((uint8_t)(ADDR_READ_FLAG | ((u1t_reg << ADDR_SHIFT) & ADDR_MASK)));
    u1t_value = spi_transfer(0U);
    GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, true);
    return u1t_value;
}

/*********************************************************************
 * @fn                - reg_set_bits
 * @brief             - Set bits of a register (read-modify-write)
 *********************************************************************/
static void reg_set_bits(uint8_t u1t_reg, uint8_t u1t_mask)
{
    reg_write(u1t_reg, (uint8_t)(reg_read(u1t_reg) | u1t_mask));
}

/*********************************************************************
 * @fn                - reg_clear_bits
 * @brief             - Clear bits of a register (read-modify-write)
 *********************************************************************/
static void reg_clear_bits(uint8_t u1t_reg, uint8_t u1t_mask)
{
    reg_write(u1t_reg, (uint8_t)(reg_read(u1t_reg) & (uint8_t)(~u1t_mask)));
}

/*********************************************************************
 * @fn                - transceive
 * @brief             - Send a frame to the card and read the answer
 *
 * @param[in]         - pt_send : bytes to send
 * @param[in]         - u4t_send_len : number of bytes to send
 * @param[out]        - pt_back : answer bytes (buffer of FIFO_MAX_BYTES)
 * @param[out]        - pt_back_bits : number of valid bits in the answer
 *
 * @return            - true = answer received without error
 *
 * @Note              - Waits with the CPU asleep until the RC522 IRQ pin
 *                      (EXTI8) fires; TRANSCEIVE_TIMEOUT_MS is only a
 *                      safety limit if the IRQ wire is missing
 *********************************************************************/
static bool transceive(const uint8_t *pt_send, uint32_t u4t_send_len,
                       uint8_t *pt_back, uint32_t *pt_back_bits)
{
    uint32_t u4t_start;
    bool     bt_done;
    bool     bt_ok = false;
    uint8_t  u1t_irq;
    uint32_t u4t_level;
    uint32_t u4t_last_bits;
    uint32_t u4t_i;

    reg_write(REG_COM_IEN, IRQ_ENABLE_RX_IDLE_TIMER);
    reg_clear_bits(REG_COM_IRQ, COM_IRQ_SET1);      /* IRQ pin back to high */
    Exti_ClearPending(RC522_IRQ_PIN);
    bg_irq_flag = false;
    reg_set_bits(REG_FIFO_LEVEL, FIFO_FLUSH);
    reg_write(REG_COMMAND, CMD_IDLE);

    for (u4t_i = 0U; u4t_i < u4t_send_len; u4t_i++) {
        reg_write(REG_FIFO_DATA, pt_send[u4t_i]);
    }
    reg_write(REG_COMMAND, CMD_TRANSCEIVE);
    reg_set_bits(REG_BIT_FRAMING, BIT_FRAMING_START_SEND);

    /* sleep until the IRQ pin interrupt: RX / idle, or RC522 timer (= no card) */
    u4t_start = Timebase_GetMs();
    while ((bg_irq_flag == false) && ((Timebase_GetMs() - u4t_start) < TRANSCEIVE_TIMEOUT_MS)) {
        Timebase_Sleep();
    }
    reg_clear_bits(REG_BIT_FRAMING, BIT_FRAMING_START_SEND);

    /* which event woke us (read once, after the interrupt) */
    u1t_irq = reg_read(REG_COM_IRQ);
    bt_done = (((u1t_irq & IRQ_TIMER) != 0U) || ((u1t_irq & IRQ_RX_OR_IDLE) != 0U));
    if (bg_irq_flag == true) {
        bg_irq_pin_ok = true;
    } else if (bt_done == true) {
        bg_irq_pin_ok = false;              /* RC522 raised IRQ but no EXTI edge */
    } else {
        /* module not answering at all */
    }

    if ((bt_done == true) && ((reg_read(REG_ERROR) & ERROR_MASK) == 0U)) {
        u4t_level = (uint32_t)reg_read(REG_FIFO_LEVEL);
        u4t_last_bits = (uint32_t)reg_read(REG_CONTROL) & RX_LAST_BITS_MASK;
        if ((u4t_last_bits != 0U) && (u4t_level > 0U)) {
            *pt_back_bits = ((u4t_level - 1U) * BITS_PER_BYTE) + u4t_last_bits;
        } else {
            *pt_back_bits = u4t_level * BITS_PER_BYTE;
        }

        if (u4t_level == 0U) {
            u4t_level = 1U;
        } else if (u4t_level > FIFO_MAX_BYTES) {
            u4t_level = FIFO_MAX_BYTES;
        } else {
            /* level in range */
        }
        for (u4t_i = 0U; u4t_i < u4t_level; u4t_i++) {
            pt_back[u4t_i] = reg_read(REG_FIFO_DATA);
        }
        bt_ok = true;
    } else {
        /* timeout or error */
    }
    return bt_ok;
}
