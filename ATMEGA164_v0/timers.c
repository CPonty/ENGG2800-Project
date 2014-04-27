#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
#include "serial.h"
#include "control.h"
#include "lcd.h"
#include "analogue.h"
//#include "timers.h"

//	Timer functions & interrupt library
//
//  Features:
//		wait_us and wait_ms
//		timer selection(setup)
//			adc 8bit, screen refresh 16bit
//		

#define DELAY_SETUP_CLOCKS 0

void wait_us (uint16_t period);
void wait_ms (uint16_t period);

void timer0_adc_period (uint8_t period);
void timer0_adc_setup (void);
void timer0_adc_pause (void);
void timer0_adc_resume (void);

void timer1_lcd_period (uint16_t period);
void timer1_lcd_setup (void);
void timer1_lcd_pause (void);
void timer1_lcd_resume (void);
void timer1_lcd_off (void);

volatile uint8_t interrupts_set = 0;
volatile uint8_t timer0_paused = 1;
volatile uint8_t timer1_paused = 1;

// ------------------------------------------------------ 

/* Delay for at least [period] microseconds.
   Not perfectly accurate.
   Same-timer interrupts will be missed
*/
void wait_us (uint16_t period) //0 < period < 65535/20
{
	//HACK REPLACE: _delay_us
	_delay_us(period);
	return;
	
	/* Keep current values */
	uint8_t tmp_TIFR1 = TIFR1;
	uint8_t tmp_TCCR1B = TCCR1B;
	uint16_t tmp_TCNT1 = TCNT1;
	uint16_t tmp_OCR1A = OCR1A;
	/* Clocks needed for function body */

	/* Disable interrupt */
	if (interrupts_set) cli();
	
	/* Set time */
	OCR1A = period*20 - DELAY_SETUP_CLOCKS;
	/* Reset current count */
	TCNT1 = 0;
	/* Force the output compare flag to clear */
	TIFR1 |= (1<<OCF1A);
	/* Run the timer - normal mode, clkdiv/1 */
	TCCR1B |= (1<<WGM12)|(1<<CS10);

	/* Wait until CTC */
	while ((TIFR1 & (1 << OCF1A)) == 0);

	/* Stop the timer */
	TCCR1B = 0;
	/* Restore state */
	TIFR1 = tmp_TIFR1;
	TCCR1B = tmp_TCCR1B;
	TCNT1 = tmp_TCNT1;
	OCR1A = tmp_OCR1A;
	
	/* Re-enable interrupts */
	if (interrupts_set) sei();
}

/* Delay for [period] milliseconds
   Uses delay_us repeatedly
*/
void wait_ms (uint16_t period) //0 < period < 65535/20
{
	//HACK REPLACE: _delay_ms
	_delay_ms(period);
	return;
	
	/* disregard for-loop time. insiginificant vs. 
	   20,000 clocks per wait  */
	uint16_t repeats;
	for (repeats = 0; repeats < period*20; repeats++) {
		wait_us(1000);
	}
}

// ------------------------------------------------------

/* 
	8bit timer0 (ADC) functions 
*/
void timer0_adc_period (uint8_t period)//in clocks/64
{
	//remember, timer is clkdiv 64! uses cs01,00
	OCR0A = period;
}

void timer0_adc_setup (void)
{
	// reset timer-counter when triggered
	TCCR0A = (1<<COM0A1);
	// use CTC; internal-clock; clkdiv 64
	TCCR0B = (1<<WGM12);//(1<<CS00)|(1<<CS01)
	// use timer compare counter A
	TIMSK0 = (1<<OCIE0A); 
	timer0_paused = 1;
}

void timer0_adc_pause (void)
{	
	// halt counters
	TCCR0B = (1<<WGM12); 
	timer0_paused = 1;
}

void timer0_adc_resume (void)
{	
	// enable counter, clocked @ div64
	TCCR0B = (1<<WGM12)|(1<<CS01)|(1<<CS00);
	timer0_paused = 0;
}

void timer0_adc_off (void)
{	
	// timer shutdown & clear
	TCCR0A = 0;
	TCCR0B = 0;
	TIMSK0 = 0;
	OCR0A = 0;
	TCNT0 = 0;
	TIFR0 |= (1<<OCF1A);
	timer1_paused = 1;
}

ISR(TIMER0_COMPA_vect)
{ 
	// force ADC read-in
	adc_fetch(); 
}

// ------------------------------------------------------ 

void timer1_lcd_period (uint16_t period)//in us
{
	//remember, timer is clkdiv1024!
	OCR1A = period*20;
}

void timer1_lcd_setup (void)
{
	// reset timer-counter when triggered
	TCCR1A = (1<<COM1A1);
	// use CTC; internal-clock; clkdiv 1024
	TCCR1B = (1<<WGM12);//(1<<CS12)|(1<<CS10)
	// use timer compare counter A
	TIMSK1 = (1<<OCIE1A); 
	timer1_paused = 1;
}

void timer1_lcd_pause (void)
{	
	// halt counters
	TCCR1B = (1<<WGM12); 
	timer1_paused = 1;
}

void timer1_lcd_resume (void)
{	
	// enable counter, clocked @ div1024
	TCCR1B = (1<<WGM12)|(1<<CS12)|(1<<CS10);
	timer1_paused = 0;
}

void timer1_lcd_off (void)
{	
	// timer shutdown & clear
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = 0;
	OCR1A = 0;
	TCNT1 = 0;
	TIFR1 |= (1<<OCF1A);
	timer1_paused = 1;
}

/*  LCD refresh interrupt */
ISR(TIMER1_COMPA_vect)
{
	/* DISABLED */
	//uncomment to draw DVM screen on adc interrupt
	//lcd_dvm();
	/* DISABLED */
}
