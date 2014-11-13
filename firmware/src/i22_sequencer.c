/*
 * i22_sequencer.c
 *
 * Created: 11/6/2013 11:16:23 AM
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
 
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *	Copyright 2013
 *	source: http://www.dsprelated.com/showcode/304.php
 *	
 *	The code is released under Creative Commons Attribution 3.0. See the license
 * 	at <https://creativecommons.org/licenses/by/3.0/us/legalcode> for more details.
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/atomic.h>
#include "i22_sequencer.h"
#include "seq_io.h"

volatile int8_t curStep = 0;
int16_t curSpeed = 0;
unsigned long lastStepUpdate = 0;
uint8_t speedState = 0;
volatile int8_t pendDirection = 1;

int8_t clockMode = 0;
int8_t lastClockMode = -1;
uint8_t runMode = 0;

uint16_t aiFLag = 0;	// ADC interrupt flag
uint16_t eiFlag = 0;	// external interrupt flag
uint16_t giFlag = 0;	// global interrupt flag

// Calculate the value needed for
// the CTC match value in OCR1A.
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)

volatile unsigned long timer1_millis;

// This variable will take in our ADC readings... it's filled in the ADC ISR
// Note we have a flag (8-bit variable) called "adc_reading_occurred" that will prevent the ISR from re-filling this
// variable until we've successfully iterated through while(1) once with the given sample
volatile int16_t sample_in_int16;

// This is the averaged output of our LPF
static volatile int16_t sample_out_int16;

// This is the unipolar 8-bit sample between 0 and 256 that we receive after lowpass filtering...
static volatile uint16_t filtered_sample;

// This variable functions as a flag to indicate that it's time to take in another ADC sample.
// The ADC ISR watches for it, and only takes in a new ADC reading if the flag is set HIGH. Once it takes the
// new sample in, it sets the flag LOW and waits again...
// Meanwhile in our main while() loop, we run DSP on incoming samples. When finished processing a sample, we set
// the flag HIGH there to indicate that we're ready to grab a new sample from ADC ISR
volatile uint8_t ready_for_adc_sample = 0;

// ADC interrupt
ISR(ADC_vect)
{
	if(ready_for_adc_sample)
	{
		// Recall that our AVR ADC's return 10-bit wide unipolar ADC samples, ranging from 0-1023.
		// First perform a subtraction to move each reading into the range of -512 to 512, then upscale it into the 16-bit range.
		// The result is a signed 16-bit sample ready for DSP.
		sample_in_int16 = (ADC - 0x200) << 6;
		
		// Set the flag low to indicate that we have a fresh reading waiting to be filtered in the while(1) loop
		ready_for_adc_sample = 0;
	}
}

// EXTERNAL INTERRUPT: called whenever INT1 (PD3) changes state
ISR(INT1_vect)
{
	// RISING edge
	if(READ_PULSE())
	{
		// ignore first ext. interrupt on mode change as this is likely not an external change but an automatic call when the interrupt is re-enabled
		if(clockMode != lastClockMode)
		return;

		SET_LED_LOW(curStep);

		// change step value
		switch(runMode)
		{
			// select step mode (FORWARD, BACKWARD, PENDULUM, RANDOM)
			case 0:  // BACKWARD
			curStep = (curStep - 1) & 7;    // decrement and bound
			break;
			case 1:  // FORWARD
			curStep = (curStep + 1) & 7;    // increment and bound
			break;
			case 2:  // RANDOM
			curStep = rand() & 7;			// generate random number and bound
			break;
			case 3:  // PENDULUM
			if(curStep + pendDirection < 0 || curStep + pendDirection > 7) {
				pendDirection *= -1;
			}
			curStep += pendDirection;
			break;
		};

		SET_CLOCKOUT_HIGH();
		SET_LED_HIGH(curStep);
	}
	// FALLING edge
	else {
		SET_CLOCKOUT_LOW();
	}
}

void runSpeedMode(int8_t mode)
{	
	// DSP happens now:
	// Lowpass filter our incoming ADC samples with a 1st-order exponential moving average IIR lowpass filter
	sample_out_int16 = dsp_ema_i16(sample_in_int16, sample_out_int16, DSP_EMA_I16_ALPHA(0.1));
	
	/// First pull the most recently filtered sample from our ADC stream:
	// Using addition, offset our reading up to the unipolar range 0 - 65535, then downscale it into the 10-bit output range.
	// The result is an unsigned 10-bit value between 0 and 1023 that is appropriate for the Number Bit display calculations:
	filtered_sample = (sample_out_int16 + 0x8000) >> 6;
	
	int16_t tmp = constrain(filtered_sample, 0, 1010);
	tmp -= 10;
	curSpeed = 2000 - (tmp << 1);
	
	// if close to a 0V SIG (above max time), pause the clock
	if(curSpeed > 2000)
	return;
	
	// Keep speed above minimum of 13mS (~77Hz)
	if(curSpeed < 13) curSpeed = 13;

	// We've now calculated a new potential sequence speed, called "curSpeed", which will
	// essentially vary inversely with the incoming ADC readings.
	
	// This newly calculated speed will only take effect when:
	// the # ms passed since our last step update is >= 1/2 of this new speed
	if(millis() - lastStepUpdate >= (curSpeed >> 1))
	{
		
		if(speedState == HIGH)  // creating FALLING EDGE
		{
			SET_CLOCKOUT_LOW();
			speedState = LOW;
			lastStepUpdate = millis();
		}
		else // assume (speedState == LOW) ==> creating RISING EDGE
		{
			SET_LED_LOW(curStep);
			
			// change step value
			switch(runMode)
			{
				// select step mode (FORWARD, BACKWARD, PENDULUM, RANDOM)
				case 0:  // BACKWARD
				curStep = (curStep - 1) & 7;    // decrement and bound
				break;
				case 1:  // FORWARD
				curStep = (curStep + 1) & 7;    // increment and bound
				break;
				case 2:  // RANDOM
				curStep = rand() & 7;
				break;
				case 3:  // PENDULUM
				if(curStep + pendDirection < 0 || curStep + pendDirection > 7)
				pendDirection *= -1;
				curStep += pendDirection;
				break;
			};
			
			SET_CLOCKOUT_HIGH();
			SET_LED_HIGH(curStep);
			
			speedState = HIGH;
			
			lastStepUpdate = millis();
			
		} // end rising edge creation
		
	} // end if(millis() - lastStepUpdate >= (curSpeed >> 1))
	
} // end runSpeedMode()

void init_adc(uint8_t adc_clk_prescalar)
{
	//The first step is to set the prescalar for the ADC clock frequency:
	ADCSRA &= ~( (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) ) ;	// clear the values initially

	switch(adc_clk_prescalar)									// then set the proper bits high
	{
		case 2:
		ADCSRA |= (1 << ADPS0);
		break;

		case 4:
		ADCSRA |= (1 << ADPS1);
		break;

		case 8:
		ADCSRA |= ( (1 << ADPS0) | (1 << ADPS1) );
		break;

		case 16:
		ADCSRA |= (1 << ADPS2);
		break;

		case 32:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS0) );
		break;

		case 64:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS1) );
		break;

		case 128:
		ADCSRA |= ( (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) );
		break;

		default:
		ADCSRA |= (1 << ADPS0);
		break;

	} // end switch()

	// Set reference voltage to AVcc, and configure ADC0 for action!
	//ADMUX = 0x40;							        // ADMUX = 0100 0000;
	// ADMUX |= (1 << ADLAR); 						// Left adjust ADC result to allow easy 8 bit reading
	ADCSRA |= (1 << ADATE);  						// Set ADC to Free-Running Mode
	// (we can do this because we just use one ADC
	// channel consistently)

	//ADCSRA |= (1 << ADIE); 						// Enable ADC interrupt
	//sei();										// Enable global interrupts

	ADCSRA |= (1 << ADEN);  						// Enable the ADC
	ADCSRA |= (1 << ADSC);  						// Start A2D Conversions
} // end init_adc()

void init_millis_timer(void)
{
	// Configure timer for millis() function
	// CTC mode, Clock/8
	TCCR1B |= (1 << WGM12) | (1 << CS11);
	// Load the high byte, then the low byte
	// into the output compare
	OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	OCR1AL = (uint8_t)CTC_MATCH_OVERFLOW;
	
	// Enable the compare match interrupt
	//ENABLE_COMP_INT();
}

ISR (TIMER1_COMPA_vect)
{
	timer1_millis++;
}

unsigned long millis ()
{
	unsigned long millis_return;
	
	// Ensure this cannot be disrupted
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		millis_return = timer1_millis;
	}
	return millis_return;
}

// This is my modified version of original source code from: http://www.dsprelated.com/showcode/304.php
// This is a first order IIR lowpass filter, called an Exponential Moving Average filter.
// Decreasing the value of alpha lowers the cutoff frequency of the filter
int16_t dsp_ema_i16(int16_t in, int16_t average, uint8_t alpha)
{
	int32_t tmp0; // calcs must be done in 32-bit math to avoid overflow
	tmp0 = (int32_t)in * (alpha) + (int64_t)average * (256 - alpha);
	return (int16_t)((tmp0 + 128) / 256); // scale back to 32-bit (with rounding)
}