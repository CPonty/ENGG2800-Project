#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
#include "serial.h"
#include "lcd.h"
#include "analogue.h"
#include "timers.h"
//#include "control.h"

//
//	Main control library
//	Features:
//		Chip communication via INTn
//		Global data
//		Task management
//		Packaged actions
//		

void do_send_interrupt (uint8_t pin);

volatile char mode = 'S';

/* ------------------------------------------------------ */

#define PIN_TRIGGER (1<<PD5)
#define PIN_RUN (1<<PD4)
#define PIN_NEXT (1<<PD3)
#define PIN_RESET (1<<PD2)

/* Signal send */
void do_send_interrupt (uint8_t pin)
{
	uint16_t i;
	if (interrupts_set) cli();
	
	//set pins
	PORTD |= PIN_TRIGGER | pin;
	//wait
	for (i=0; i<(25*20); i++) asm("nop");
	//switch off
	PORTD &= ~(PIN_TRIGGER | pin);
	
	if (interrupts_set) sei();
}

void task_stop(void)
{
	/* Cancel all feasible tasks */
	lcd_wipe_all();
	do_send_interrupt(PIN_RESET);
	adc_pause();

	timer1_lcd_pause();
	timer0_adc_pause();
	adc_memset();
	dvm_memset();
	dvm_run = 0;
	adc_run = 0;
	_delay_us(10);
}

