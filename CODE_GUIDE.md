# คู่มืออ่านโค้ด Bullet Train Simulator

คู่มือนี้เขียนให้คนที่ยังไม่ถนัดภาษา C อ่านโค้ดโปรเจกต์นี้ได้
ไม่ต้องอ่านทุกบรรทัด ให้อ่านตามลำดับในข้อ 1 แล้วเปิดข้ออื่นเมื่อสงสัย

**สารบัญ**
1. [เริ่มอ่านตรงไหน](#1-เริ่มอ่านตรงไหน)
2. [ภาษา C ฉบับย่อ (เฉพาะที่โปรเจกต์นี้ใช้)](#2-ภาษา-c-ฉบับย่อ)
3. [อ่านชื่อตัวแปรให้ออก](#3-อ่านชื่อตัวแปรให้ออก)
4. [รูปแบบโค้ดตาม guideline ที่อาจดูแปลก](#4-รูปแบบโค้ดตาม-guideline-ที่อาจดูแปลก)
5. [รูปแบบการตั้งค่า peripheral](#5-รูปแบบการตั้งค่า-peripheral)
6. [แผนที่ไฟล์](#6-แผนที่ไฟล์)
7. [Interrupt แต่ละตัวทำงานยังไง](#7-interrupt-แต่ละตัวทำงานยังไง)
8. [ตรรกะของเกม](#8-ตรรกะของเกม)
9. [อยากปรับค่า แก้ตรงไหน](#9-อยากปรับค่า-แก้ตรงไหน)
10. [คำถามที่อาจโดนถามตอนพรีเซนต์](#10-คำถามที่อาจโดนถามตอนพรีเซนต์)
11. [คำศัพท์](#11-คำศัพท์)

---

## 1. เริ่มอ่านตรงไหน

| ลำดับ | เปิดไฟล์ | ดูอะไร |
|---|---|---|
| 1 | `Src/main.c` | ไฟล์สั้นมาก มีแค่ "เตรียมระบบ แล้ววนเล่นเกม" |
| 2 | `Src/train_game.c` → `TrainGame_Init()` | เปิดอุปกรณ์ทุกตัวตามลำดับ |
| 3 | `Src/train_game.c` → `TrainGame_RunJourney()` | เนื้อเรื่องของเกม 1 รอบ ตั้งแต่แตะบัตรจนสรุปผล |
| 4 | `Src/train_game.c` → `drive_leg()` | การขับรถ 1 ช่วง (หัวใจของเกม) |
| 5 | driver แต่ละตัว ตามตารางในข้อ 6 | ดูว่าฮาร์ดแวร์แต่ละชิ้นถูกสั่งยังไง |
| 6 | `Inc/board.h` | อุปกรณ์ไหนต่อขาไหน |

**ปุ่มลัดใน STM32CubeIDE ที่ช่วยได้มาก**

| ปุ่ม | ใช้ทำอะไร |
|---|---|
| `Ctrl` + คลิกชื่อ หรือ `F3` | กระโดดไปที่ตัวฟังก์ชัน / ค่าคงที่นั้น |
| `Alt` + `←` | กลับไปที่เดิม |
| `Ctrl` + `Alt` + `H` | ดูว่าฟังก์ชันนี้ถูกเรียกจากที่ไหนบ้าง (Call Hierarchy) |
| `Ctrl` + `H` | ค้นคำในทั้งโปรเจกต์ |
| `Ctrl` + `O` | รายชื่อฟังก์ชันทั้งหมดในไฟล์ที่เปิดอยู่ |

---

## 2. ภาษา C ฉบับย่อ

### 2.1 ไฟล์ `.h` กับ `.c`
- **`.h` (header, โฟลเดอร์ `Inc`)** = เหมือน "เมนู" บอกว่าไฟล์นี้มีฟังก์ชันอะไรให้คนอื่นเรียกใช้ และมีค่าคงที่อะไร
- **`.c` (source, โฟลเดอร์ `Src`)** = ตัวโค้ดจริงของฟังก์ชันเหล่านั้น
- `#include "uart.h"` = "ขอใช้ของที่อยู่ใน uart.h" ไฟล์ที่ include แล้วจะเรียก `UART_SendString(...)` ได้

### 2.2 `#define` = ตั้งชื่อให้ตัวเลข
```c
#define SEG_DISTANCE_M          (1500)   /* distance of each leg (m) */
```
ทุกที่ที่เขียน `SEG_DISTANCE_M` คอมไพเลอร์จะแทนด้วย `1500` อยากเปลี่ยนค่าก็แก้ที่นี่ที่เดียว
ตัว `U` ท้ายตัวเลข เช่น `1000U` แปลว่าเป็นเลขไม่ติดลบ (unsigned) ซึ่งเป็นกฎของ guideline

### 2.3 ชนิดข้อมูลที่เจอบ่อย
| ชนิด | เก็บค่าได้ | ใช้กับอะไรในโปรเจกต์ |
|---|---|---|
| `uint8_t` | 0 – 255 (1 byte) | ข้อมูลที่รับส่งกับ RC522, ตัวเลข 7-segment |
| `uint16_t` | 0 – 65,535 | ค่า ADC (0 – 4095) |
| `uint32_t` | 0 – 4,294,967,295 | เวลาเป็น ms, ค่ารีจิสเตอร์, ตัวนับ |
| `int32_t` | ติดลบได้ | ระยะทาง ความเร็ว (ตอนคำนวณอาจติดลบ) |
| `bool` | `true` / `false` | สถานะ เช่น กดปุ่มอยู่ไหม |
| `char` | ตัวอักษร 1 ตัว | ปุ่มที่พิมพ์ เช่น `'g'` |
| `const char *` | ข้อความ | `"สถานีชาร์จไฟ"` |

### 2.4 ฟังก์ชัน
```c
uint16_t ADC_Read(uint32_t u4t_channel)
```
อ่านจากซ้ายไปขวา: ฟังก์ชันชื่อ `ADC_Read` รับค่า 1 ตัว (หมายเลขช่อง) แล้ว **คืนค่า** เป็น `uint16_t`
ถ้าเขียน `void` แปลว่าไม่คืนค่า หรือไม่รับค่า เช่น `void UART_Init(void)`

### 2.5 คำสั่งควบคุม
```c
if (ADC_Read(ADC_CH_LDR) > HEADLIGHT_ON_THRESHOLD) {   /* ถ้า ... */
    LED_Set(LED_YELLOW, true);
} else {                                                /* ไม่งั้น ... */
    LED_Set(LED_YELLOW, false);
}

while (s4t_dist > 0) { ... }       /* ทำซ้ำตราบที่เงื่อนไขยังจริง */

for (u4t_i = 0U; u4t_i < 4U; u4t_i++) { ... }   /* ทำซ้ำ 4 รอบ: i = 0,1,2,3 */

switch (et_door[u4t_i]) {          /* เลือกทำตามค่า */
    case DOOR_CLOSED: ... break;
    case DOOR_READY:  ... break;
    default:          ... break;
}
```
`u4t_i++` = เพิ่มค่าทีละ 1, `u4t_new--` = ลดทีละ 1

### 2.6 enum / struct / array
- **enum** = ตั้งชื่อให้เลข 0, 1, 2, ...
  ```c
  typedef enum { BUTTON_1 = 0, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_COUNT } button_id_t;
  ```
  `BUTTON_COUNT` = 4 (นับจำนวนให้อัตโนมัติ)
- **struct** = กล่องที่รวมหลายค่าไว้ด้วยกัน เข้าถึงสมาชิกด้วย `.`
  ```c
  btn_event_t st_event;
  st_event.u4_stop_presses = 0U;
  ```
- **array** = ตัวแปรหลายช่องเรียงกัน เลขช่องเริ่มที่ 0
  `u2g_adc_buf[3]` มีช่อง `[0] [1] [2]`

### 2.7 pointer (`&`, `*`, `->`)
- `&x` = **ที่อยู่** ของตัวแปร x, `*p` = **ค่า** ที่ p ชี้อยู่
- ใช้เมื่อฟังก์ชันต้องส่งค่ากลับมากกว่า 1 ค่า เช่น
  ```c
  char ct_rx;
  if (UART_ReadChar(&ct_rx) == true) { ... }   /* คืน true/false และเขียนตัวอักษรลง ct_rx */
  ```
- `->` = เข้าถึงสมาชิกผ่าน pointer
  - `USART2->DR`, `GPIOA->MODER`, `TIM2->ARR` → **รีจิสเตอร์ของชิป** (`USART2`, `GPIOA`, `TIM2` คือ pointer ไปยังตำแหน่งรีจิสเตอร์ มาจากไฟล์ CMSIS `stm32f411xe.h`)
  - `pt_event->u4_stop_presses` → สมาชิกของ struct ที่ส่งมาเป็น pointer

### 2.8 `static`, `volatile`, `const`
| คำ | ความหมาย | ตัวอย่าง |
|---|---|---|
| `static` (หน้าฟังก์ชัน / ตัวแปรนอกฟังก์ชัน) | ใช้ได้เฉพาะในไฟล์นี้ (private) | `static uint32_t drive_leg(...)` |
| `volatile` | ค่าอาจถูกเปลี่ยนโดย **ISR หรือฮาร์ดแวร์** ได้ทุกเมื่อ คอมไพเลอร์ต้องอ่านค่าจริงทุกครั้ง **ตัวแปรที่ ISR แก้ต้องมีเสมอ** | `static volatile uint32_t u4g_ms` |
| `const` | ค่าคงที่ อ่านอย่างเดียว | ตารางขา `stg_button_pins` |

### 2.9 Bitwise: ภาษาของรีจิสเตอร์ (สำคัญที่สุด)
รีจิสเตอร์ 1 ตัวมี 32 บิต แต่ละบิตเป็นสวิตช์ของฟังก์ชันหนึ่ง

| เขียน | ความหมาย |
|---|---|
| `1U << 5` | เลข 1 ที่บิตตำแหน่ง 5 (= 0b100000) |
| `x \|= m` | **เปิด** บิตที่ m ระบุ (บิตอื่นไม่ยุ่ง) |
| `x &= ~m` | **ปิด** บิตที่ m ระบุ (บิตอื่นไม่ยุ่ง) |
| `(x & m) != 0U` | **เช็ค** ว่าบิตนั้นเป็น 1 ไหม |
| `x >> 4` | เลื่อนบิตไปทางขวา 4 ตำแหน่ง |

ตัวอย่างจริงในโปรเจกต์
```c
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;     /* เปิดบิต: จ่ายนาฬิกาให้ USART2 (เปิดเครื่อง) */
EXTI->IMR &= ~u4t_mask;                   /* ปิดบิต: ไม่ให้ line นี้เกิด interrupt      */
if ((USART2->SR & USART_SR_RXNE) != 0U)   /* เช็คบิต: มีข้อมูลเข้ามาแล้วหรือยัง         */
```
ชื่อยาวๆ อย่าง `RCC_APB1ENR_USART2EN` คือ `#define` ของตำแหน่งบิต มาจาก CMSIS
เปิด Reference Manual (RM0383) แล้วค้นชื่อรีจิสเตอร์ เช่น "APB1ENR" จะเจอคำอธิบายทุกบิต

---

## 3. อ่านชื่อตัวแปรให้ออก

ชื่อในโปรเจกต์นี้บอกชนิดและขอบเขตไว้ในตัว (ตามกฎของรายวิชา)

| ขึ้นต้น / ลงท้าย | ความหมาย | ตัวอย่าง |
|---|---|---|
| `u1t_` `u2t_` `u4t_` | ตัวแปรในฟังก์ชัน ไม่ติดลบ ขนาด 1 / 2 / 4 byte | `u4t_i`, `u2t_pot` |
| `s4t_` | ตัวแปรในฟังก์ชัน ติดลบได้ 4 byte | `s4t_dist`, `s4t_speed` |
| `bt_` `ct_` `pt_` `et_` `st_` | ตัวแปรในฟังก์ชัน: bool / char / pointer / enum / struct | `bt_done`, `pt_name` |
| `u4g_` `bg_` `s4g_` `u2g_` `u1g_` | ตัวแปร **global ของไฟล์** (g = global) | `u4g_ms`, `bg_irq_flag` |
| `ctg_` `ptg_` `etg_` `stg_` | ตารางค่าคงที่ระดับไฟล์ | `stg_button_pins` |
| `u4_` `b_` (ไม่มี t/g) | สมาชิกของ struct | `u4_stop_presses` |
| `ตัวพิมพ์ใหญ่ทั้งหมด` | ค่าคงที่ `#define` | `DRIVE_TICK_MS` |
| `Module_Function` | ฟังก์ชันที่ไฟล์อื่นเรียกได้ (public) | `UART_SendString`, `Button_TakePresses` |
| `ตัวเล็ก_ทั้งหมด()` | ฟังก์ชันภายในไฟล์ (private, `static`) | `drive_leg`, `button_settle` |
| `xxx_IRQHandler` | **ISR**: ชิปเรียกเองเมื่อเกิด interrupt ห้ามเรียกเอง | `USART2_IRQHandler` |
| `xxx_Callback` | ฟังก์ชันที่ ISR ของไฟล์อื่นเรียก | `Button_ExtiCallback` |
| ลงท้าย `_t` | ชื่อชนิดข้อมูล (type) | `button_id_t`, `btn_event_t` |

---

## 4. รูปแบบโค้ดตาม guideline ที่อาจดูแปลก

โค้ดเขียนตาม Guideline Checksheet ของรายวิชา เลยมีหลายจุดที่ดูยาวกว่าปกติ แต่ความหมายเหมือนเดิม

| เห็นแบบนี้ | ความหมาย / ทำไม |
|---|---|
| `if (bt_done == true)` | = `if (bt_done)` กฎให้เขียนเปรียบเทียบชัดๆ |
| `else { /* No action */ }` | กฎ: ทุก `if` ต้องมี `else` แม้ไม่ทำอะไร |
| `/* ... */` แทน `//` | กฎห้ามใช้ comment แบบ `//` |
| `(void)spi_transfer(...)` | ตั้งใจไม่ใช้ค่าที่ฟังก์ชันคืนมา |
| `(uint32_t)x` | แปลงชนิดข้อมูลให้ชัดเจน (cast) |
| บล็อก `@fn @brief @param @return` | คำอธิบายฟังก์ชัน อ่านตรงนี้ก่อนจะรู้ว่าฟังก์ชันทำอะไร |
| หัวข้อ `/* Private define ---- */` ฯลฯ | ทุกไฟล์เรียงหัวข้อเหมือน Template.c / Template.h |

ลำดับหัวข้อในไฟล์ `.c`: Includes → Private define → Private variables → Private function prototypes →
**Public functions** (เรียกจากไฟล์อื่น) → **Callback functions** (ISR) → Private functions

---

## 5. รูปแบบการตั้งค่า peripheral

ทุก peripheral ในโปรเจกต์ตั้งค่าตามขั้นตอนเดียวกัน จำ 6 ข้อนี้แล้วอ่านฟังก์ชัน `xxx_Init()` ได้ทุกตัว

1. **เปิดนาฬิกา** ให้ peripheral: `RCC->APB1ENR |= RCC_APB1ENR_xxxEN;` (ถ้าไม่เปิด peripheral จะไม่ทำงานเลย)
2. **ตั้งโหมดขา GPIO**: `GPIO_SetMode(port, pin, GPIO_MODE_xxx)` (input / output / alternate function / analog)
3. **ตั้งค่า peripheral**: ความเร็ว โหมด ช่อง ฯลฯ
4. **เปิด interrupt ใน peripheral**: บิตที่ลงท้าย `IE` เช่น `USART_CR1_RXNEIE`, `TIM_DIER_UIE`
5. **เปิด interrupt ใน NVIC**: `NVIC_EnableIRQ(xxx_IRQn);` (NVIC = ตัวจัดการ interrupt ของ CPU)
6. **เขียน ISR**: `void xxx_IRQHandler(void)` ชื่อต้องตรงกับในไฟล์ `Startup/startup_stm32f411retx.s` แล้วชิปจะกระโดดมาเอง

ตัวอย่าง `UART_Init()` ใน `Src/uart.c`
```c
RCC->APB1ENR |= RCC_APB1ENR_USART2EN;                    /* 1. เปิดนาฬิกา USART2           */
GPIO_SetMode(UART_TX_PORT, UART_TX_PIN, GPIO_MODE_AF);   /* 2. PA2/PA3 เป็นขาของ UART     */
GPIO_SetAltFunc(UART_TX_PORT, UART_TX_PIN, UART_GPIO_AF);
USART2->BRR = UART_BRR_115200;                           /* 3. ความเร็ว 115200 bps        */
USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE); /* 3+4. เปิดส่ง/รับ + interrupt รับ */
NVIC_EnableIRQ(USART2_IRQn);                             /* 5. เปิดใน NVIC                 */
                                                         /* 6. ISR = USART2_IRQHandler()  */
```

---

## 6. แผนที่ไฟล์

| ไฟล์ (`Src/*.c` + `Inc/*.h`) | หน้าที่ | ฟังก์ชันที่ควรรู้ | Interrupt |
|---|---|---|---|
| `main.c` | จุดเริ่มโปรแกรม | `main()` | – |
| `board` | ตารางขาทั้งหมด + ฟังก์ชัน GPIO พื้นฐาน | `GPIO_SetMode`, `GPIO_WritePin`, `GPIO_ReadPin` | – |
| `timebase` | นาฬิกาของระบบ 1 ms และการหลับรอ | `Timebase_GetMs`, `Timebase_Sleep`, `Timebase_DelayMs` | `SysTick_Handler` |
| `uart` | คุยกับ serialterminal.com | `UART_SendString`, `UART_ReadChar` | `USART2_IRQHandler` |
| `adc` | อ่าน LDR และโพเทนฯ ผ่าน DMA | `ADC_Read` | – (ใช้ DMA) |
| `exti` | ตั้งค่า EXTI และ ISR ของ EXTI ทุกตัว | `Exti_ConfigLine` | `EXTI3/4/9_5/15_10_IRQHandler` |
| `button` | ปุ่ม 4 ปุ่ม + กันสัญญาณเด้ง | `Button_TakePresses`, `Button_IsPressed`, `Button_ClearPresses` | `TIM3_IRQHandler` (+ `Button_ExtiCallback`) |
| `buzzer` | เสียง buzzer | `Buzzer_Beep` | `TIM2_IRQHandler` |
| `rc522` | เครื่องอ่านบัตร RFID | `RC522_Init`, `RC522_ReadUid` | `RC522_ExtiCallback` |
| `led` | LED 4 ดวง | `LED_Set`, `LED_SetAll`, `LED_BlinkAll` | – |
| `seg7` | 7-segment (ส่งเลข BCD 4 บิต) | `Seg7_Show` | – |
| `headlight` | ไฟหน้าอัตโนมัติ (LED เหลือง) ตามแสง | `Headlight_Update` | – |
| `train_game` | ตรรกะเกมทั้งหมด | `TrainGame_Init`, `TrainGame_RunJourney` | – |
| `Startup/startup_*.s` | ตาราง vector ของ interrupt (ไม่ต้องแก้) | – | – |

---

## 7. Interrupt แต่ละตัวทำงานยังไง

**Interrupt** คือการที่ฮาร์ดแวร์ "สะกิด" CPU เมื่อมีเหตุการณ์ CPU หยุดงานปัจจุบันชั่วคราว
ไปทำ **ISR** (Interrupt Service Routine = ฟังก์ชัน `xxx_IRQHandler`) แล้วกลับมาทำต่อจากจุดเดิม

กฎที่โปรเจกต์นี้ใช้:
- **ISR ต้องสั้น** แค่เก็บข้อมูลหรือตั้ง flag ส่วนงานหนัก (พิมพ์ข้อความ ตรรกะเกม) ให้โปรแกรมหลักทำ
- ตอนโปรแกรมหลักต้องรอ จะเรียก `Timebase_Sleep()` ซึ่งข้างในคือ `__WFI()` (Wait For Interrupt)
  CPU จะ **หลับ** จนกว่ามี interrupt ตัวไหนก็ได้มาปลุก ไม่ต้องวนเช็คค่าซ้ำๆ (นี่คือเหตุผลที่โปรเจกต์ **ไม่มี polling**)

### 7.1 SysTick: นาฬิกา 1 ms (`timebase.c`)
```c
void SysTick_Handler(void)
{
    u4g_ms++;           /* เกิดทุก 1 ms -> นับเวลา */
}
```
- `Timebase_GetMs()` คืนเวลาเป็น ms นับจากเปิดเครื่อง
- `Timebase_DelayMs(n)` = หลับรอจนครบ n ms (SysTick ปลุกทุก 1 ms เพื่อเช็คว่าครบหรือยัง)
- ค่าที่ตั้ง: `LOAD = 16,000,000 / 1000 − 1 = 15999` → นับครบทุก 1 ms

### 7.2 UART: interrupt รับ / ส่ง (`uart.c`)
ใช้ **ring buffer** (array ที่วนเป็นวงกลม) เป็นคิวพักข้อมูล
`head` = ตำแหน่งที่จะเขียนต่อ, `tail` = ตำแหน่งที่จะอ่านต่อ, ถ้า `head == tail` คือคิวว่าง

**รับ (ผู้เล่นพิมพ์ `g`):**
1. USART2 ได้รับ 1 byte → บิต RXNE = 1 → เกิด interrupt
2. `USART2_IRQHandler()` อ่าน `USART2->DR` แล้วเก็บลง `u1g_rx_buf` (คิวรับ 64 byte)
3. เกมเรียก `UART_ReadChar(&ct_rx)` เมื่อพร้อม เพื่อหยิบตัวอักษรออกจากคิว

**ส่ง (ข้อความขึ้นจอ):**
1. `UART_SendString("...")` → `UART_SendChar` ใส่ทีละตัวลง `u1g_tx_buf` (คิวส่ง 1024 byte) แล้วเปิด `TXEIE`
2. ทุกครั้งที่ USART2 ส่งตัวก่อนหน้าเสร็จ (บิต TXE = 1) → interrupt → ISR ส่งตัวถัดไป
3. คิวว่าง → ISR ปิด `TXEIE` เอง
→ เกมไม่ต้องรอส่งข้อความ ใส่คิวแล้วทำงานต่อได้ทันที

### 7.3 ADC + DMA (`adc.c`)
- ADC1 ตั้งให้ **สแกน 3 ช่องวนไปเรื่อยๆ** (continuous scan)
- ทุกครั้งที่แปลงเสร็จ 1 ช่อง **DMA2 Stream0** คัดลอกค่าจาก `ADC1->DR` ไปใส่ array เอง โดยไม่ต้องใช้ CPU
- โหมด circular: ครบ 3 ช่องแล้ววนกลับไปช่องแรกของ array

| ช่องใน `u2g_adc_buf[]` | อุปกรณ์ | ขา | ADC channel |
|---|---|---|---|
| `[0]` | LDR (เซนเซอร์แสง) | PA1 | 1 |
| `[1]` | โพเทนฯ คันเร่ง (ภายนอก) | PC2 | 12 |
| `[2]` | โพเทนฯ บนบอร์ด | PA4 | 4 |

`ADC_Read(ช่อง)` แค่คืนค่าจาก array ที่ DMA เขียนไว้ล่าสุด จึง **ไม่ต้องมี ISR** และ CPU ไม่ต้องรอเลย

### 7.4 ปุ่ม: EXTI + TIM3 กันสัญญาณเด้ง (`exti.c`, `button.c`)
**ปัญหา:** ปุ่มกลไกตอนกด/ปล่อย หน้าสัมผัสจะ "เด้ง" ติดๆ ดับๆ หลายครั้งใน 1–10 ms
ถ้านับทุก edge กด 1 ครั้งจะนับได้หลายครั้ง

**ขั้นตอน:**
1. กดปุ่ม → ขาเปลี่ยนจาก 1 เป็น 0 (ปุ่มเป็น active-low มี pull-up) → **EXTI** เกิด interrupt
2. `EXTIx_IRQHandler()` ใน `exti.c` เรียก `Button_ExtiCallback()`
3. callback: ล้าง flag แล้ว **ปิด (mask) line ชั่วคราว** เพื่อไม่สนใจการเด้ง และสั่ง **TIM3** นับ 20 ms
4. ครบ 20 ms → `TIM3_IRQHandler()` → `button_settle()` อ่านขาอีกครั้งตอนนิ่งแล้ว
   - ถ้าเปลี่ยนเป็น "กด" จริง → `u4g_presses[ปุ่ม]++` (นับ 1 ครั้ง)
   - เปิด line กลับ
5. เกมเรียก `Button_TakePresses(BTN_STOP)` เพื่อหยิบจำนวนครั้งที่กดออกไป (แล้วรีเซ็ตเป็น 0)

| ปุ่ม | หน้าที่ในเกม | ขา | EXTI line → ISR |
|---|---|---|---|
| 1 | Deadman | PA10 | 10 → `EXTI15_10_IRQHandler` |
| 2 | เบรกฉุกเฉิน | PB3 | 3 → `EXTI3_IRQHandler` |
| 3 | STOP (จอดสถานี) | PB5 | 5 → `EXTI9_5_IRQHandler` |
| 4 | สับราง / ชาร์จไฟ | PB4 | 4 → `EXTI4_IRQHandler` |

ใน `Button_TakePresses` มี `__disable_irq()` / `__set_PRIMASK()` เพื่อปิด interrupt ชั่วพริบตาระหว่าง "อ่านแล้วรีเซ็ต"
กันไม่ให้ ISR นับเพิ่มแทรกตรงกลางจนการกดหายไป

### 7.5 Buzzer: TIM2 (`buzzer.c`)
- `Buzzer_Beep(n)` ตั้งจำนวนครั้งที่ต้องสลับขาเป็น `2n` แล้วเริ่ม TIM2 จากนั้น **คืนค่าทันที** เกมไม่ต้องรอเสียงจบ
- TIM2 เกิด interrupt ทุก 200 µs → `TIM2_IRQHandler()` สลับขา PC3 → ได้คลื่นสี่เหลี่ยม 2.5 kHz
- นับครบแล้ว ISR หยุด timer เอง

**การคำนวณ** (นาฬิกา 16 MHz)
- `PSC = 16,000,000 / 1,000,000 − 1 = 15` → timer นับที่ 1 MHz (1 µs ต่อครั้ง)
- `ARR = 1,000,000 / 5,000 − 1 = 199` → ครบทุก 200 µs = 5,000 ครั้ง/วินาที
- สลับขา 5,000 ครั้ง/วินาที = เสียง 2,500 Hz, `Buzzer_Beep(100)` = 100 รอบ = 40 ms

### 7.6 RC522: ขา IRQ ผ่าน EXTI8 (`rc522.c`)
1. `RC522_ReadUid()` เรียก `transceive()` เพื่อส่งคำสั่งถามบัตรผ่าน SPI (แบบ bit-bang)
2. ส่งเสร็จแล้ว CPU **หลับ** (`Timebase_Sleep()`)
3. RC522 ดึงขา IRQ (D15 / PB8) ลงเป็น 0 เมื่อ **บัตรตอบ** หรือเมื่อ **ตัวจับเวลาในชิปหมด** (≈15 ms = ไม่มีบัตร)
4. → EXTI8 → `EXTI9_5_IRQHandler()` → `RC522_ExtiCallback()` ตั้ง `bg_irq_flag = true`
5. CPU ตื่นขึ้นมา อ่านผลจาก RC522 ครั้งเดียว

มีเวลาสำรอง 30 ms ถ้าลืมต่อสาย IRQ จะยังอ่านบัตรได้ (ช้าลง) และ `RC522_IsIrqPinOk()` จะทำให้เกมขึ้นข้อความเตือนให้ต่อสาย

**Bit-bang SPI** (`spi_transfer()`): ใช้ GPIO สร้างสัญญาณ SPI เอง ทีละบิต 8 รอบต่อ 1 byte
เขียนบิตลง MOSI → SCK ขึ้น → อ่านบิตจาก MISO → SCK ลง
(ใช้ SPI hardware ไม่ได้เพราะขา SPI1 ชนกับ LED บน shield)

### 7.7 สรุป interrupt ทั้งหมด
| Interrupt | เกิดเมื่อ | ISR (ไฟล์) | ทำอะไร |
|---|---|---|---|
| SysTick | ทุก 1 ms | `SysTick_Handler` (timebase.c) | นับเวลา ปลุก CPU |
| USART2 | รับ 1 byte / ส่งพร้อมรับตัวถัดไป | `USART2_IRQHandler` (uart.c) | เก็บลงคิวรับ / ส่งตัวถัดไป |
| EXTI3, 4, 9_5, 15_10 | ขาปุ่มเปลี่ยน, ขา IRQ ของ RC522 ลง | `EXTIx_IRQHandler` (exti.c) | เริ่ม debounce / แจ้งว่าบัตรตอบ |
| TIM3 | ครบ 20 ms หลังปุ่มเปลี่ยน | `TIM3_IRQHandler` (button.c) | ยืนยันการกด นับครั้ง |
| TIM2 | ทุก 200 µs ระหว่างเสียงดัง | `TIM2_IRQHandler` (buzzer.c) | สลับขา buzzer |
| (DMA2 Stream0) | ADC แปลงเสร็จ | ไม่มี ISR | คัดลอกค่าเอง |

ทุก interrupt ใช้ priority เท่ากัน (ค่าเริ่มต้น) จึงไม่แทรกกันเอง ถ้าเกิดพร้อมกัน NVIC จะทำทีละตัว
และเพราะ ISR สั้นมาก จึงไม่มีตัวไหนต้องรอนาน

---

## 8. ตรรกะของเกม

### 8.1 เกม 1 รอบ: `TrainGame_RunJourney()`
```
reset_trip()            ปิดไฟ ล้างค่า
wait_for_card()         รอแตะบัตร RFID (ถามบัตรทุก 50 ms)
LED_BlinkAll()          ไฟ 4 ดวงกระพริบ
print_mission()         บอกภารกิจ: ต้องถึงขอนแก่นก่อน 12:02:30
start_trip()            รอพิมพ์ 'g' -> นาฬิกาเริ่ม 12:00:00
drive_to_station("สถานีรับผู้โดยสาร")  -> ถ้าจอด: passenger_minigame()
drive_to_station("สถานีชาร์จไฟ")       -> ถ้าจอด: charge_minigame()
drive_to_junction("จุดแยกทาง")         -> ต้องอยู่เส้นทาง 2 ไม่งั้นแพ้
drive_to_station("สถานีขอนแก่น", เขตชุมชน 600 m) -> ไม่จอด = ชน, ถึงช้า = แพ้
print_summary()         สรุปผล เวลาที่ใช้ คำเตือนความเร็ว
```

### 8.2 ขับ 1 ช่วง: `drive_leg()` (วน 1 รอบ = 1 วินาที)
1. แสดงตัวนับ deadman บน 7-segment
2. `wait_drive_tick()` หลับรอ 1 วินาที (อัปเดตไฟหน้าไปด้วย) แล้วเก็บจำนวนการกดปุ่ม
   ถ้ากด **เบรกฉุกเฉิน** จะกลับมาทันที → `run_alarm("EMERGENCY")` รอพิมพ์ `r` → ขับต่อจากจุดเดิม
3. อ่านคันเร่ง `ADC_Read(THROTTLE_ADC_CH)` → **ความเร็ว = 60 + ค่าโพเทนฯ × 240 / 4095** (60–300 km/h)
4. เหลือ ≤ 200 m → ไฟน้ำเงินติดและถาม ถ้ากด **STOP** ทัน = จะจอด
5. จุดแยก: กดปุ่ม 4 จำนวนคี่ = สับราง
6. **Deadman**: ไม่มีการกดปุ่มและคันเร่งขยับน้อยกว่า 60 → นับถอยหลัง ถ้าต่ำกว่า 0 = หลับใน → alarm
7. **ระยะทาง −= ความเร็ว × 1000 / 3600** (เมตรต่อวินาที)
8. เขตชุมชน: เข้าเขตแล้ว 5 วินาที กล้องเริ่มจับ ถ้าเกิน 120 km/h จะบันทึกไว้เตือนตอนสรุป
9. `print_status()` พิมพ์เวลา ความเร็ว ระยะทาง ไฟหน้า

ระยะเหลือ 0 → จบช่วง คืนค่า "จอด / ขับผ่าน" หรือ "อยู่ทางไหน"

### 8.3 ฟังก์ชันอื่นใน `train_game.c`
| ฟังก์ชัน | ทำอะไร |
|---|---|
| `key_received` / `wait_for_key` | เช็คว่าพิมพ์ตัวที่ต้องการหรือยัง / หลับรอจนพิมพ์ |
| `trip_clock_s`, `send_clock`, `send_time_tag` | นาฬิกาในเกม (เริ่ม 12:00:00) และพิมพ์ `[hh:mm:ss]` |
| `send_headlight` | พิมพ์สถานะไฟหน้า |
| `gauge_show` | เกจไฟ 4 ระดับ เขียว → เหลือง → แดง → น้ำเงิน |
| `run_alarm`, `resume_driving` | ไฟแดง + เสียงเตือนจนพิมพ์ `r` / คืนไฟตอนขับต่อ |
| `wait_depart` | จบกิจกรรมสถานี รอพิมพ์ `g` เพื่อออกรถ |
| `passenger_minigame` | ปุ่ม 1–4 เปิดประตูตู้ 1–4 รอ 5 วินาที ไฟกระพริบแล้วกดปิด |
| `charge_minigame` | กดปุ่ม 4 ครบ 50 ครั้ง เกจเต็ม |
| `print_summary` | สรุปผลแพ้/ชนะ เวลาที่ใช้ การฝ่าความเร็ว |

---

## 9. อยากปรับค่า แก้ตรงไหน

แก้ค่า `#define` แล้ว Build ใหม่ (รูปค้อน หรือ `Ctrl` + `B`) และ Run

| อยากเปลี่ยน | ชื่อค่า | ไฟล์ | ค่าปัจจุบัน |
|---|---|---|---|
| ระยะทางแต่ละช่วง | `SEG_DISTANCE_M` | train_game.c | 1500 m |
| ระยะที่เริ่มถามก่อนถึงสถานี | `APPROACH_M` | train_game.c | 200 m |
| ความเร็วต่ำสุด / สูงสุด | `SPEED_MIN_KMH` / `SPEED_MAX_KMH` | train_game.c | 60 / 300 |
| คันเร่งใช้โพเทนฯ ตัวไหน | `THROTTLE_ADC_CH` | train_game.c | `ADC_CH_POT_EXT` (เปลี่ยนเป็น `ADC_CH_POT_BOARD` = ตัวบนบอร์ด) |
| เปิด/ปิด deadman, เลขเริ่มนับ | `DEADMAN_ENABLED`, `DEADMAN_COUNT_START` | train_game.c | 1, 9 |
| เขตชุมชน: ระยะ / ความเร็ว / ช่วงผ่อนผัน | `ZONE_M`, `ZONE_LIMIT_KMH`, `ZONE_GRACE_MS` | train_game.c | 600 m, 120, 5000 ms |
| เวลาที่ต้องถึง | `DEADLINE_AFTER_START_S` | train_game.c | 150 s (12:02:30) |
| จำนวนกดชาร์จ / เวลาขึ้นรถ | `CHARGE_PRESSES`, `DOOR_LOAD_MS` | train_game.c | 50, 5000 ms |
| ความยาวเสียงแต่ละแบบ | `BEEP_xxx` | train_game.c | – |
| ความมืดที่ไฟหน้าติด | `HEADLIGHT_ON_THRESHOLD` | headlight.c | 1500 |
| เวลากันเด้งของปุ่ม | `BUTTON_DEBOUNCE_MS` | button.c | 20 ms |
| ความถี่เสียง buzzer | `BUZZER_TOGGLE_HZ` (= 2 × ความถี่เสียง) | buzzer.c | 5000 |
| ย้ายขาอุปกรณ์ | `xxx_PORT`, `xxx_PIN` | board.h | – |

---

## 10. คำถามที่อาจโดนถามตอนพรีเซนต์

**Q: Polling คืออะไร ทำไมไม่ใช้?**
A: Polling คือการให้ CPU วนเช็ค flag ของฮาร์ดแวร์ซ้ำๆ เช่น `while (!(USART2->SR & RXNE));` CPU เสียเวลาทั้งหมดไปกับการรอ
และถ้าเช็คไม่ทันข้อมูลอาจหาย โปรเจกต์นี้ใช้ interrupt / DMA แทน ฮาร์ดแวร์แจ้งเองเมื่อมีเหตุการณ์
ระหว่างรอ CPU หลับด้วย `__WFI()`

**Q: UART ใช้ interrupt ยังไง?**
A: interrupt RXNE เก็บตัวอักษรที่รับลงคิว, interrupt TXE ส่งข้อความจากคิวทีละตัว
ทั้งหมดอยู่ใน `USART2_IRQHandler` เกมแค่ใส่และหยิบข้อมูลจากคิว (ข้อ 7.2)

**Q: ADC ใช้ DMA ยังไง ทำไมไม่มี ISR?**
A: ADC สแกน 3 ช่องต่อเนื่อง DMA คัดลอกผลลง array ให้เองทุกครั้งที่แปลงเสร็จ แบบวนซ้ำ (circular)
ค่าล่าสุดจึงอยู่ใน array ตลอด ไม่ต้องมี ISR มาย้ายค่า (ข้อ 7.3)

**Q: ใช้ EXTI กี่จุด?**
A: 5 จุด: ปุ่ม 4 ปุ่ม (line 3, 4, 5, 10) และขา IRQ ของ RC522 (line 8)

**Q: แก้ปัญหาปุ่มเด้งยังไง?**
A: EXTI จับ edge แรกแล้วปิด line ชั่วคราว ให้ TIM3 นับ 20 ms แล้วอ่านขาตอนนิ่ง ค่อยนับการกด (ข้อ 7.4)

**Q: Timer ใช้ทำอะไร คำนวณยังไง?**
A: TIM2 สร้างเสียง buzzer 2.5 kHz (PSC 15, ARR 199 → 200 µs) และ TIM3 กันปุ่มเด้ง 20 ms
(PSC 15999 → 1 kHz, ARR 19 → 20 ms) สูตร: เวลา = (PSC+1) × (ARR+1) / 16 MHz

**Q: ถ้า interrupt เกิดพร้อมกันล่ะ?**
A: ทุกตัว priority เท่ากัน จึงไม่แทรกกัน NVIC จะทำทีละตัว ISR ทุกตัวสั้นมาก (แค่ตั้ง flag หรือเก็บค่า) จึงไม่มีตัวไหนต้องรอนาน

**Q: ตัวแปรที่ใช้ร่วมกันระหว่าง ISR กับโปรแกรมหลัก ป้องกันยังไง?**
A: ประกาศเป็น `volatile` และตอนอ่านแล้วรีเซ็ตตัวนับ (`Button_TakePresses`) จะปิด interrupt ชั่วขณะ
คิว UART ใช้หลัก "ฝั่งหนึ่งเขียน head อีกฝั่งเขียน tail" จึงไม่ชนกัน

**Q: ทำไม RFID ไม่ใช้ SPI hardware?**
A: ขา SPI1 (PA5–PA7) ถูก LED บน shield ใช้อยู่ จึงทำ SPI ด้วย GPIO (bit-bang) บนขา analog header แทน
แต่การรอคำตอบจากบัตรใช้ interrupt ผ่านขา IRQ (EXTI8)

**Q: ทำไมไม่ใช้ Watchdog?**
A: เกมมีช่วงรอผู้เล่นนาน (รอพิมพ์ `g` หรือแตะบัตร) ถ้าเปิด watchdog บอร์ดจะรีเซ็ตระหว่างรอ
ข้อ 5 ของข้อกำหนดจึงใช้ Timer (TIM2, TIM3) แทน

---

## 11. คำศัพท์

| คำ | ความหมาย |
|---|---|
| Register (รีจิสเตอร์) | ช่องหน่วยความจำพิเศษที่ใช้สั่งงาน/อ่านสถานะ peripheral |
| Peripheral | อุปกรณ์ในชิป เช่น UART, ADC, Timer, DMA |
| RCC | ตัวจ่ายสัญญาณนาฬิกา ต้องเปิดก่อนใช้ peripheral ทุกตัว |
| GPIO | ขาอเนกประสงค์ ตั้งเป็น input / output / analog / alternate function |
| Interrupt / IRQ | สัญญาณจากฮาร์ดแวร์ที่ขัดจังหวะ CPU ให้ไปทำ ISR |
| ISR / IRQHandler | ฟังก์ชันที่ทำงานเมื่อเกิด interrupt |
| NVIC | ตัวจัดการ interrupt ของ CPU (เปิด/ปิด/จัดลำดับ) |
| EXTI | External Interrupt: interrupt จากการเปลี่ยนระดับของขา GPIO |
| DMA | Direct Memory Access: ย้ายข้อมูลเองโดยไม่ใช้ CPU |
| ADC | แปลงแรงดันไฟ (0–3.3 V) เป็นตัวเลข 0–4095 |
| UART | การสื่อสาร serial (ต่อกับคอมผ่าน ST-LINK) |
| Polling | วนเช็คสถานะซ้ำๆ (โปรเจกต์นี้ไม่ใช้) |
| Debounce | กันสัญญาณเด้งของปุ่ม |
| Ring buffer | คิวแบบวงกลม ใช้พักข้อมูลระหว่าง ISR กับโปรแกรมหลัก |
| Flag | ตัวแปร/บิตที่บอกว่า "มีเหตุการณ์เกิดขึ้นแล้ว" |
| PSC / ARR | ตัวหารนาฬิกา / ค่าที่ timer นับถึงแล้วเริ่มใหม่ |
| WFI | Wait For Interrupt: คำสั่งให้ CPU หลับจนกว่ามี interrupt |
| Bit-bang | สร้างสัญญาณโปรโตคอลเองด้วย GPIO แทนฮาร์ดแวร์ |
| CMSIS | ไฟล์ header มาตรฐานของ ARM/ST ที่ให้ชื่อรีจิสเตอร์ (ไม่ใช่ HAL) |
| HAL | ไลบรารีสำเร็จรูปของ ST (โปรเจกต์นี้ไม่ใช้) |
