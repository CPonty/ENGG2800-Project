#include <avr/io.h>
#include <avr/interrupt.h>
//#define F_CPU 20000000UL
//#include <util/delay.h>
#include <avr/pgmspace.h>

//---------------------------------------------------------------------
// DECLARATIONS

/* TEMPORARY */
//activate debug mode to force continuous sweep
//due to damaged serial communications hardware, the controller is not going
//to activate this system, so it must be done manually
/* TEMPORARY */
#define DEBUG 1

#define PIN_WAIT (1<<PD2)
#define PIN_RUN (1<<PD3)
#define PIN_NEXT (1<<PD4)
#define PIN_RESET (1<<PD5)
 
// timer_value = clkspd / noteFreq / sampleCount - 1 

// samplecount = clkspd / noteFreq / InterruptLength - 1 

//interruptLength = ~70clks

// 50: (400000) 512 samples, timer=780clk
// 200: (100000) 512 samples, timer=194clk
// 500: (40000) 512 samples, timer=77clk
// 1000: 285max, 256 samples, timer=77clk
// 3000: 97max, 64 samples, timer=103clk
// 6000: 47max, 32 samples, timer=103clk
// 10000: 28max, 16 samples, timer=124clk
// 16000: 17max, 16 samples, timer=77clk
//const uint16_t timer1_ovf_vect[] = {780,194,77,77,103,103,124,77};

// Increment size  = _maxstep/samples
//const uint8_t noteInc[] = {1,1,1,2,8,16,32,32};


/*	PIN 4 BROKE
	reimplementation capping the sample rate at 128/wave. More samples
	gives a saw-tooth effect due to the dead pin.
*/
#define _maxstep 128

static const uint8_t sineStep[] PROGMEM = {128,134,140,146,152,158,165,
170,176,182,188,193,198,203,208,213,218,222,226,230,234,237,240,243,245,
248,250,251,253,254,254,255,255,255,254,254,253,251,250,248,245,243,240,
237,234,230,226,222,218,213,208,203,198,193,188,182,176,170,165,158,152,
146,140,134,128,121,115,109,103,97,90,85,79,73,67,62,57,52,47,42,37,33,
29,25,21,18,15,12,10,7,5,4,2,1,1,0,0,0,1,1,2,4,5,7,10,12,15,18,21,25,29,
33,37,42,47,52,57,62,67,73,79,85,90,97,103,109,115,121};

// timer_value = clkspd / noteFreq / sampleCount - 1 

// samplecount = clkspd / noteFreq / InterruptLength - 1 

//interruptLength = ~70clks

// 50: (400000) 64 samples, timer=6249clk
// 200: (100000) 64 samples, timer=1561clk
// 500: (40000) 64 samples, timer=624clk
// 1000: 285max, 64 samples, timer=311clk
// 3000: 97max, 64 samples, timer=103clk
// 6000: 47max, 32 samples, timer=103clk
// 10000: 28max, 16 samples, timer=124clk
// 16000: 17max, 16 samples, timer=77clk
const uint16_t timer1_ovf_vect[] = {6249,1561,624,311,103,103,124,77};

// Increment size  = _maxstep/samples
const uint8_t noteInc[] = {2,2,2,2,2,4,8,8};

//---------------------------------------------------------------------

const uint16_t freq_ovf_vect[] = {50,200,500,1000,3000,6000,10000,16000};
volatile uint16_t plays_left = 0;

volatile uint8_t incrementer = 1;

/* interrupt states */
volatile uint16_t cyclesLeft = 0;
volatile uint16_t latestLookup = 0;
volatile uint8_t global_note = 0;
#define _note global_note

/* functions */
	//void write16( volatile uint16_t * reg, uint16_t x );
void TIMER1A_active( unsigned char yes );
void play_note( void );
void reset( void );
uint8_t debounce_port(volatile uint8_t * port, uint8_t mask, 
						uint16_t wait_us);

