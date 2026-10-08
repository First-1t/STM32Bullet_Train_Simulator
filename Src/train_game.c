/*******************************************************************************
 * File Name    : train_game.c
 * Description  : Bullet Train Simulator game logic (state machine).
 *
 *   [START]  tap RFID card -> 4 LEDs blink -> mission briefing -> 'g'
 *   Each leg : speed from throttle (pot), 200 m before the target the blue LED
 *              lights and the driver is asked (train keeps moving).
 *   1) Passenger station : STOP in time = stop -> door minigame (4 cars) -> 'g'
 *   2) Charging station  : STOP in time = stop -> press button 4 x50     -> 'g'
 *   3) Junction          : press button 4 to switch to track 2 (Khon Kaen)
 *                          before the junction, otherwise -> lose
 *   4) Khon Kaen station : STOP in time and before 12:02:30 = win
 *                          last 600 m = community zone, limit 120 km/h,
 *                          speed camera starts 5 s after entering the zone
 *   Deadman  : 7-segment counts 9 -> 0; moving the throttle or a new button
 *              press restarts it; reaching 0 = driver asleep -> alarm
 *   Emergency: button 2 while driving -> alarm, 'r' -> continue from same spot
 * Date         : 2026-10-08
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "train_game.h"

/* Private includes ----------------------------------------------------------*/
#include "timebase.h"
#include "uart.h"
#include "adc.h"
#include "led.h"
#include "button.h"
#include "seg7.h"
#include "buzzer.h"
#include "rc522.h"
#include "headlight.h"

/* Private typedef -----------------------------------------------------------*/

/* Private enum --------------------------------------------------------------*/
typedef enum {
    POLL_TIMEOUT = 0,            /* 1 second passed                     */
    POLL_EMERGENCY               /* emergency button pressed            */
} poll_result_t;

typedef enum {
    LEG_STATION = 0,             /* ask "stop?" near the target         */
    LEG_JUNCTION                 /* button 4 switches the track         */
} leg_type_t;

typedef enum {
    DOOR_CLOSED = 0,             /* not opened yet                      */
    DOOR_LOADING,                /* open, passengers boarding           */
    DOOR_READY,                  /* boarding done, waiting to close     */
    DOOR_DONE                    /* closed                              */
} door_state_t;

/* Private struct ------------------------------------------------------------*/
/* Button events during one poll period (only new presses are counted) */
typedef struct {
    bool     b_stop_held;        /* STOP was down at some time          */
    uint32_t u4_stop_presses;
    uint32_t u4_track_presses;
    uint32_t u4_dead_presses;
} btn_event_t;

/* Private union -------------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Button roles */
#define BTN_DEADMAN             (BUTTON_1)
#define BTN_EMERGENCY           (BUTTON_2)
#define BTN_STOP                (BUTTON_3)
#define BTN_TRACK               (BUTTON_4)

/* Driving */
#define SEG_DISTANCE_M          (1500)   /* distance of each leg (m)            */
#define APPROACH_M              (200)    /* ask / warn this far before target   */
#define SPEED_MIN_KMH           (60)
#define SPEED_MAX_KMH           (300)
#define M_PER_KM                (1000)
#define SEC_PER_HOUR_S          (3600)
#define THROTTLE_ADC_CH         (ADC_CH_POT_EXT)  /* ADC_CH_POT_BOARD = pot on shield */
#define THROTTLE_ACTIVE_DELTA   (60)     /* throttle change = driver awake      */

/* Deadman */
#define DEADMAN_ENABLED         (1)      /* 1 = on, 0 = off                     */
#define DEADMAN_COUNT_START     (9)

/* Community zone before Khon Kaen */
#define ZONE_M                  (600)
#define ZONE_LIMIT_KMH          (120)
#define ZONE_GRACE_MS           (5000U)
#define NO_ZONE_M               (0)

/* Tracks at the junction */
#define TRACK_UDON              (1U)
#define TRACK_KHONKAEN          (2U)
#define TRACK_NAME_COUNT        (3U)
#define TOGGLE_DIVISOR          (2U)

/* Station minigames */
#define CAR_COUNT               (4U)
#define DOOR_LOAD_MS            (5000U)
#define CHARGE_PRESSES          (50U)
#define CHARGE_REPORT_EVERY     (10U)
#define GAUGE_LEVELS            (4U)

/* Clock (trip clock starts at 12:00:00 when 'g' is pressed) */
#define TIME_SCALE              (1U)     /* 1 = real time, 60 = 1 s -> 1 min    */
#define SEC_PER_MIN             (60U)
#define MIN_PER_HOUR            (60U)
#define SEC_PER_HOUR            (3600U)
#define HOURS_PER_DAY           (24U)
#define CLOCK_START_S           (12U * SEC_PER_HOUR)
#define DEADLINE_AFTER_START_S  (150U)   /* 2 min 30 s -> must arrive by 12:02:30 */
#define ARRIVE_DEADLINE_S       (CLOCK_START_S + DEADLINE_AFTER_START_S)
#define TWO_DIGIT_LIMIT         (10U)

/* Timing (ms) */
#define POLL_PERIOD_MS          (1000U)
#define BUTTON_SAMPLE_MS        (10U)
#define BLINK_HALF_PERIOD_MS    (250U)
#define ALARM_GAP_MS            (250U)
#define RFID_CHECK_MS           (2000U)
#define START_BLINK_TIMES       (5U)
#define START_BLINK_HALF_MS     (100U)

/* Buzzer lengths (cycles) */
#define BEEP_CARD_OK            (300U)
#define BEEP_ARRIVE             (250U)
#define BEEP_DONE               (200U)
#define BEEP_ALARM              (120U)
#define BEEP_STOP_ACK           (100U)
#define BEEP_SHORT              (80U)
#define BEEP_DOOR               (60U)

