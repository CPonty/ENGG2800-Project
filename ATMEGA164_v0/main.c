#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
#include "lcd.h"
#include "serial.h"
#include "analogue.h"
#include "timers.h"
#include "control.h"

#define DEBUG 0

//
// ENGG2800 Project - Main File
//
//

int main(void) 
{

	/* Setup DDR */
	DDRA |= 0b00111111;
	DDRB = 0b00000000;
	DDRC = 0b11111111;
	DDRD |= 0b00111111;
	
	/* Hardware setup */
	//timer0_adc_period(26240);
	timer0_adc_setup();
	timer1_lcd_setup();
	//setup_adc(0);
	setup_serial();
	lcd_init();
	lcd_wipe_all();
	
	/* INCOMPLETE */
	// run hardcoded scenarios to test finished features
	// this accounts for the broken serial on-chip
	/* INCOMPLETE */
  
	/* Splash screen */
	//task_set('M');
	lcd_splash();
	#if (!DEBUG)
	_delay_ms(2000);
	#endif
	lcd_wipe_all();
	lcd_splash_animate();
	#if (!DEBUG)
	_delay_ms(100);
	#endif

	/* Menu screen */
	lcd_wipe_all();
	lcd_menu();
	#if (!DEBUG)
	_delay_ms(2000);
	#endif

	/* DVM Screen */
	lcd_wipe_all();
	lcd_dvm_back();
	_delay_ms(1000);
	lcd_dvm_data(3475);
	_delay_ms(300);
	lcd_dvm_data(2293);
	_delay_ms(300);
	lcd_dvm_data(3011);
	_delay_ms(300);
	lcd_dvm_data(3234);
	_delay_ms(300);
	lcd_dvm_data(3099);
	_delay_ms(300);
	lcd_dvm_data(3375);
	_delay_ms(1000);

	/* Graphing Screen */
	lcd_wipe_all();
	lcd_graph_background();
	_delay_ms(2000);

	/* Sweep Mode - ADC */
	do_send_interrupt(PIN_RUN);
	setup_adc(MIC_PIN);
	adc_resume();
	mode = 'S';
	
	adc_range[0] = 30*4;
	adc_range[1] = 0;
	adc_range[2] = 12*4;
	adc_range[3] = 27*4;
	adc_range[4] = 1*4;
	adc_range[5] = 15*4;
	adc_range[6] = 25*4;
	adc_range[7] = 13*4;
	
	adc_rescale();
	lcd_graph(adc_bars);

	/* Enable global interrupts to handle the rest */
	interrupts_set = 1;
	sei();

	#if DEBUG
	while(1){
		output_string("w");
		_delay_ms(10);
	}
	#endif
	/* Wait for PC-ready */
	//while(!READY);

	while(1);
	return 0;
}

//------------------------------------------------------------
