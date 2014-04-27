// serial.h
//
// setup_serial() must be called before serial communication
// can take place.
//

#ifndef SERIAL_H
#define SERIAL_H

/* Enable serial send/rcv */
void setup_serial (void);

/* Add string to outgoing buffer */
void output_char (char c);
void output_string (char * str);
void output_flush (void);

/* Globals */
volatile uint8_t READY;

#endif