/* Serial keys */
#define KEY_GO_LOWER            ('g')
#define KEY_GO_UPPER            ('G')
#define KEY_RESET_LOWER         ('r')
#define KEY_RESET_UPPER         ('R')

/* Leg result for stations */
#define LEG_RESULT_PASS         (0U)
#define LEG_RESULT_STOP         (1U)

#define SEG7_ZERO               (0U)

/* Private macro -------------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
static const char *const ptg_track_name[TRACK_NAME_COUNT] = {
    "", "อุดรธานี", "ขอนแก่น"
};

static const char *const ptg_charge_level_name[GAUGE_LEVELS + 1U] = {
    "", "เขียว", "+เหลือง", "+แดง", "+น้ำเงิน (เต็ม)"
};

/* car i uses button i and LED i (LED order = gauge order) */
static const button_id_t etg_car_button[CAR_COUNT] = {
    BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4
};
static const led_id_t etg_car_led[CAR_COUNT] = {
    LED_GREEN, LED_YELLOW, LED_RED, LED_BLUE
};

/* Private variables ---------------------------------------------------------*/
static uint32_t u4g_train_no = 0U;         /* train number, +1 every trip     */
static uint32_t u4g_trip_ms0 = 0U;         /* ms when the trip clock started  */
static bool     bg_trip_on = false;        /* true = trip clock is running    */
static bool     bg_zone_entered = false;   /* entered the community zone      */
static uint32_t u4g_zone_violation = 0U;   /* speed camera hits               */
static int32_t  s4g_zone_max_kmh = 0;      /* highest speed caught by camera  */
static bool     bg_prev_stop = false;      /* button states of the last sample */
static bool     bg_prev_track = false;
static bool     bg_prev_dead = false;

/* External variables --------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static bool          rising_edge(bool bt_now, bool *pt_prev);
static bool          key_received(char ct_lower, char ct_upper);
static void          wait_for_key(char ct_lower, char ct_upper, bool bt_update_headlight);
static uint32_t      trip_clock_s(void);
static void          send_2digits(uint32_t u4t_value);
static void          send_clock(uint32_t u4t_clock_s);
static void          send_time_tag(void);
static void          send_headlight(void);
static poll_result_t poll_buttons_1s(btn_event_t *pt_event);
static void          gauge_show(uint32_t u4t_level);
static void          run_alarm(const char *pt_message);
static void          resume_driving(bool bt_asking);
static void          print_status(const char *pt_name, bool bt_junction, int32_t s4t_dist,
                                  int32_t s4t_speed, uint16_t u2t_pot, uint32_t u4t_track,
                                  bool bt_in_zone);
static uint32_t      drive_leg(const char *pt_name, leg_type_t et_type, int32_t s4t_zone_m);
static bool          drive_to_station(const char *pt_name, int32_t s4t_zone_m);
static uint32_t      drive_to_junction(const char *pt_name);
static void          wait_depart(void);
static void          passenger_minigame(void);
static void          charge_minigame(void);
static void          reset_trip(void);
static void          wait_for_card(uint8_t *pt_uid);
static void          print_mission(void);
static void          start_trip(void);
static void          print_summary(bool bt_win, const char *pt_reason, uint32_t u4t_end_s);

/* Private user code ---------------------------------------------------------*/

/* Public functions ----------------------------------------------------------*/
/*********************************************************************
 * @fn                - TrainGame_Init
 * @brief             - Initialise every peripheral used by the game and
 *                      print the RC522 version (wiring check)
 *
 * @param[in]         - none
 *
 * @return            - none
 *********************************************************************/
void TrainGame_Init(void)
{
    Board_Init();
    Timebase_Init();
    UART_Init();
    ADC_Init();
    LED_Init();
    Button_Init();
    Seg7_Init();
    Buzzer_Init();
    RC522_Init();

    UART_SendString("\r\nRC522 VersionReg = 0x");
    UART_SendHex8(RC522_GetVersion());
    UART_SendString("\r\n");
}

/*********************************************************************
 * @fn                - TrainGame_RunJourney
 * @brief             - Play one complete trip: START -> 4 legs -> summary
 *
 * @param[in]         - none
 *
 * @return            - none
 *
 * @Note              - Call in the main loop; returns when the trip ends
 *********************************************************************/
void TrainGame_RunJourney(void)
{
    uint8_t     u1t_uid[RC522_UID_LEN];
    uint32_t    u4t_i;
    uint32_t    u4t_end_s;
    bool        bt_win = false;
    const char *pt_reason = "";

    /* --- START --- */
    reset_trip();
    wait_for_card(u1t_uid);
    UART_SendString("  RFID ผ่าน! UID: ");
    for (u4t_i = 0U; u4t_i < RC522_UID_LEN; u4t_i++) {
        UART_SendHex8(u1t_uid[u4t_i]);
        UART_SendChar(' ');
    }
    UART_SendString("\r\n");
    Buzzer_Beep(BEEP_CARD_OK);
    LED_BlinkAll(START_BLINK_TIMES, START_BLINK_HALF_MS);
    print_mission();
    start_trip();

    /* --- 1) passenger station --- */
    if (drive_to_station("สถานีรับผู้โดยสาร", NO_ZONE_M) == true) {
        passenger_minigame();
    } else {
        /* passed through */
    }

    /* --- 2) charging station --- */
    if (drive_to_station("สถานีชาร์จไฟ", NO_ZONE_M) == true) {
        charge_minigame();
    } else {
        /* passed through */
    }

    /* --- 3) junction: must be on track 2 (Khon Kaen) --- */
    if (drive_to_junction("จุดแยกทาง") != TRACK_KHONKAEN) {
        u4t_end_s = trip_clock_s();
        run_alarm("WRONG ROUTE! ไม่ได้สับราง วิ่งไปทางอุดรธานี เลยเป้าหมาย -> แพ้");
        pt_reason = "ไปผิดเส้นทาง";
    } else {
        send_time_tag();
        UART_SendString("[JUNCTION] เข้าเส้นทาง 2 มุ่งหน้าขอนแก่น\r\n");

        /* --- 4) Khon Kaen: stop in time and before the deadline --- */
        if (drive_to_station("สถานีขอนแก่น", ZONE_M) == false) {
            u4t_end_s = trip_clock_s();
            Seg7_Show(SEG7_ZERO);
            run_alarm("CRASH! จอดไม่ทัน ชนปลายทาง -> แพ้");
            pt_reason = "ชนปลายทาง";
        } else {
            u4t_end_s = trip_clock_s();
            if (u4t_end_s > ARRIVE_DEADLINE_S) {
                Seg7_Show(SEG7_ZERO);
                run_alarm("LATE! ถึงขอนแก่นช้ากว่าเวลากำหนด -> แพ้");
                pt_reason = "มาสาย";
            } else {
                bt_win = true;
                send_time_tag();
                UART_SendString("จอดสถานีขอนแก่นสำเร็จ ทันเวลา -> ดับเครื่อง\r\n");
                LED_SetAll(false);
                Seg7_Show(SEG7_ZERO);
            }
        }
    }

    print_summary(bt_win, pt_reason, u4t_end_s);
}

