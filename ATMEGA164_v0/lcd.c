#define F_CPU 20000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "serial.h"
#include "analogue.h"
#include "timers.h"
#include "control.h"
//#include "lcd.h"

//
// LCD T6963C communications library
//	- Hardware abstraction
//  - Packaged drawing tools
//

//use C for actual, D for testing
#define DDR_DATA DDRC //D
#define PORT_DATA PORTC //D
#define PIN_DATA PINC //D

#define WIDTH_PIXELS 128
#define WIDTH_BYTES 16
#define HEIGHT_PIXELS 64
#define HEIGHT_BYTES 8

#define CHAR_COUNT (WIDTH_BYTES*HEIGHT_BYTES)
#define BYTE_COUNT (WIDTH_BYTES*HEIGHT_PIXELS)

#define LENGTH_CHARSET 64 //64 stored 8x8bit character images
#define CHARSET_CHAR_BYTES 8 //8 bytes/character

#define TEXT_ADDR 0
#define GRAPHICS_ADDR (TEXT_ADDR + CHAR_COUNT)
#define CHARSET_ADDR 0 //not implemented yet

#define WRITE_BIT (1<<PA0)
#define READ_BIT (1<<PA1)
#define ENABLE_BIT (1<<PA2)
#define COM_DATA_BIT (1<<PA3)
#define RESET_BIT (1<<PA4)
#define FONT_BIT (1<<PA5)
#define CONTROL_MASK (0b00111111-FONT_BIT)

/*  Set address pointer from the last two bytes */
#define lcd_pointer_set() lcd_instruction(0b00100100);

/*  Write last byte to memory & increment ADP 
    ADP = arithmetic data pointer  */
#define lcd_write_with_increment() lcd_instruction(0b11000000);

/*	Shift the ascii value to cut off the first 32 characters
	Which the lcd does not support  */
#define ascii_to_lcd(c) (c-32)

//uint8_t bar_pixels[8] = {1,5,23,44,55,32,16,3};

// ---------------------------------------------------

/*  Function prototypes  */

void lcd_init (void);
uint8_t pin_reverse(uint8_t value);
// ---------------------------------------------------
void lcd_hold (void);
void lcd_wait_ready (void);
void lcd_instruction (uint8_t instruction);
void lcd_put_data (uint8_t data);
void lcd_put_data_16 (uint16_t data);
void lcd_string (char * string);
// ---------------------------------------------------
void lcd_goto_address (uint16_t address);
void lcd_goto_text (uint8_t x, uint8_t y);
void lcd_goto_pixelblock (uint8_t x, uint8_t y);
// ---------------------------------------------------
void lcd_wipe_text (void);
void lcd_wipe_pixels (void);
void lcd_wipe_charset (void);
void lcd_wipe_all (void);
// ---------------------------------------------------
void lcd_splash (void);
void lcd_splash_sine (uint8_t offset);
void lcd_splash_animate (void);
void lcd_menu (void);
void lcd_graph_background (void);
void lcd_graph_bar (uint8_t n, uint8_t len);
void lcd_graph (volatile uint8_t bar_vect[8]);
void lcd_dvm_back (void);
void lcd_dvm_data (uint16_t value);

