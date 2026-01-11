## Clock Tree Configuration

Clock configuration is set for maximum performance (72MHz):

| Parameter | Value | Rationale |
| :--- | :--- | :--- |
| **Source** | **HSE** (8MHz) | External 8MHz crystal (NanoVNA standard). |
| **HSE State** | HSEON (Crystal) | MCU oscillator used (not external generator). |
| **PLL Source** | HSE / PREDIV(1) | PLL Input = 8MHz. |
| **PLL Mul** | **x9** | 8MHz * 9 = **72MHz** (Max SYSCLK). |
| **Flash Latency** | **2 WS** | Required for 48MHz < SYSCLK <= 72MHz. |
| **AHB Prescaler** | DIV1 | HCLK = 72MHz. |
| **APB1 Prescaler** | **DIV2** | PCLK1 = 36MHz (Max allowed: 36MHz). |
| **APB2 Prescaler** | DIV1 | PCLK2 = 72MHz (Max allowed: 72MHz). |