/* Callback functions --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*********************************************************************
 * @fn                - rising_edge
 * @brief             - Detect a new press (released -> pressed)
 *
 * @param[in]         - bt_now : current button state
 * @param[in,out]     - pt_prev : state of the last sample (updated)
 *
 * @return            - true = new press
 *********************************************************************/
static bool rising_edge(bool bt_now, bool *pt_prev)
{
    bool bt_edge = false;

    if ((bt_now == true) && (*pt_prev == false)) {
        bt_edge = true;
    } else {
        /* No action */
    }
    *pt_prev = bt_now;
    return bt_edge;
}

/*********************************************************************
 * @fn                - key_received
 * @brief             - Non-blocking check for a key on the serial port
 *
 * @param[in]         - ct_lower / ct_upper : accepted characters
 *
 * @return            - true = the key was received
 *********************************************************************/
static bool key_received(char ct_lower, char ct_upper)
{
    char ct_rx = '\0';
    bool bt_match = false;

    if (UART_ReadChar(&ct_rx) == true) {
        if ((ct_rx == ct_lower) || (ct_rx == ct_upper)) {
            bt_match = true;
        } else {
            /* other key: ignore */
        }
    } else {
        /* nothing received */
    }
    return bt_match;
}

/*********************************************************************
 * @fn                - wait_for_key
 * @brief             - Block until the key is typed on the serial port
 *
 * @param[in]         - ct_lower / ct_upper : accepted characters
 * @param[in]         - bt_update_headlight : true = keep the headlight working
 *********************************************************************/
static void wait_for_key(char ct_lower, char ct_upper, bool bt_update_headlight)
{
    bool bt_done = false;

    while (bt_done == false) {
        if (bt_update_headlight == true) {
            Headlight_Update();
        } else {
            /* LEDs are kept as they are */
        }
        bt_done = key_received(ct_lower, ct_upper);
    }
}

/*********************************************************************
 * @fn                - trip_clock_s
 * @brief             - Trip clock in seconds of the day (12:00:00 at 'g')
 *********************************************************************/
static uint32_t trip_clock_s(void)
{
    uint32_t u4t_elapsed_s = 0U;

    if (bg_trip_on == true) {
        u4t_elapsed_s = ((Timebase_GetMs() - u4g_trip_ms0) / MS_PER_SECOND) * TIME_SCALE;
    } else {
        /* clock not started: stays at 12:00:00 */
    }
    return (CLOCK_START_S + u4t_elapsed_s);
}

/*********************************************************************
 * @fn                - send_2digits
 * @brief             - Send a number 0..99 with a leading zero
 *********************************************************************/
static void send_2digits(uint32_t u4t_value)
{
    if (u4t_value < TWO_DIGIT_LIMIT) {
        UART_SendChar('0');
    } else {
        /* No action */
    }
    UART_SendUint(u4t_value);
}

/*********************************************************************
 * @fn                - send_clock
 * @brief             - Send a time of day as HH:MM:SS
 *********************************************************************/
static void send_clock(uint32_t u4t_clock_s)
{
    send_2digits((u4t_clock_s / SEC_PER_HOUR) % HOURS_PER_DAY);
    UART_SendChar(':');
    send_2digits((u4t_clock_s / SEC_PER_MIN) % MIN_PER_HOUR);
    UART_SendChar(':');
    send_2digits(u4t_clock_s % SEC_PER_MIN);
}

/*********************************************************************
 * @fn                - send_time_tag
 * @brief             - Send "[HH:MM:SS] " at the start of a log line
 *********************************************************************/
static void send_time_tag(void)
{
    UART_SendChar('[');
    send_clock(trip_clock_s());
    UART_SendString("] ");
}

/*********************************************************************
 * @fn                - send_headlight
 * @brief             - Send the headlight (yellow LED) state
 *********************************************************************/
static void send_headlight(void)
{
    if (Headlight_IsOn() == true) {
        UART_SendString("ไฟหน้า: เปิด");
    } else {
        UART_SendString("ไฟหน้า: ปิด");
    }
}

/*********************************************************************
 * @fn                - poll_buttons_1s
 * @brief             - Keep the headlight working and sample the buttons
 *                      for 1 second
 *
 * @param[out]        - pt_event : button events (only new presses counted)
 *
 * @return            - POLL_EMERGENCY at once if the emergency button is
 *                      pressed, otherwise POLL_TIMEOUT after 1 second
 *
 * @Note              - Holding a button does not block the game
 *********************************************************************/