// ---------------------------------------------------
static const char splash_1kb[] PROGMEM = { 
0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 11, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 20, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 42, 240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 91, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 85, 208, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 219, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 1, 87, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 15, 110, 224, 3, 255, 255, 255, 252, 0, 0, 0, 0, 0, 
0, 0, 0, 29, 155, 192, 60, 0, 0, 0, 2, 0, 0, 0, 0, 0, 
0, 0, 0, 54, 255, 1, 194, 170, 170, 170, 170, 0, 0, 0, 0, 0, 
0, 0, 0, 58, 86, 2, 41, 36, 146, 73, 36, 0, 0, 0, 0, 0, 
0, 0, 0, 119, 120, 13, 74, 146, 73, 36, 148, 0, 0, 0, 0, 0, 
0, 0, 0, 210, 240, 50, 81, 73, 36, 146, 74, 0, 0, 0, 0, 0, 
0, 0, 0, 254, 192, 73, 10, 36, 146, 74, 164, 0, 0, 0, 0, 0, 
0, 0, 0, 33, 0, 165, 255, 255, 255, 249, 42, 0, 0, 0, 0, 0, 
0, 0, 0, 95, 1, 84, 234, 171, 90, 196, 148, 0, 0, 0, 0, 0, 
0, 0, 0, 84, 2, 146, 63, 253, 239, 170, 68, 0, 0, 0, 0, 0, 
0, 0, 0, 170, 5, 73, 74, 175, 123, 19, 84, 0, 0, 0, 0, 0, 
0, 0, 0, 214, 10, 45, 47, 251, 214, 74, 36, 0, 0, 0, 0, 0, 
0, 0, 0, 180, 9, 88, 146, 189, 252, 167, 84, 0, 0, 0, 0, 0, 
0, 0, 1, 84, 21, 46, 75, 247, 85, 42, 138, 0, 0, 0, 0, 0, 
0, 0, 127, 88, 40, 154, 165, 187, 250, 143, 84, 0, 0, 0, 0, 0, 
0, 0, 65, 168, 37, 79, 43, 127, 168, 93, 36, 0, 0, 0, 0, 0, 
0, 0, 87, 136, 84, 173, 193, 128, 58, 182, 148, 0, 0, 0, 0, 0, 
0, 0, 138, 208, 165, 31, 117, 123, 210, 254, 164, 0, 0, 0, 0, 0, 
0, 0, 163, 208, 146, 75, 223, 0, 31, 171, 20, 0, 0, 0, 0, 0, 
0, 0, 150, 241, 85, 93, 123, 64, 10, 254, 164, 0, 0, 0, 0, 0, 
0, 0, 171, 225, 32, 143, 221, 64, 31, 171, 20, 0, 0, 0, 0, 0, 
0, 0, 212, 1, 86, 90, 239, 64, 10, 254, 164, 0, 0, 0, 0, 0, 
0, 1, 52, 2, 137, 47, 117, 64, 15, 170, 150, 0, 0, 0, 0, 0, 
0, 1, 108, 2, 84, 155, 191, 0, 26, 255, 72, 0, 0, 0, 0, 0, 
0, 0, 164, 2, 162, 174, 213, 119, 127, 170, 36, 0, 0, 0, 0, 0, 
0, 1, 116, 2, 74, 77, 255, 0, 21, 255, 84, 0, 0, 0, 0, 0, 
0, 1, 76, 2, 145, 47, 85, 64, 14, 170, 148, 0, 0, 0, 0, 0, 
0, 0, 212, 1, 76, 155, 255, 64, 15, 254, 68, 0, 0, 0, 0, 0, 
0, 0, 151, 225, 34, 173, 85, 64, 26, 171, 84, 0, 0, 0, 0, 0, 
0, 0, 166, 241, 170, 79, 255, 64, 15, 254, 148, 0, 0, 0, 0, 0, 
0, 0, 147, 80, 145, 45, 87, 0, 29, 86, 68, 0, 0, 0, 0, 0, 
0, 0, 171, 208, 170, 159, 241, 109, 82, 251, 42, 0, 0, 0, 0, 0, 
0, 0, 69, 152, 68, 74, 139, 0, 25, 94, 164, 0, 0, 0, 0, 0, 
0, 0, 83, 72, 82, 175, 81, 255, 250, 54, 148, 0, 0, 0, 0, 0, 
0, 0, 127, 40, 42, 90, 151, 191, 233, 78, 74, 0, 0, 0, 0, 0, 
0, 0, 1, 116, 21, 13, 74, 245, 188, 151, 36, 0, 0, 0, 0, 0, 
0, 0, 0, 204, 16, 188, 39, 127, 213, 66, 170, 0, 0, 0, 0, 0, 
0, 0, 0, 162, 13, 74, 173, 213, 254, 43, 36, 0, 0, 0, 0, 0, 
0, 0, 0, 170, 5, 41, 46, 254, 171, 85, 84, 0, 0, 0, 0, 0, 
0, 0, 0, 93, 2, 148, 155, 87, 255, 137, 10, 0, 0, 0, 0, 0, 
0, 0, 0, 85, 1, 74, 125, 250, 170, 228, 164, 0, 0, 0, 0, 0, 
0, 0, 0, 50, 128, 163, 247, 223, 255, 250, 84, 0, 0, 0, 0, 0, 
0, 0, 0, 238, 192, 84, 8, 32, 0, 1, 74, 0, 0, 0, 0, 0, 
0, 0, 0, 243, 112, 50, 165, 74, 170, 173, 36, 0, 0, 0, 0, 0, 
0, 0, 0, 93, 120, 12, 170, 85, 85, 80, 148, 0, 0, 0, 0, 0, 
0, 0, 0, 123, 174, 6, 73, 34, 34, 38, 84, 0, 0, 0, 0, 0, 
0, 0, 0, 44, 95, 1, 164, 148, 149, 73, 68, 0, 0, 0, 0, 0, 
0, 0, 0, 29, 219, 192, 117, 73, 72, 146, 42, 0, 0, 0, 0, 0, 
0, 0, 0, 14, 174, 224, 7, 255, 255, 255, 252, 0, 0, 0, 0, 0, 
0, 0, 0, 1, 87, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 1, 186, 240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 173, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 86, 224, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 49, 112, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 20, 176, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 15, 240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0 , 0, 0, 0, 0, 0, 0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0  };

