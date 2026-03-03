
# ESP32 DevKit V2 Hardware Configuration

## Overview
This document outlines the hardware configuration for the ESP32 DevKit V2 microcontroller.

## Specifications
- **Microcontroller**: ESP32
- **Board**: DevKit V2
- **Processor**: Dual-core Xtensa 32-bit CPU
- **RAM**: 520 KB SRAM
- **Flash**: 4 MB (typically)


## LED Status Indicator
- **LED**: Status LED on board
- **GPIO**: GPIO2
- **Function**: Indicates board status during various events


## Relay Configuration

| Relay | GPIO | Logic |
|-------|------|-------|
| Relay 1 | GPIO4 | Active Low |
| Relay 2 | GPIO16 | Active Low |
| Relay 3 | GPIO17 | Active Low |


## Switch Configuration

| Switch | GPIO | Pull-up Resistor | Logic |
|--------|------|------------------|-------|
| SW1 | GPIO34 | R10k | Active Low |
| SW2 | GPIO35 | R10k | Active Low |
| SW3 | GPIO32 | R10k | Active Low |


## Isolated Input Configuration

| Input | GPIO | Voltage Range | Logic |
|-------|------|---------------|-------|
| ISO1 | GPIO33 | 10-24V TTL | Active Low |
| ISO2 | GPIO27 | 10-24V TTL | Active Low |


## OLED (I2C) Configuration

### สรุป
- ประเภทที่พบบ่อย: OLED 0.96" / 1.3" driven by SSD1306
- ความละเอียดตัวอย่าง: 128x64 (หรือ 128x32)
- I2C Address ทั่วไป: 0x3C (บางรุ่น 0x3D)
- แรงดัน: 3.3V (ห้ามจ่าย 5V ถ้าจอไม่รองรับ)

### การต่อสาย (ตัวอย่าง)
| OLED Pin | ESP32 DevKit V2 |
|----------|------------------|
| VCC      | 3.3V             |
| GND      | GND              |
| SDA      | GPIO21 (I2C SDA) |
| SCL      | GPIO22 (I2C SCL) |
| RST      | GPIO5 (optional) |


## RS485 (Serial0) — MAX13487, Auto Direction และสวิตช์ RS232/RS485

### สรุปการต่อ
- ใช้ UART0 (Serial0) ของ ESP32 (TX0 = GPIO1, RX0 = GPIO3) ต่อกับ MAX13487 เพื่อเชื่อมบัส RS485
- MAX13487 ทำงานแบบ half‑duplex (A/B) และควบคุมทิศทางด้วย DE/RE (Auto‑Direction โดยให้ MCU/RTS ควบคุม)
- บนบอร์ดมีสวิตช์แบบ SPDT สลับโหมด RS232 <-> RS485 (สลับการเชื่อมต่อ TX/RX ไปยังตัวแปลงที่เหมาะสม)

### ตัวอย่างการต่อสาย
| ESP32 (UART0) | MAX13487 |
|---------------|----------|
| TX0 (GPIO1)   | DI       |
| RX0 (GPIO3)   | RO       |
| control GPIO / UART RTS | DE (และควบคุม /RE) |
| GND           | GND      |
| MAX13487 A    | บัส A    |
| MAX13487 B    | บัส B    |
| VCC           | 3.3V     |