static poll_result_t poll_buttons_1s(btn_event_t *pt_event)
{
    poll_result_t et_result = POLL_TIMEOUT;
    uint32_t      u4t_start = Timebase_GetMs();
    bool          bt_done = false;
    bool          bt_stop;

    while (bt_done == false) {
        Headlight_Update();
        if (Button_IsPressed(BTN_EMERGENCY) == true) {
            et_result = POLL_EMERGENCY;
            bt_done = true;
        } else if ((Timebase_GetMs() - u4t_start) >= POLL_PERIOD_MS) {
            bt_done = true;
        } else {
            bt_stop = Button_IsPressed(BTN_STOP);
            if (bt_stop == true) {
                pt_event->b_stop_held = true;
            } else {
                /* No action */
            }
            if (rising_edge(bt_stop, &bg_prev_stop) == true) {
                pt_event->u4_stop_presses++;
            } else {
                /* No action */
            }
            if (rising_edge(Button_IsPressed(BTN_TRACK), &bg_prev_track) == true) {
                pt_event->u4_track_presses++;
            } else {
                /* No action */
            }
            if (rising_edge(Button_IsPressed(BTN_DEADMAN), &bg_prev_dead) == true) {
                pt_event->u4_dead_presses++;
            } else {
                /* No action */
            }
            Timebase_DelayMs(BUTTON_SAMPLE_MS);
        }
    }
    return et_result;
}

/*********************************************************************
 * @fn                - gauge_show
 * @brief             - 4-step LED gauge: 1 = green, 2 = +yellow,
 *                      3 = +red, 4 = +blue (full)
 *********************************************************************/
static void gauge_show(uint32_t u4t_level)
{
    uint32_t u4t_i;

    for (u4t_i = 0U; u4t_i < GAUGE_LEVELS; u4t_i++) {
        LED_Set(etg_car_led[u4t_i], (u4t_level > u4t_i));
    }
}

/*********************************************************************
 * @fn                - run_alarm
 * @brief             - Red LED + repeated beeps until 'r' is typed
 *
 * @param[in]         - pt_message : reason shown on the serial terminal
 *********************************************************************/
static void run_alarm(const char *pt_message)
{
    bool bt_reset = false;

    LED_Set(LED_RED, true);
    LED_Set(LED_GREEN, false);
    LED_Set(LED_BLUE, false);
    send_time_tag();
    UART_SendString("*** ALARM: ");
    UART_SendString(pt_message);
    UART_SendString(" -> ไฟแดงติด! พิมพ์ 'r' เพื่อรีเซ็ต ***\r\n");

    while (bt_reset == false) {
        Headlight_Update();
        Buzzer_Beep(BEEP_ALARM);
        Timebase_DelayMs(ALARM_GAP_MS);
        bt_reset = key_received(KEY_RESET_LOWER, KEY_RESET_UPPER);
    }
    LED_Set(LED_RED, false);
}

/*********************************************************************
 * @fn                - resume_driving
 * @brief             - Restore the driving LEDs after an alarm
 *
 * @param[in]         - bt_asking : true = the blue "near" LED was on
 *********************************************************************/
static void resume_driving(bool bt_asking)
{
    LED_Set(LED_GREEN, true);
    LED_Set(LED_BLUE, bt_asking);
    UART_SendString("  ขับต่อจากจุดเดิม...\r\n");
}

/*********************************************************************
 * @fn                - print_status
 * @brief             - Print the 1-second driving status line
 *********************************************************************/
static void print_status(const char *pt_name, bool bt_junction, int32_t s4t_dist,
                         int32_t s4t_speed, uint16_t u2t_pot, uint32_t u4t_track,
                         bool bt_in_zone)
{
    send_time_tag();
    if (bt_junction == true) {
        UART_SendString("ระยะถึงจุดแยก: ");
    } else {
        UART_SendString("ระยะถึงสถานี: ");
    }
    UART_SendUint((uint32_t)s4t_dist);
    UART_SendString(" m (");
    UART_SendString(pt_name);
    UART_SendString(") | ความเร็ว: ");
    UART_SendUint((uint32_t)s4t_speed);
    UART_SendString(" km/h (คันเร่ง ");
    UART_SendUint((uint32_t)u2t_pot);
    UART_SendString(")");
    if (bt_junction == true) {
        UART_SendString(" | เส้นทาง ");
        UART_SendUint(u4t_track);
    } else {
        /* No action */
    }
    if (bt_in_zone == true) {
        UART_SendString(" | เขตชุมชน <= ");
        UART_SendUint((uint32_t)ZONE_LIMIT_KMH);
    } else {
        /* No action */
    }
    UART_SendString(" | ");
    send_headlight();
    UART_SendString("\r\n");
}

/*********************************************************************
 * @fn                - drive_leg
 * @brief             - Drive one leg of SEG_DISTANCE_M to pt_name
 *
 * @param[in]         - pt_name : name of the target (shown in the log)
 * @param[in]         - et_type : LEG_STATION or LEG_JUNCTION
 * @param[in]         - s4t_zone_m : last metres that are a community zone
 *                                   (NO_ZONE_M = none)
 *
 * @return            - LEG_STATION  : LEG_RESULT_STOP / LEG_RESULT_PASS
 *                      LEG_JUNCTION : selected track when arriving
 *
 * @Note              - Emergency / deadman alarm resumes from the same spot
 *********************************************************************/
