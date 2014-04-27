//
// timers.h
//

#ifndef ANALOGUE_H
#define ANALOGUE_H

/* ADC hardware */
void setup_adc (uint8_t pin_numb);
void adc_pause (void);
void adc_resume (void);
void adc_off (void);
void adc_fetch (void);

/* Data processing & events */
void adc_process (void);
void adc_rescale (void);
void adc_dvm_process (void);
void adc_memset (void);
void dvm_memset (void);

/* Globals */
volatile uint16_t adc_run;
volatile uint8_t adc_range[8];
volatile uint8_t adc_bars[8];
volatile uint8_t current_note;
volatile uint16_t dvm_run;

/* Macros */
#define ADC_FREQ 32800

#define DVM_PIN (1<<PA6)
#define MIC_PIN (1<<PA7)

#endif
