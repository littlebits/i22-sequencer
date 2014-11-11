/*
 * i22_sequencer.h
 *
 * Created: 11/6/2013 11:16:02 AM
 *  Author: Rory
 *
 * Copyright 2014 littleBits Electronics
 *
 * This file is part of i22-sequencer.
 *
 * i22-sequencer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * i22-sequencer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License at <http://www.gnu.org/licenses/> for more details.
 */ 

#ifndef I22_SEQUENCER_H_
#define I22_SEQUENCER_H_

// system clock speed
#define F_CPU 8000000UL

extern volatile int8_t curStep;
extern int16_t curSpeed;
extern unsigned long lastStepUpdate;
extern uint8_t speedState;
extern volatile int8_t pendDirection;

extern int8_t clockMode;
extern int8_t lastClockMode;
extern uint8_t runMode;

extern uint16_t aiFLag;	// ADC interrupt flag
extern uint16_t eiFlag;	// external interrupt flag
extern uint16_t giFlag;	// global interrupt flag

extern volatile unsigned long timer1_millis;
extern volatile int16_t sample_in_int16;
extern volatile uint8_t ready_for_adc_sample;

// This calculation is used for alpha in the EMA filter; it determines our cutoff frequency...
#define DSP_EMA_I16_ALPHA(x) ( (uint8_t)(x * 255) )

// function prototypes
void readPulse(void);
void runSpeedMode(int8_t mode);
void init_adc(uint8_t adc_clk_prescalar);
void init_millis_timer(void);
unsigned long millis(void);
int16_t dsp_ema_i16(int16_t, int16_t, uint8_t);

#endif /* I22-SEQUENCER_H_ */