static uint32_t drive_leg(const char *pt_name, leg_type_t et_type, int32_t s4t_zone_m)
{
    bool        bt_junction = (et_type == LEG_JUNCTION);
    int32_t     s4t_dist = SEG_DISTANCE_M;
    int32_t     s4t_count = DEADMAN_COUNT_START;
    int32_t     s4t_speed;
    int32_t     s4t_dpot;
    bool        bt_asking = false;          /* inside APPROACH_M           */
    bool        bt_stop = false;            /* station: driver chose stop  */
    bool        bt_active;
    bool        bt_asleep;
    bool        bt_in_zone = false;
    uint32_t    u4t_zone_t0 = 0U;
    uint32_t    u4t_track = TRACK_UDON;
    uint32_t    u4t_result;
    uint16_t    u2t_pot;
    uint16_t    u2t_last_pot = ADC_Read(THROTTLE_ADC_CH);
    btn_event_t st_event;

    LED_Set(LED_GREEN, true);
    send_time_tag();
    UART_SendString("[DRIVE] มุ่งหน้า ");
    UART_SendString(pt_name);
    UART_SendString("\r\n");
    if (bt_junction == true) {
        send_time_tag();
        UART_SendString("  ตอนนี้อยู่เส้นทาง 1 (อุดรธานี) -> กดปุ่ม 4 เพื่อสับราง\r\n");
    } else {
        /* No action */
    }

    while (s4t_dist > 0) {
        st_event.b_stop_held = false;
        st_event.u4_stop_presses = 0U;
        st_event.u4_track_presses = 0U;
        st_event.u4_dead_presses = 0U;
        Seg7_Show((uint8_t)s4t_count);

        if (poll_buttons_1s(&st_event) == POLL_EMERGENCY) {
            /* emergency brake -> alarm -> continue from the same distance */
            run_alarm("EMERGENCY");
            resume_driving(bt_asking);
            s4t_count = DEADMAN_COUNT_START;
            u2t_last_pot = ADC_Read(THROTTLE_ADC_CH);
        } else {
            /* throttle and how much it moved in the last second */
            u2t_pot = ADC_Read(THROTTLE_ADC_CH);
            s4t_dpot = (int32_t)u2t_pot - (int32_t)u2t_last_pot;
            if (s4t_dpot < 0) {
                s4t_dpot = -s4t_dpot;
            } else {
                /* No action */
            }
            u2t_last_pot = u2t_pot;

            /* station: STOP inside the approach zone = will stop */
            if ((bt_junction == false) && (bt_asking == true) && (bt_stop == false) &&
                ((st_event.b_stop_held == true) || (st_event.u4_stop_presses > 0U))) {
                bt_stop = true;
                Buzzer_Beep(BEEP_STOP_ACK);
                send_time_tag();
                UART_SendString(">> รับทราบ: จะจอด ");
                UART_SendString(pt_name);
                UART_SendString("\r\n");
            } else {
                /* No action */
            }

            /* junction: odd number of button-4 presses = switch track */
            if ((bt_junction == true) && ((st_event.u4_track_presses % TOGGLE_DIVISOR) != 0U)) {
                if (u4t_track == TRACK_UDON) {
                    u4t_track = TRACK_KHONKAEN;
                } else {
                    u4t_track = TRACK_UDON;
                }
                Buzzer_Beep(BEEP_SHORT);
                send_time_tag();
                UART_SendString(">> สับราง: เส้นทาง ");
                UART_SendUint(u4t_track);
                UART_SendString(" (");
                UART_SendString(ptg_track_name[u4t_track]);
                UART_SendString(")\r\n");
            } else {
                /* No action */
            }

            /* deadman: any new press or throttle movement = awake */
            bt_active = ((st_event.u4_dead_presses > 0U) || (st_event.u4_stop_presses > 0U) ||
                         (st_event.u4_track_presses > 0U) || (s4t_dpot > THROTTLE_ACTIVE_DELTA));
            bt_asleep = false;
#if (DEADMAN_ENABLED == 1)
            if (bt_active == true) {
                s4t_count = DEADMAN_COUNT_START;
            } else {
                s4t_count--;
                if (s4t_count < 0) {
                    bt_asleep = true;
                } else {
                    /* still counting */
                }
            }
#else
            (void)bt_active;
            s4t_count--;
            if (s4t_count < 0) {
                s4t_count = DEADMAN_COUNT_START;
            } else {
                /* No action */
            }
#endif

            if (bt_asleep == true) {
                Seg7_Show(SEG7_ZERO);
                run_alarm("DEADMAN (หลับใน: ไม่ขยับคันเร่งหรือกดปุ่มเลย)");
                resume_driving(bt_asking);
                s4t_count = DEADMAN_COUNT_START;
                u2t_last_pot = ADC_Read(THROTTLE_ADC_CH);
            } else {
                /* move forward: speed (km/h) from the throttle */
                s4t_speed = SPEED_MIN_KMH +
                            (int32_t)(((uint32_t)u2t_pot * (uint32_t)(SPEED_MAX_KMH - SPEED_MIN_KMH)) /
                                      ADC_MAX_VALUE);
                s4t_dist = s4t_dist - ((s4t_speed * M_PER_KM) / SEC_PER_HOUR_S);
                if (s4t_dist < 0) {
                    s4t_dist = 0;
                } else {
                    /* No action */
                }

                /* community zone: enter -> grace time -> camera every second */
                if ((s4t_zone_m > 0) && (bt_in_zone == false) && (s4t_dist <= s4t_zone_m)) {
                    bt_in_zone = true;
                    u4t_zone_t0 = Timebase_GetMs();
                    bg_zone_entered = true;
                    Buzzer_Beep(BEEP_SHORT);
                    send_time_tag();
                    UART_SendString("[ZONE] เข้าเขตชุมชน จำกัด ");
                    UART_SendUint((uint32_t)ZONE_LIMIT_KMH);
                    UART_SendString(" km/h -> กล้องตรวจจับความเร็วเริ่มทำงานใน ");
                    UART_SendUint(ZONE_GRACE_MS / MS_PER_SECOND);
                    UART_SendString(" วินาที\r\n");
                } else {
                    /* No action */
                }
                if ((bt_in_zone == true) && ((Timebase_GetMs() - u4t_zone_t0) >= ZONE_GRACE_MS) &&
                    (s4t_speed > ZONE_LIMIT_KMH)) {
                    if (u4g_zone_violation == 0U) {
                        send_time_tag();
                        UART_SendString("[CAMERA] ตรวจพบความเร็ว ");
                        UART_SendUint((uint32_t)s4t_speed);
                        UART_SendString(" km/h เกินกำหนดในเขตชุมชน (จะถูกเตือนตอนสรุปผล)\r\n");
                    } else {
                        /* already reported */
                    }
                    u4g_zone_violation++;
                    if (s4t_speed > s4g_zone_max_kmh) {
                        s4g_zone_max_kmh = s4t_speed;
                    } else {
                        /* No action */
                    }
                } else {
                    /* No action */
                }

                /* approach zone: warn once, the train keeps moving */
                if ((bt_asking == false) && (s4t_dist > 0) && (s4t_dist <= APPROACH_M)) {
                    bt_asking = true;
                    LED_Set(LED_BLUE, true);
                    send_time_tag();
                    UART_SendString("[NEAR] ใกล้ ");
                    UART_SendString(pt_name);
                    UART_SendString(" อีก ");
                    UART_SendUint((uint32_t)s4t_dist);
                    if (bt_junction == true) {
                        UART_SendString(" m -> ตอนนี้อยู่เส้นทาง ");
                        UART_SendUint(u4t_track);
                        UART_SendString(" (");
                        UART_SendString(ptg_track_name[u4t_track]);
                        UART_SendString(") กดปุ่ม 4 เพื่อสับราง\r\n");
                    } else {
                        UART_SendString(" m -> กด STOP ถ้าจะจอด (ไม่กด = ขับผ่าน)\r\n");
                    }
                } else {
                    /* No action */
                }

                print_status(pt_name, bt_junction, s4t_dist, s4t_speed, u2t_pot, u4t_track,
                             bt_in_zone);
            }
        }
    }

    LED_Set(LED_BLUE, false);
    if (bt_junction == true) {
        u4t_result = u4t_track;
    } else {
        send_time_tag();
        if (bt_stop == true) {
            UART_SendString("[ARRIVE] ถึง ");
            UART_SendString(pt_name);
            UART_SendString(" -> จอด\r\n");
            u4t_result = LEG_RESULT_STOP;
        } else {
            UART_SendString("[PASS] ไม่ได้ตอบ -> ขับผ่าน ");
            UART_SendString(pt_name);
            UART_SendString("\r\n");
            u4t_result = LEG_RESULT_PASS;
        }
    }
    return u4t_result;
}

