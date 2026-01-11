## Clock Tree Configuration (AT32F403AVGT7)

Clock configuration is set for maximum performance (240MHz):

| Parameter | Value | Rationale |
| :--- | :--- | :--- |
| **Source** | **HSE** (8MHz) | External 8MHz crystal (NanoVNA standard). |
| **HSE State** | HSEON (Crystal) | MCU oscillator used (not external generator). |
| **PLL Source** | HSE / PREDIV(1) | PLL Input = 8MHz. |
| **PLL Mul** | **x30** | 8MHz * 30 = **240MHz** (Max SYSCLK). |
| **Flash Latency** | **7 WS** | Required for SYSCLK > 200MHz. |
| **AHB Prescaler** | DIV1 | HCLK = 240MHz. |
| **APB1 Prescaler** | **DIV2** | PCLK1 = 120MHz (Max allowed: 120MHz). |
| **APB2 Prescaler** | DIV1 | PCLK2 = 240MHz (Max allowed: 240MHz). |