static const char graph_1kb[1024] PROGMEM = {

0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x40, 
0x93, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x40, 
0x93, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x7D, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 
0x83, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 
0x83, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 
0xFD, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0xFD, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 
0x81, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 

0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x81, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 
0x83, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 
0x83, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x61, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x91, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 
0x91, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 

0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 
0x3F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 
0xC1, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x39, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x07, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5E, 
0x39, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 
0xC1, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 
0x3F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 
0x07, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 
0x39, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 
0xD1, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x39, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x07, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 

0x0F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x0F, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 
0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69, 
0x93, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 
0x93, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x63, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x95, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0x99, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 
0xFF, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x40, 
0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x40 };

#define SINE_Y (HEIGHT_PIXELS - 8)

static const uint8_t sineStep[32]  = {
6, 6, 7, 8, 9, 10, 11, 11, 12, 11, 11, 10, 9, 8, 
7, 6, 6, 6, 5, 4, 3, 2, 1, 1, 0, 1, 1, 2, 3, 4, 5, 6 };

static uint8_t sineBlocks[4*14] = {0};

// ---------------------------------------------------

void lcd_init (void)
{
	//uint16_t i;

	//PORT_DATA == data
	DDR_DATA = 0xFF;
	//porta == control
	DDRA = CONTROL_MASK;
	// reset chip, 1ms
	PORTA = CONTROL_MASK;
	PORTA &= ~RESET_BIT;
	_delay_ms(2);
	// end reset, set font
	PORTA |= CONTROL_MASK;
	// set graphics address
	lcd_put_data_16(GRAPHICS_ADDR);
	lcd_instruction(0b01000010);
	// set graphics bytesize
	lcd_put_data_16(WIDTH_BYTES);
	lcd_instruction(0b01000011);
	// set text address
	lcd_put_data_16(TEXT_ADDR);
	lcd_instruction(0b01000000);
	// set text bytesize
	lcd_put_data_16(WIDTH_BYTES);
	lcd_instruction(0b01000001);
	//IF IMPLEMENT: set character set address
	//IF IMPLEMENT: set character set size
	// reset address pointer
	lcd_goto_address(0);
	// switch on: (display, text area, graphic area)
	lcd_instruction(0b10011100);
	// mode set: OR-write ("paint-over")
	lcd_instruction(0b10000000);
}