/*********************************************************************
 * @fn                - drive_to_station
 * @brief             - Drive to a station
 *
 * @return            - true = driver stopped at the station
 *********************************************************************/
static bool drive_to_station(const char *pt_name, int32_t s4t_zone_m)
{
    return (drive_leg(pt_name, LEG_STATION, s4t_zone_m) == LEG_RESULT_STOP);
}

/*********************************************************************
 * @fn                - drive_to_junction
 * @brief             - Drive to the junction
 *
 * @return            - track selected when arriving (TRACK_xxx)
 *********************************************************************/
static uint32_t drive_to_junction(const char *pt_name)
{
    return drive_leg(pt_name, LEG_JUNCTION, NO_ZONE_M);
}

/*********************************************************************
 * @fn                - wait_depart
 * @brief             - End of a station activity: wait for 'g' to depart
 *                      (LEDs stay as they are while waiting)
 *********************************************************************/
static void wait_depart(void)
{
    UART_FlushRx();
    Seg7_Show(SEG7_ZERO);
    send_time_tag();
    UART_SendString(">> พร้อมออกรถ พิมพ์ 'g' เพื่อออกจากสถานี\r\n");
    wait_for_key(KEY_GO_LOWER, KEY_GO_UPPER, false);
    LED_SetAll(false);
    send_time_tag();
    UART_SendString("[DEPART] ออกจากสถานี\r\n");
}

/*********************************************************************
 * @fn                - passenger_minigame
 * @brief             - Buttons 1-4 = doors of cars 1-4 (LED G/Y/R/B).
 *                      Press: open, LED on while boarding (DOOR_LOAD_MS),
 *                      then LED blinks: press the same button to close.
 *                      All 4 closed -> 'g'.
 *********************************************************************/
