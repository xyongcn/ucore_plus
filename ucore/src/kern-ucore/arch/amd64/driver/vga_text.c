#include <arch.h>
#include <assert.h>

#include "vga_text.h"

#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25

static uint32_t vga_text_cursor_x;
static uint32_t vga_text_cursor_y;
extern uint16_t addr_6845;

uint32_t vga_text_get_width()
{
  return VGA_TEXT_WIDTH;
}

bool vga_text_get_cursor_position(uint32_t *x_store, uint32_t *y_store)
{
  *x_store = vga_text_cursor_x;
  *y_store = vga_text_cursor_y;
  return 1;
}

bool vga_text_set_cursor_position(uint32_t x, uint32_t y)
{
  if(x >= VGA_TEXT_WIDTH || y >= VGA_TEXT_HEIGHT) {
    panic("vga_text_set_cursor_position");
    return 0;
  }
  vga_text_cursor_x = x;
  vga_text_cursor_y = y;
  uint16_t linear_pos = (uint16_t)(y * vga_text_get_width() + x);
  outb(addr_6845, 14);
  outb(addr_6845 + 1, linear_pos >> 8);
  outb(addr_6845, 15);
  outb(addr_6845 + 1, linear_pos);
  return 1;
}