uint8_t pin_reverse(uint8_t value)
{
	/* Sort bits in reverse order for device connections */
	uint8_t i;
	uint8_t out = 0;
	for (i = 0; i<8; i++) {
		if (value & (1<<i)) {
			out |= (1 << (7-i));
		}
	}
	//return value;
	return out;
}

// ---------------------------------------------------

void lcd_hold (void)
{
	/* Wait for the LCD's 1/64 duty cycle 
	   (  3clk: for-loop operations
	    + 1clk: no-operation )
	     4x16  ==>  64 clock cycles */
	volatile uint8_t i;
	for (i = 0; i < 20; i++) asm("nop");
}

void lcd_wait_ready (void)
{
	// set control signals
	DDR_DATA = 0x00;
	PORTA |= CONTROL_MASK;
	PORTA &=  ~(READ_BIT + ENABLE_BIT);
	// wait for lcd-ready
	while ((pin_reverse(PIN_DATA) & 0b11) != 0b11);
	// restore controls (off)
	DDR_DATA = 0xFF;
	PORTA |= CONTROL_MASK;
}

void lcd_instruction (uint8_t instruction)
{
	// when ready
	lcd_wait_ready();
	// set control/data
	PORT_DATA = pin_reverse(instruction);
	PORTA |= CONTROL_MASK;
	PORTA &=  ~(WRITE_BIT + ENABLE_BIT);
	// wait for LCD to read & return to original state
	lcd_hold();
	PORTA |= CONTROL_MASK;
}

void lcd_put_data (uint8_t data)
{
	/* NB: order 16-bit values (low,high)
	*/
	// when ready
	lcd_wait_ready();
	// set control/data
	DDR_DATA = 0xFF;
	PORT_DATA = pin_reverse(data);
	PORTA |= CONTROL_MASK;
	PORTA &= ~(WRITE_BIT + ENABLE_BIT + COM_DATA_BIT);
	// wait for LCD to read & return to original state
	lcd_hold();
	PORTA |= CONTROL_MASK;
}

void lcd_put_data_16 (uint16_t data)
{
	/* Write high/low bytes separately */
	lcd_put_data(data & 0xFF);
	lcd_put_data(data >> 8);
}

void lcd_string (char * string)
{
	uint8_t i = 0;
	// loop through the string performing data writes
	while (string[i] != '\0') {
  		lcd_put_data(ascii_to_lcd(string[i++]));
  		lcd_write_with_increment();
	}
}

// ---------------------------------------------------

void lcd_goto_address (uint16_t address)
{
	/* Move the address pointer with a series of LCD writes */
	lcd_put_data_16(address);
	lcd_pointer_set();
}

void lcd_goto_text (uint8_t x, uint8_t y)
{
	/* Generate an address pointer to go to */
	lcd_goto_address(TEXT_ADDR + x + (y*WIDTH_BYTES));
}

void lcd_goto_pixelblock (uint8_t x, uint8_t y)
{
	/* Generate an address pointer to go to */
	lcd_goto_address(GRAPHICS_ADDR + x + (y*WIDTH_BYTES));
}

// ---------------------------------------------------

void lcd_wipe_text (void)
{
	uint16_t i;
	// move pointer to text area
	lcd_goto_address(TEXT_ADDR);
	// write blanks to memory
	for (i = 0; i < CHAR_COUNT; i++) {
 		lcd_put_data(0);
 		lcd_write_with_increment();
 	}
}

void lcd_wipe_pixels (void)
{
	uint16_t i;
	// move pointer to graphic area
	lcd_goto_address(GRAPHICS_ADDR);
	// write blanks to memory for all graphics
	for (i = 0; i < BYTE_COUNT; i++) {
  		lcd_put_data(0);
  		lcd_write_with_increment();
  	}
}

void lcd_wipe_charset (void)
{
	uint16_t i;
	// move pointer to start
	lcd_goto_address(CHARSET_ADDR);
	// write entire character generator contents blank
	for (i = 0; i < (LENGTH_CHARSET * CHARSET_CHAR_BYTES); i++) {
  		lcd_put_data(0);
  		lcd_write_with_increment();
  	}
}