static void passenger_minigame(void)
{
    door_state_t et_door[CAR_COUNT];
    bool         bt_prev[CAR_COUNT];
    uint32_t     u4t_open_ms[CAR_COUNT];
    uint32_t     u4t_done = 0U;
    uint32_t     u4t_now;
    uint32_t     u4t_i;
    bool         bt_blink_on;

    for (u4t_i = 0U; u4t_i < CAR_COUNT; u4t_i++) {
        et_door[u4t_i] = DOOR_CLOSED;
        bt_prev[u4t_i] = Button_IsPressed(etg_car_button[u4t_i]);   /* ignore held buttons */
        u4t_open_ms[u4t_i] = 0U;
    }
    Buzzer_Beep(BEEP_ARRIVE);
    LED_SetAll(false);
    Seg7_Show(SEG7_ZERO);
    send_time_tag();
    UART_SendString("[PASSENGER] รับผู้โดยสาร: กดปุ่ม 1-4 เปิดประตูตู้ 1-4 (เขียว/เหลือง/แดง/น้ำเงิน)\r\n");
    UART_SendString("  ไฟติด = กำลังขึ้นรถ 5 วิ | ไฟกระพริบ = ขึ้นครบ -> กดปุ่มเดิมเพื่อปิดประตู\r\n");

    while (u4t_done < CAR_COUNT) {
        u4t_now = Timebase_GetMs();
        bt_blink_on = (((u4t_now / BLINK_HALF_PERIOD_MS) % TOGGLE_DIVISOR) != 0U);

        for (u4t_i = 0U; u4t_i < CAR_COUNT; u4t_i++) {
            if (rising_edge(Button_IsPressed(etg_car_button[u4t_i]), &bt_prev[u4t_i]) == true) {
                switch (et_door[u4t_i]) {
                    case DOOR_CLOSED:
                        et_door[u4t_i] = DOOR_LOADING;
                        u4t_open_ms[u4t_i] = u4t_now;
                        Buzzer_Beep(BEEP_DOOR);
                        send_time_tag();
                        UART_SendString("  เปิดประตูตู้ ");
                        UART_SendUint(u4t_i + 1U);
                        UART_SendString(" ผู้โดยสารกำลังขึ้น...\r\n");
                        break;
                    case DOOR_READY:
                        et_door[u4t_i] = DOOR_DONE;
                        u4t_done++;
                        Buzzer_Beep(BEEP_DOOR);
                        Seg7_Show((uint8_t)u4t_done);
                        send_time_tag();
                        UART_SendString("  ปิดประตูตู้ ");
                        UART_SendUint(u4t_i + 1U);
                        UART_SendString(" เรียบร้อย (");
                        UART_SendUint(u4t_done);
                        UART_SendString("/4)\r\n");
                        break;
                    default:
                        /* boarding or already closed: ignore */
                        break;
                }
            } else {
                /* No action */
            }

            if ((et_door[u4t_i] == DOOR_LOADING) && ((u4t_now - u4t_open_ms[u4t_i]) >= DOOR_LOAD_MS)) {
                et_door[u4t_i] = DOOR_READY;
                send_time_tag();
                UART_SendString("  ตู้ ");
                UART_SendUint(u4t_i + 1U);
                UART_SendString(" ขึ้นครบแล้ว -> กดปุ่ม ");
                UART_SendUint(u4t_i + 1U);
                UART_SendString(" เพื่อปิดประตู\r\n");
            } else {
                /* No action */
            }

            if (et_door[u4t_i] == DOOR_LOADING) {
                LED_Set(etg_car_led[u4t_i], true);
            } else if (et_door[u4t_i] == DOOR_READY) {
                LED_Set(etg_car_led[u4t_i], bt_blink_on);
            } else {
                LED_Set(etg_car_led[u4t_i], false);
            }
        }
        Timebase_DelayMs(BUTTON_SAMPLE_MS);
    }

    LED_SetAll(false);
    send_time_tag();
    UART_SendString("  ปิดประตูครบทุกตู้แล้ว\r\n");
    wait_depart();
}

/*********************************************************************
 * @fn                - charge_minigame
 * @brief             - Press button 4 CHARGE_PRESSES times; the LED gauge
 *                      fills with the number of presses. Full -> 'g'.
 *********************************************************************/
static void charge_minigame(void)
{
    uint32_t u4t_count = 0U;
    uint32_t u4t_level;
    uint32_t u4t_last_level = 0U;
    bool     bt_prev = Button_IsPressed(BTN_TRACK);

    Buzzer_Beep(BEEP_ARRIVE);
    LED_SetAll(false);
    Seg7_Show(SEG7_ZERO);
    send_time_tag();
    UART_SendString("[CHARGE] ชาร์จไฟ: กดปุ่ม 4 รัว ๆ ให้ครบ ");
    UART_SendUint(CHARGE_PRESSES);
    UART_SendString(" ครั้ง\r\n");

    while (u4t_count < CHARGE_PRESSES) {
        if (rising_edge(Button_IsPressed(BTN_TRACK), &bt_prev) == true) {
            u4t_count++;
            u4t_level = ((u4t_count * GAUGE_LEVELS) + (CHARGE_PRESSES - 1U)) / CHARGE_PRESSES;
            gauge_show(u4t_level);
            Seg7_Show((uint8_t)((u4t_count * SEG7_MAX_DIGIT) / CHARGE_PRESSES));
            if ((u4t_level != u4t_last_level) || ((u4t_count % CHARGE_REPORT_EVERY) == 0U)) {
                send_time_tag();
                UART_SendString("  ชาร์จ ");
                UART_SendUint(u4t_count);
                UART_SendString("/");
                UART_SendUint(CHARGE_PRESSES);
                UART_SendString(" ไฟ: ");
                UART_SendString(ptg_charge_level_name[u4t_level]);
                UART_SendString("\r\n");
                u4t_last_level = u4t_level;
            } else {
                /* No action */
            }
        } else {
            /* No action */
        }
        Timebase_DelayMs(BUTTON_SAMPLE_MS);
    }

    Buzzer_Beep(BEEP_DONE);
    send_time_tag();
    UART_SendString("  แบตเต็ม 100%!\r\n");
    wait_depart();
}

/*********************************************************************
 * @fn                - reset_trip
 * @brief             - All lights off, clock stopped, zone record cleared
 *********************************************************************/
static void reset_trip(void)
{
    LED_SetAll(false);
    Seg7_Show(SEG7_ZERO);
    bg_trip_on = false;
    bg_zone_entered = false;
    u4g_zone_violation = 0U;
    s4g_zone_max_kmh = 0;
    UART_SendString("\r\n==== Bullet Train Sim ====\r\n");
}

/*********************************************************************
 * @fn                - wait_for_card
 * @brief             - Silent until an RFID card is read. The RC522 is set
 *                      up again every trip and checked every RFID_CHECK_MS.
 *
 * @param[out]        - pt_uid : UID of the card (RC522_UID_LEN bytes)
 *********************************************************************/
