//
// timers.h
//
//

#ifndef TIMERS_H
#define TIMERS_H

/* delay for [period] milli/micro seconds 
** interrupt-safe */
void wait_us (uint16_t period);
void wait_ms (uint16_t period);

/* adc timer */
void timer0_adc_period (uint8_t period);
void timer0_adc_setup (void);
void timer0_adc_pause (void);
void timer0_adc_resume (void);

/* lcd refresh timer */
void timer1_lcd_period (uint16_t period);
void timer1_lcd_setup (void);
void timer1_lcd_pause (void);
void timer1_lcd_resume (void);
void timer1_lcd_off (void);

/* system state */
volatile uint8_t interrupts_set;
volatile uint8_t timer0_paused;
volatile uint8_t timer1_paused;

#endif
