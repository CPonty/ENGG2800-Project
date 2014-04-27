//
// lcd.h
//

#ifndef LCD_H
#define LCD_H

// ---------------------------------------------------

void lcd_init (void);
//uint8_t pin_reverse(uint8_t value);

// ---------------------------------------------------

//void lcd_hold (void);
//void lcd_wait_ready (void);
//void lcd_instruction (uint8_t instruction);
//void lcd_put_data (uint8_t data);
//void lcd_put_data_16 (uint16_t data);
//void lcd_string (char * string);

// ---------------------------------------------------

//void lcd_goto_address (uint16_t address);
//void lcd_goto_text (uint8_t x, uint8_t y);
//void lcd_goto_pixelblock (uint8_t x, uint8_t y);

// ---------------------------------------------------

//void lcd_wipe_text (void);
//void lcd_wipe_pixels (void);
//void lcd_wipe_charset (void);
void lcd_wipe_all (void);

// ---------------------------------------------------

void lcd_splash (void);
void lcd_graph_background (void);
//void lcd_graph_bar (uint8_t n, uint8_t len);
void lcd_graph (volatile uint8_t bar_vect[8]);
//void lcd_splash_sine (uint8_t offset);
void lcd_splash_animate (void);
void lcd_menu (void);
void lcd_dvm_back (void);
void lcd_dvm_data (uint16_t value);

#endif
