#ifndef __KERN_DRIVER_VGA_TEXT_H__
#define __KERN_DRIVER_VGA_TEXT_H__

#include <types.h>

void vga_text_init();
uint32_t vga_text_get_width();
uint32_t vga_text_get_height();
bool vga_text_set_char();
bool vga_text_get_char();

bool vga_text_set_cursor_position(uint32_t x, uint32_t y);
bool vga_text_get_cursor_position(uint32_t *x_store, uint32_t *y_store);

#endif
