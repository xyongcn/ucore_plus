#include <arch.h>
#include <intr.h>
#include <asm/mipsregs.h>

#define get_status(x) __asm volatile("mfc0 %0,$12" : "=r" (x))
#define set_status(x) __asm volatile("mtc0 %0,$12" :: "r" (x))

static int saved_count;
bool warning_flag = 0;

/* intr_enable - enable irq interrupt */
void intr_enable(void)
{
  /*serial_putc('^');
  serial_putc('E');
  serial_putc('\n');*/
  uint32_t x;
	get_status(x);
  if(x & ST0_IE) {
    return;
  }
  unsigned int passed = read_c0_count() - saved_count;
  /*if(passed > 10000) {
    serial_putc('^');
    serial_putc('E');
    serial_putc('\n');
    warning_flag = 1;
  }*/
	x |= ST0_IE;
	set_status(x);
}

/* intr_disable - disable irq interrupt */
void intr_disable(void)
{
  /*if(warning_flag) {
    serial_putc('^');
    serial_putc('D');
    serial_putc('\n');
    warning_flag = 0;
  }*/
	uint32_t x;
	get_status(x);
  if(!(x & ST0_IE)) {
    return;
  }
  saved_count = read_c0_count();
	x &= ~ST0_IE;
	set_status(x);
}