//---------------------------------------------------------------------
// FUNCTIONS

uint8_t pin_d_last = 0;

/* Startup
*/
int main(void) 
{
	uint16_t i;

	/* port IO values */
	DDRB = 0xFF; // portb(0..7) => r2r
	DDRD = 0x00; // portd(2..5) <= controller
	PORTB = 0x00;

	/* 16-bit timer setup 
	*/
	TCCR1A = (1<<COM1A1); // reset timer-counter on trigger
	TCCR1B = 0; // disable for now
	TIMSK = (1<<OCIE1A); // use timer compare counter A

	/* Running state */
	reset();
	global_note = 0;

	/* Wait for MCU *///1000
	for (i=0; i<(1000*20); i++) asm("nop");

	//
	#if (DEBUG)
	global_note = 1;
	play_note();
	#endif
	//

	/* run forever using interrupts */
	sei();
	while(1) {
		/* output testing */
		#if DEBUG
		if (!plays_left) {
			cli();
			global_note++;
			if (global_note == 9) global_note = 1;
			play_note();
			sei();
		}
		#endif

		/* Chip communication */
		#if (!DEBUG)
		
		// wait for line high
		while(!(PORTD & PIN_WAIT)) {
			asm("nop");
		}

		//debounce
		if (debounce_port(&PORTD, PIN_WAIT, 10)) {
			//pin change!
			cli();

			//disable/enable notes
			if (PORTD & PIN_RUN) {
				//enable note
				if (!global_note) global_note = 1;
				play_note();
			} else {
				//temporary stop
				if (!(PORTD & PIN_NEXT)) reset();				
			}

			//next note
			if (PORTD & PIN_NEXT) {
				if (global_note) global_note++;
				if (global_note == 9) global_note = 1;
			}

			//reset
			if (PORTD & PIN_RESET) {
				//perma-stop
				reset();
				global_note = 0;
			}

			//
			sei();

		}
		// wait for line low
		while(PORTD & PIN_WAIT) {
			asm("nop");
		}

		#endif
	}
	while(1);
	return 0;
}

/* Debounce
*/
uint8_t debounce_port(volatile uint8_t * port, uint8_t mask, 
					  uint16_t wait_us)
{
	uint16_t i;
	/* Wait */
	for (i=0; i<(wait_us*20); i++) asm("nop");

	/* Return if same */
	if (*port & mask) return 1;
	else return 0;
}


/* Timer1A Event
*/
ISR(TIMER1_COMPA_vect)
{
	/* Output lookup table */
	PORTB = pgm_read_byte(&sineStep[latestLookup]);
	latestLookup += incrementer;

	/* Check if end-of-table */
	if (latestLookup >= _maxstep) {
		latestLookup = 0;
		//
		#if DEBUG
		plays_left--;
		#endif
	}
}


/* Reset Code
*/
void reset( void )
{
	/* Disable output */
	PORTB = 0; // zero voltage
	TIMER1A_active(0); // timer off
}

/* Play note
*/
void play_note( void )
{

	PORTB = 0; // zero voltage
	TIMER1A_active(0); // timer off

	/* Valid command? */
	if (!((_note > 0) && (_note < 9))) return;
	else {

		/* Note - start output */
		incrementer = noteInc[_note-1];
		plays_left = freq_ovf_vect[_note-1]*2; // set cycles remaining
		latestLookup = 0;

		OCR1A = timer1_ovf_vect[_note-1]; // set timer length
		TIMER1A_active(1);
		//OCR1A = 1;

	}
}

//---------------------------------------------------------------------
// UTILITIES



/* Toggle Timer A
*/
void TIMER1A_active( unsigned char yes )
{
	if (yes) {
		/*  
		Activate timer A
		CTC mode, use undivided clock 
		*/
		TCCR1B |= (1<<WGM12)|(1<<CS10);
	} else {
		TCCR1B = 0;
	}
}
