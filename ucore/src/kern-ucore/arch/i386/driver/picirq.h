#ifndef __KERN_DRIVER_PICIRQ_H__
#define __KERN_DRIVER_PICIRQ_H__

void pic_init(void);
void pic_enable(unsigned int irq);
void pic_send_end_of_interrupt(unsigned int irq);

#define IRQ_OFFSET      32

#endif /* !__KERN_DRIVER_PICIRQ_H__ */
