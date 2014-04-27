#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 20000000UL
#include <util/delay.h>
//#include "serial.h"
#include "control.h"
#include "lcd.h"
#include "timers.h"
#include "analogue.h"

//
//	Serial communications library
//

#define BUFSIZE 64
static volatile char buffer[BUFSIZE];
static volatile uint8_t insert_pos = 0;
static volatile uint8_t bytes_in_buffer = 0;
volatile uint8_t READY = 0;

void output_char (char c);
void output_string (char * string);
void output_flush (void);
void setup_serial (void);
void output_clear (void);

/* ------------------------------------------------------ */

//initialise
void setup_serial (void)
{
	/* baud rate */
	UBRR0 = 169;
	/* activate In interrupt, In/Out hardware */
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<UDRIE0)|(1<<TXEN0) ;
}

//circular buffer WRITE
void output_char (char c) 
{
	/* Check buffer not full */
	if (bytes_in_buffer < BUFSIZE) {
		buffer[insert_pos++] = c;
		bytes_in_buffer++;
		/* Wrap cursor if reached end */
		if (insert_pos == BUFSIZE) {
			insert_pos = 0;
		}
	}
}

//buffer whole string
void output_string (char * string)
{
	unsigned char i;
	/* loop output_char over string length */
	for (i = 0; string[i] != '\0'; i++) {
		output_char(string[i]);
	}
}

//flush serial buffer
void output_flush (void)
{
	/* Enable buffer-out-empty interrupt */
	UCSR0B |= (1<<UDRIE0);
}

void output_clear (void)
{
	/* Wipe the output buffer */
	insert_pos = 0;
	bytes_in_buffer = 0;
}

//UART Data Register Empty
ISR(USART0_UDRE_vect) 
{
	/* Check needing to send */
	if (bytes_in_buffer > 0) {
		/* Sending! */
		
		char c;
		/* Read bytes_in_buffer backwards from the cursor */
		if (insert_pos - bytes_in_buffer < 0) {
			/* wrap pointer backwards in buffer */
			c = buffer[insert_pos - bytes_in_buffer + BUFSIZE];
		} else {
			c = buffer[insert_pos - bytes_in_buffer];
		}
		
		/* Mark as used */
		bytes_in_buffer--;
		/* send it! */
		UDR0 = c;
	}
	/* If not, disable interrupt */
	else {
		UCSR0B &= ~(1<<UDRIE0);
	}
}

#define DEBUG 1

//UART Receive Complete
ISR(USART0_RX_vect) 
{
	/* Retrieve last byte */
	char c = UDR0;

	#if DEBUG
	_delay_ms(1);
	output_string("mm");
	output_flush();
	#endif
	
	/* What to do? */
	switch (c) {
		case 'M':
			if (READY) {
				/* Goto menu */
				task_stop();
				lcd_menu();
				//task_set('M');
			} else {
				READY = 1;
			}
			//override serial with an echo
			#if (!DEBUG)
			output_clear();
			#endif
			output_string("MM");
			output_flush();
			break;
		case 'V':
			if (READY) {
				/* Run voltmeter */
				task_stop();
				lcd_dvm_back();
					//enable DVM draw alarm (no longer needed)
				setup_adc(DVM_PIN);
				adc_resume();
					//enable d
				//task_set('V');
				mode = 'V';
				//override serial with an echo
				#if (!DEBUG)
				output_clear();
				#endif
				output_string("VV");
				output_flush();
			}
			break;
		case 'S':
			if (READY) {
			/* Run sweep */
				task_stop();
				//signal chip start
				do_send_interrupt(PIN_RUN);
				lcd_graph_background();
				//setup adc
				setup_adc(MIC_PIN);
				adc_resume();
				//task_set('S');
				mode = 'S';
				//override serial with an echo
				#if (!DEBUG)
				output_clear();
				#endif
				output_string("SS");
				output_flush();
			}
			break;
	}
}
