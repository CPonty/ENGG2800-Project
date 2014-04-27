//
// control.h
//

#ifndef CONTROL_H
#define CONTROL_H

/* Chip communication */
void do_send_interrupt (uint8_t pin);
void task_stop(void);

volatile char mode;

#define PIN_TRIGGER (1<<PD5)
#define PIN_RUN (1<<PD4)
#define PIN_NEXT (1<<PD3)
#define PIN_RESET (1<<PD2)

#endif