void lcd_wipe_all (void)
{
	/* Fast wipe-all */
	lcd_wipe_text();
	lcd_wipe_charset();
	lcd_wipe_pixels();
}

// ---------------------------------------------------

void lcd_splash (void)
{
	uint16_t i;

	/* buffer out the uq splash screen
	   1kb of program memory  */
	lcd_goto_address(GRAPHICS_ADDR);
	for (i = 0; i < BYTE_COUNT; i++) {
		lcd_put_data(pgm_read_byte(&splash_1kb[i]));
		lcd_write_with_increment();
	}
}

void lcd_splash_sine (uint8_t offset)
{
	uint8_t i, j, k;
	
	// clear sineBlocks
	for (i = 0; i < (14*4); i++) {
		sineBlocks[i] = 0;
	}
	// set sineBlocks
	for (i = 0; i < 32; i++) {
		j = sineStep[(i+offset)%32];
		if (j!=0) sineBlocks[(i>>3) + (j-1)*4] |= ((1<<7) >> (i%8));
		sineBlocks[(i>>3) + j*4] |= ((1<<7) >> (i%8));
		if (j!=13) sineBlocks[(i>>3) + (j+1)*4] |= ((1<<7) >> (i%8));
	}
	// buffer out sineBlock, 4 times
	//for each sineBlock(4)
	for (i = 0; i<4; i++) {
		//for each row(14)
		for (j = 0; j<14; j++) {
			//for each column(4)
			for (k = 0; k<4; k++) {
				lcd_goto_pixelblock((i*4)+k, SINE_Y - j);
				lcd_put_data(sineBlocks[k+(4*j)]);
				lcd_write_with_increment();
			}
		}
	}
}

void lcd_splash_animate (void)
{
	uint8_t i, j = 0;
	// display splash screen text
	lcd_goto_text(0,0);
	lcd_string("|              |");		
	lcd_goto_text(0,1);
	lcd_string("| Starting     |");
	lcd_goto_text(0,2);
	lcd_string("|              |");
	// animate a sine wave
	for (i = 0; i < 32; i++) {
		//text
		if (j%8 == 2) {
			lcd_goto_text(10 + (j>>3),1);
			lcd_string(".");
		}
		if (j%2 == 1) {
			lcd_goto_text((j>>1),3);
			lcd_string("*");
		}
		//plot
		//lcd_wipe_pixels();
		lcd_splash_sine(i*3);
		_delay_ms(75);
		j++;
	}
}

void lcd_menu (void)
{
	/* print the menu screen text */
	lcd_goto_text(0,0);
	lcd_string("| Team 34      |");
	lcd_goto_text(0,1);
	lcd_string("| Audio Spectra|");
	lcd_goto_text(0,2);
	lcd_string("| Analyzer     |");
	lcd_goto_text(0,3);
	lcd_string("----------------");
	lcd_goto_text(0,5);
	lcd_string("=> AUDIO SWEEP");
	lcd_goto_text(0,7);
	lcd_string("=> VOLTMETER");
}

void lcd_graph_background (void)
{
	uint16_t i;
	
	/* buffer out the graph background
	   1kb of program memory  */
	lcd_goto_address(GRAPHICS_ADDR);
	for (i = 0; i < BYTE_COUNT; i++) {
		lcd_put_data(pgm_read_byte(&graph_1kb[i]));
		lcd_write_with_increment();
	}
}

void lcd_graph_bar (uint8_t n, uint8_t len)
{
	/* Print the bar length at a relative position */
	uint8_t i, j, bar_byte;

	// for each byte of length
	for (i = 0; i <= (len>>3); i++) {
		//get the bar piece length
		if (i==(len>>3)) {
			bar_byte = 0xFF<<(7-(len%8));
		} else {
			bar_byte = 0xFF;
		}
		//write a 4-pixel-thick block
		for (j = 0; j < 4; j++) {
			lcd_goto_pixelblock(i+4, (n*8) + (j+2));
			lcd_put_data(bar_byte);
			lcd_write_with_increment();
		}
	}
}