static void wait_for_card(uint8_t *pt_uid)
{
    bool     bt_got_card = false;
    bool     bt_alive;
    bool     bt_alive_now;
    uint32_t u4t_check_ms;

    UART_SendString("[START] แตะบัตร RFID เพื่อเริ่ม...\r\n");
    RC522_Init();
    bt_alive = RC522_IsAlive();
    u4t_check_ms = Timebase_GetMs();
    if (bt_alive == false) {
        UART_SendString("[RFID] โมดูล RC522 ไม่ตอบ -> เช็กสาย SDA=A0 SCK=A3 MOSI=A4 MISO=A5, 3.3V, GND, RST\r\n");
    } else {
        /* No action */
    }

    while (bt_got_card == false) {
        bt_got_card = RC522_ReadUid(pt_uid);
        if ((bt_got_card == false) && ((Timebase_GetMs() - u4t_check_ms) >= RFID_CHECK_MS)) {
            u4t_check_ms = Timebase_GetMs();
            bt_alive_now = RC522_IsAlive();
            if ((bt_alive_now == true) && (bt_alive == false)) {
                RC522_Init();
                UART_SendString("[RFID] เชื่อมต่อโมดูลได้แล้ว แตะบัตรได้เลย\r\n");
            } else if ((bt_alive_now == false) && (bt_alive == true)) {
                UART_SendString("[RFID] โมดูล RC522 หลุด -> เช็กสาย\r\n");
            } else {
                /* no change */
            }
            bt_alive = bt_alive_now;
        } else {
            /* No action */
        }
    }
}

/*********************************************************************
 * @fn                - print_mission
 * @brief             - Mission briefing before departure
 *********************************************************************/
static void print_mission(void)
{
    UART_SendString("==== ภารกิจ ====\r\n");
    UART_SendString("  ปลายทาง: สถานีขอนแก่น\r\n");
    UART_SendString("  ออกรถ ");
    send_clock(CLOCK_START_S);
    UART_SendString(" -> ต้องถึงสถานีขอนแก่นก่อน ");
    send_clock(ARRIVE_DEADLINE_S);
    UART_SendString(" (ถึงช้ากว่านี้ = มาสาย แพ้)\r\n");
    UART_SendString("  ที่จุดแยกต้องเข้าเส้นทาง 2 (ขอนแก่น) แต่รางตั้งต้นคือเส้นทาง 1 (อุดรธานี)\r\n");
    UART_SendString("  -> กดปุ่ม 4 เพื่อสับรางให้ทันก่อนถึงจุดแยก ไม่งั้นเลยเป้าหมาย = แพ้\r\n");
    UART_SendString("  ทุกสถานี: เหลือ 200 m จะถาม -> กด STOP ทัน = จอด\r\n");
    UART_SendString("  ก่อนถึงขอนแก่นเป็นเขตชุมชน จำกัด ");
    UART_SendUint((uint32_t)ZONE_LIMIT_KMH);
    UART_SendString(" km/h (มีกล้องตรวจจับความเร็ว)\r\n");
    UART_SendString("  Deadman: ต้องขยับคันเร่งหรือกดปุ่มบ้าง ถ้านิ่งจน 7-seg นับถึง 0 = หลับใน\r\n");
}

/*********************************************************************
 * @fn                - start_trip
 * @brief             - Wait for 'g', then start the trip clock and depart
 *********************************************************************/
static void start_trip(void)
{
    UART_SendString("[START] พิมพ์ 'g' เพื่อเริ่ม\r\n");
    wait_for_key(KEY_GO_LOWER, KEY_GO_UPPER, true);

    u4g_train_no++;
    LED_Set(LED_GREEN, true);
    u4g_trip_ms0 = Timebase_GetMs();
    bg_trip_on = true;
    send_time_tag();
    UART_SendString("[DEPART] ออกรถ (ไฟเขียว)\r\n");
}

/*********************************************************************
 * @fn                - print_summary
 * @brief             - Trip result, time used and speed camera warning
 *
 * @param[in]         - bt_win : true = won
 * @param[in]         - pt_reason : reason when lost
 * @param[in]         - u4t_end_s : clock when the trip ended
 *********************************************************************/
static void print_summary(bool bt_win, const char *pt_reason, uint32_t u4t_end_s)
{
    uint32_t u4t_used_s = u4t_end_s - CLOCK_START_S;

    UART_SendString("==== สรุปผล: รถหมายเลข #");
    UART_SendUint(u4g_train_no);
    UART_SendString(" ====\r\n");

    UART_SendString("  ออกรถ ");
    send_clock(CLOCK_START_S);
    UART_SendString(" -> จบ ");
    send_clock(u4t_end_s);
    UART_SendString(" (กำหนดถึงก่อน ");
    send_clock(ARRIVE_DEADLINE_S);
    UART_SendString(")\r\n");

    UART_SendString("  ใช้เวลาเดินทาง ");
    UART_SendUint(u4t_used_s / SEC_PER_MIN);
    UART_SendString(" นาที ");
    UART_SendUint(u4t_used_s % SEC_PER_MIN);
    UART_SendString(" วินาที\r\n");

    if (bg_zone_entered == true) {
        if (u4g_zone_violation > 0U) {
            UART_SendString("  เตือน: ขับเร็วเกิน ");
            UART_SendUint((uint32_t)ZONE_LIMIT_KMH);
            UART_SendString(" km/h ในเขตชุมชน (กล้องจับได้ ");
            UART_SendUint(u4g_zone_violation);
            UART_SendString(" ครั้ง, สูงสุด ");
            UART_SendUint((uint32_t)s4g_zone_max_kmh);
            UART_SendString(" km/h)\r\n");
        } else {
            UART_SendString("  เขตชุมชน: ขับตามกำหนดความเร็ว ไม่มีใบเตือน\r\n");
        }
    } else {
        /* did not reach the zone */
    }

    if (bt_win == true) {
        UART_SendString("  ผล: ชนะ! ถึงขอนแก่นทันเวลา (เหลือเวลา ");
        UART_SendUint(ARRIVE_DEADLINE_S - u4t_end_s);
        UART_SendString(" วินาที)\r\n");
    } else {
        UART_SendString("  ผล: แพ้ (");
        UART_SendString(pt_reason);
        UART_SendString(")\r\n");
    }
}
