#ifndef DRV_LCD_H
#define DRV_LCD_H

#include "measlib/types.h"
#include <stdint.h>

void *meas_drv_lcd_init(void);
void meas_drv_lcd_blit(void *handle, meas_rect_t rect, const void *pixels);

#endif // DRV_LCD_H
