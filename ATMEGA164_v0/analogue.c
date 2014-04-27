#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
#include "serial.h"
#include "control.h"
#include "lcd.h"
#include "timers.h"
//#include "analogue.h"

//	Analogue-Digital conversion and processing library
//
//  Features:
//		adc hardware
//		bit->string conversion
//		digital I-buffers and processing
//			

/* Macros */
#define ADC_FREQ 26240
/* 	adc_freq  = 	noteFreqMax * 2.05
				=> (16000*2.05) = (32,800)
	max_reads = 	freq * time(0.8 second)
*/
#define DVM_FREQ 210
/* 	adc_freq  = 	waveFreqMax * 2.1
				=> (500*2.1) = (1050)
	max_reads = 	freq * time(0.2 second)
*/

#define DVM_PIN (1<<PA6)
#define MIC_PIN (1<<PA7)

void setup_adc (uint8_t pin_numb);
void adc_pause (void);
void adc_resume (void);
void adc_off (void);
void adc_fetch (void);

void adc_rescale (void);
void adc_process (void);
void adc_dvm_process (void);
void adc_memset (void);
void dvm_memset (void);


/*************/
volatile uint8_t adc_run = 0;
volatile uint16_t adc_reads = 0;
volatile uint8_t adc_low[8] = {255,255,255,255,255,255,255,255};
volatile uint8_t adc_high[8] = {0,0,0,0,0,0,0,0};
volatile uint8_t adc_range[8] = {1,1,1,1,1,1,1,1};

volatile uint8_t adc_bars[8] = {1,1,1,1,1,1,1,1};
volatile uint8_t current_note = 0;

volatile uint16_t dvm_low = 5000;
volatile uint16_t dvm_high = 0;
volatile uint16_t dvm_range = 1;
volatile uint16_t dvm_run = 0;
volatile uint16_t dvm_reads = 0;
// ------------------------------------------------------ 

/* ADC hardware operation */

void setup_adc (uint8_t pin_numb)
{
	ADMUX |= (1<<REFS0) + pin_numb; 
		// (1<<REFS0) sets AVCC as reference-positive
		// pin_numb sets MUX4..0 to select reference pin
	ADCSRA |= (1<<ADIE); 
		//(1<<ADEN) enables ADC conversion 
		//(1<<ADIE) enables ADC convert-complete interrupt
	DIDR0 = 0b11000000;
		//digital input disable mask for ADC pins
	ADCSRB = (1<<ADTS2)|(1<<ADTS0);
}

void adc_pause (void) 
{
	//
	adc_run = 0;
	ADCSRA &= ~(1<<ADEN);
}

void adc_resume (void) 
{
	//
	adc_run = 1;
	ADCSRA |= (1<<ADEN);
}

void adc_off (void)
{
	ADMUX = 0;
	ADCSRA = 0;
}

/* Force ADC read */
void adc_fetch (void)
{
	ADCSRA |= (1<<ADSC);
}

/* ADC receive */
ISR(ADC_vect)
{
	if (adc_run) adc_process();


}

// ------------------------------------------------------ 

/* Rescale ADC peaks to be close to 0..255 */
void adc_rescale (void)
{	/* NB: no division or fractional truncating, uses
			primarily bit shifting as the AVR has no float ALU */
	uint8_t i, j;
	uint8_t max = 0;
	uint8_t max_index = 0;
	uint8_t shifts = 0;
	uint8_t adds = 0;
	//copy across values
	for (i = 0; i<8; i++) {
		adc_bars[i] = adc_range[i];
	}
	//find the max
	for (i = 0; i<8; i++) {
		if (adc_bars[i] > max) {
			max = adc_bars[i];
			max_index = i;
		}
	}
	//shift to increase scale above 50%
	while(adc_bars[max_index] < 128) {
		adc_bars[max_index] <<= 1;
		shifts++;
	}
	//add to increase scale above 75%
	while (adc_bars[max_index] < (64*3)) {
		adc_bars[max_index] += (adc_bars[max_index] >> 2);
		adds++;
	}
	//apply to all
	for (i = 0; i<8; i++) {
		for (j = 0; j<shifts; j++) {
			adc_bars[i] <<= 1;
		}
		for (j = 0; j<adds; j++) {
			adc_bars[i] += (adc_bars[i] >> 2);
		}
	}
}

/* Do something with the inbound ADC value */
void adc_process (void)
{	
	uint8_t adc_result = ADC>>2;
	
	//halt serial-recv until finished
	if (interrupts_set) cli();
	
	/* Process Value */
	adc_reads++;
	if (adc_result < adc_low[current_note]) {
		adc_low[current_note] = adc_result;
	} else
	if (adc_result > adc_high[current_note]) {
		adc_high[current_note] = adc_result;
	}
	
	/* Last Sample for this Note */
	if (adc_reads == ADC_FREQ-1) {
		adc_pause();
		
		/* Post-processing */
		adc_range[current_note] = adc_high[current_note] - adc_low[current_note];
		if (adc_range[current_note] == 0) {
			adc_range[current_note] = 1;
		}
		output_char((char)adc_range[current_note]);
		current_note++;
		adc_reads = 0;
		
		/* Continuation select */
		if (current_note == 8) {
			
			/* Last note: finalise and restart */
			adc_rescale();
			lcd_graph(adc_bars);
			output_char('\0');
			adc_memset();
		}
		//resume sweep
		output_flush();
		do_send_interrupt(PIN_NEXT);
		_delay_us(10);
		adc_resume();
	}
	
	//re-enable serial
	if (interrupts_set) sei();
}



/* Do something with the inbound ADC value */
void adc_dvm_process (void)
{
	uint16_t dvm_result = ADC*5;
	if (dvm_result > 5000) dvm_result = 5000;

	//halt serial-recv until finished
	if (interrupts_set) cli();
	
	/* Process Value */
	dvm_reads++;
	if (dvm_result < dvm_low) {
		dvm_low = dvm_result;
	} else
	if (dvm_result > dvm_high) {
		dvm_high = dvm_result;
	}
	
	/* Last Sample before LCD update/buffer out */
	if (adc_reads == ADC_FREQ-1) {
		adc_pause();
		
		/*  REQUIRES FINISHING  */
		//process dvm values here: duplicate the existing sweep adc code, 
		//and change the source/target memory, and the screen to redraw
		/*  REQUIRES FINISHING  */


		//resume sweep
		output_flush();
		adc_resume();
	}

	//re-enable serial
	if (interrupts_set) sei();
}

/*
	Memory allocation and setup before sweep
*/
void adc_memset (void)
{
	uint8_t i;
	current_note = 0;
	for (i=0; i<8; i++) {
		adc_low[i] = 255;
		adc_high[i] = 0;
		adc_range[i] = 1;
		adc_bars[i] = 1;
	}
}

void dvm_memset (void)
{
	dvm_low = 255;
	dvm_high = 0;
	dvm_range = 1;
	dvm_reads = 0;
}

