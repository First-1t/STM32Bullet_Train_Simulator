# STM32Bullet_Train_Simulator

เกมจำลองการขับรถไฟความเร็วสูงบน **NUCLEO-F411RE + NEXTY Training Shield 1**
เขียนแบบ register-level ด้วย CMSIS ล้วน (ไม่ใช้ HAL) ควบคุมผ่าน serial terminal (115200 8N1)
ทำงานด้วย interrupt / DMA ทั้งหมด **ไม่มี polling**

📖 **ไม่ถนัดภาษา C? เริ่มอ่านที่ [CODE_GUIDE.md](CODE_GUIDE.md)** — คู่มืออ่านโค้ดทีละขั้น

## ภาพรวมเกม
1. **START** แตะบัตร RFID → ไฟ 4 ดวงกระพริบ → แจ้งภารกิจ → พิมพ์ `g` ออกรถ (นาฬิกาเริ่ม 12:00:00)
2. **สถานีรับผู้โดยสาร** ไฟน้ำเงินติดเมื่อเหลือ 200 m กด STOP ทัน = จอด
   → มินิเกมเปิด/ปิดประตู 4 ตู้ (ปุ่ม 1–4) → พิมพ์ `g`
3. **สถานีชาร์จไฟ** จอดแล้วกดปุ่ม 4 รัว 50 ครั้งให้เกจเต็ม → พิมพ์ `g`
4. **จุดแยกทาง** ต้องกดปุ่ม 4 สับรางไปเส้นทาง 2 (ขอนแก่น) ก่อนถึง ไม่งั้นแพ้
5. **สถานีขอนแก่น** ต้องจอดให้ทันและถึงก่อน 12:02:30
   600 m สุดท้ายเป็นเขตชุมชน จำกัด 120 km/h (กล้องตรวจหลังเข้าเขต 5 วินาที)

- ความเร็ว 60–300 km/h ตามโพเทนชิโอมิเตอร์ (คันเร่ง)
- **Deadman**: 7-segment นับ 9 → 0 ต้องขยับคันเร่งหรือกดปุ่ม ไม่งั้นถือว่าหลับใน
- **Emergency** (ปุ่ม 2) ไฟแดง + buzzer พิมพ์ `r` แล้วขับต่อจากจุดเดิม
- ไฟหน้า (LED เหลือง) เปิด/ปิดอัตโนมัติตามเซนเซอร์แสง

## Peripheral และ Interrupt
| ข้อกำหนด | ใช้ในโปรเจกต์ |
|---|---|
| GPIO | ปุ่ม 4, LED 4, 7-segment, buzzer, ขา RC522 |
| UART (interrupt) | USART2 interrupt RXNE + TXE พร้อม ring buffer |
| ADC (DMA) | ADC1 สแกน 3 ช่องต่อเนื่อง + DMA2 Stream0 แบบ circular |
| EXTI | ปุ่ม 4 ปุ่ม (line 3, 4, 5, 10) + ขา IRQ ของ RC522 (line 8) |
| Timer | TIM2 = เสียง buzzer, TIM3 = กันปุ่มเด้ง 20 ms, SysTick = เวลา 1 ms |

ระหว่างรอ CPU จะหลับด้วย `__WFI()` แล้วให้ interrupt มาปลุก

## โครงสร้างไฟล์
| โมดูล | หน้าที่ |
|---|---|
| `board` | header ของชิป, pin map ทั้งหมด, ฟังก์ชัน GPIO, `SystemInit` |
| `timebase` | SysTick interrupt 1 ms, sleep / delay |
| `exti` | ตั้งค่า EXTI และ ISR ของ EXTI ทุกตัว (ส่งต่อให้ button / rc522) |
| `uart` / `adc` / `led` / `button` / `seg7` / `buzzer` / `rc522` | ไดรเวอร์ฮาร์ดแวร์ |
| `headlight` | ไฟหน้าอัตโนมัติ |
| `train_game` | ตรรกะเกม (state machine, มินิเกม, สรุปผล) |
| `main.c` | เรียก `TrainGame_Init()` แล้ววน `TrainGame_RunJourney()` |

โค้ดทำตาม coding guideline ของรายวิชา (ห้าม `//`, ห้าม ternary, ทุก `if` มี `else`,
ไม่มี magic number ฯลฯ) และใช้รูปแบบไฟล์ตาม Template.c / Template.h

## การต่อวงจร
| อุปกรณ์ | ขา |
|---|---|
| LED เขียว / เหลือง (ไฟหน้า) / แดง / น้ำเงิน | PB6 / PA7 / PA6 / PA5 |
| ปุ่ม 1 (Deadman) / 2 (Emergency) / 3 (STOP) / 4 (สับราง, ชาร์จ) | PA10 / PB3 / PB5 / PB4 |
| 7-segment (BCD) | PC7, PA8, PB10, PA9 |
| LDR (เซนเซอร์แสง) | PA1 (A1) |
| โพเทนฯคันเร่งภายนอก (ขากลาง) | PC2 |
| Buzzer (+) | PC3 |
| RC522: SDA / SCK / MOSI / MISO | A0 / A3 / A4 / A5 (RST และ 3.3V → 3V3) |
| RC522: IRQ | D15 (PB8) |
| UART (ST-LINK virtual COM) | PA2 / PA3 |

⚠️ RC522 และโพเทนฯใช้ไฟ **3.3V เท่านั้น**

## การ build
เปิดด้วย **STM32CubeIDE** (File → Import → Existing Projects into Workspace)

ไฟล์ CMSIS ไม่ได้อยู่ใน repo นี้ โปรเจกต์ต้องการ include path ไปที่
`Library/CMSIS/Core/Include` และ `Library/CMSIS-DEVICE-F4/Include`
(ตั้งไว้ใน Project Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths)
ถ้าวางโปรเจกต์ไว้คนละที่ ให้แก้ path สองตัวนี้ให้ตรงกับเครื่องตัวเอง