void lcd_graph (volatile uint8_t bar_vect[8])
{
	/* Calculate & display the frequency analysis graph */
	uint8_t i, x;
	uint8_t ints[3];
	char string[3];
	uint8_t bars[8] = {0};

	//print the graph background
	lcd_graph_background();

	//print the frequencies
	lcd_goto_text(12,0);
	lcd_string("50");
	lcd_goto_text(12,1);
	lcd_string("200");
	lcd_goto_text(12,2);
	lcd_string("500");
	lcd_goto_text(12,3);
	lcd_string("1K");
	lcd_goto_text(12,4);
	lcd_string("3K");
	lcd_goto_text(12,5);
	lcd_string("6K");
	lcd_goto_text(12,6);
	lcd_string("10K");
	lcd_goto_text(12,7);
	lcd_string("16K");

	//each frequency:
	for (i = 0; i < 8; i++) {
		
		//convert adc maxima
		bars[i] = (bar_vect[i] >> 2);
		if (bars[i] == 0) bars[i] = 1;
		
		//convert: int -> string
		x = bar_vect[i];
		ints[0] = x%10;
		ints[1] = (x%100 - ints[0])/10;
		ints[2] = (x - ints[1]*10 - ints[0])/100;
		string[2] = (char)(ints[0]+16);
		string[1] = (char)(ints[1]+16);
		string[0] = (char)(ints[2]+16);

		//print the amplitude text
		lcd_goto_text(1,i);

		if ((ints[2]==0) && (ints[1]==0)) {
			lcd_put_data(' '-32);
  			lcd_write_with_increment();
			lcd_put_data(' '-32);
  			lcd_write_with_increment();
			lcd_put_data(string[2]);
  			lcd_write_with_increment();

		} else if (ints[2]==0) {
			lcd_put_data(' '-32);
  			lcd_write_with_increment();
			lcd_put_data(string[1]);
  			lcd_write_with_increment();
			lcd_put_data(string[2]);
  			lcd_write_with_increment();
		} else {
			lcd_put_data(string[0]);
  			lcd_write_with_increment();
			lcd_put_data(string[1]);
  			lcd_write_with_increment();
			lcd_put_data(string[2]);
  			lcd_write_with_increment();	
		}

		//graph the bar
		lcd_graph_bar(i, bars[i]);
	}
}

void lcd_dvm_back (void)
{
	/* print the lcd background text */
	lcd_goto_text(0,0);
	lcd_string("| Digital      |");
	lcd_goto_text(0,1);
	lcd_string("| Voltmeter:   |");
	lcd_goto_text(0,2);
	lcd_string("----------------");
	lcd_goto_text(0,4);
	lcd_string(" >> 0.000 pk-pk ");

}

void lcd_dvm_data (uint16_t dvm_value)
{
	uint8_t x;
	uint8_t ints[4];
	char string[4];

	/* rescale DVM value */
	x = dvm_value; //<< 1;

	/* print the lcd numerical text */
	lcd_goto_text(4,4);

	//convert: int -> string
	ints[0] = x%10;
	ints[1] = (x%100 - ints[0])/10;
	ints[2] = (x - ints[1]*10 - ints[0])/100;
	ints[3] = (x  - ints[2]*100- ints[1]*10 - ints[0])/1000;
	string[3] = (char)(ints[0]+16);
	string[2] = (char)(ints[1]+16);
	string[1] = (char)(ints[2]+16);
	string[0] = (char)(ints[3]+16);

	lcd_put_data(string[0]);
	lcd_write_with_increment();
	lcd_put_data('.'-32);
	lcd_write_with_increment();
	lcd_put_data(string[1]);
	lcd_write_with_increment();
	lcd_put_data(string[2]);
	lcd_write_with_increment();	
	lcd_put_data(string[3]);
	lcd_write_with_increment();	
}

// ---------------------------------------------------
