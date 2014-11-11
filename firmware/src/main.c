/*
 *	littleBits i22 Sequencer Bit
 *	uC = ATmega168-PA
 *	SysClk = 8 MHz (external crystal)
 *  Author: littleBits Electronics, Inc.
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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/wdt.h>        // Watchdog Timer

#include "seq_io.h"
#include "i22_sequencer.h"

// function prototype
void setup(void);

int main(void)
{
	setup();
	
	while(1)
	{
		clockMode	=	READ_CLOCK_MODE();
		runMode		=	READ_RUN_MODE();

		// SPEED MODE
		if(clockMode == 0)
		{
			// if last mode was STEP mode or the firmware just began
			if(lastClockMode != clockMode) {
				cli();
				DISABLE_STEP_INT();				// Disable pin change interrupt
				ENABLE_COMP_INT();				// Enable compare match interrupt for millis()
				ENABLE_ADC_INT();				// Enable ADC interrupt
				sei();

				SET_CLOCKOUT_LOW();
				speedState = LOW;
				ready_for_adc_sample = 1;		// prepare for an ADC sample
				lastStepUpdate = millis();
			}
			aiFLag = (ADCSRA >> ADIE) & 1;	// check to make sure the adc interrupt flag is set
			eiFlag = (EIMSK >> INT1) & 1;	// check to make sure the external interrupt flag is cleared
			if(aiFLag != 1 || eiFlag == 1) { while(1); }	// if not set, then it is likely memory has been corrupted by a disruptive power condition (attaching a speaker). Wait until the WDT resets the uC.
			
			// run speed mode once we have a new ADC reading
			if(!ready_for_adc_sample) {
				runSpeedMode(runMode);
				ready_for_adc_sample = 1;	// request a new ADC sample
			}
		}
		// STEP MODE
		else
		{
			// if last mode was SPEED mode or the firmware just began
			if(lastClockMode != clockMode) {
				cli();
				DISABLE_COMP_INT();				// Disable compare match interrupt for millis()
				DISABLE_ADC_INT();				// Disable ADC interrupt
				ENABLE_STEP_INT();				// Enable pin change interrupt
				sei();
			}
			aiFLag = (ADCSRA >> ADIE) & 1;		// check to make sure the adc interrupt flag is cleared
			eiFlag = (EIMSK >> INT1) & 1;		// check to make sure the external interrupt flag is set
			if(eiFlag != 1 || aiFLag == 1) { while(1); }	// if not set, then it is likely memory has been corrupted by a disruptive power condition (attaching a speaker). Wait until the WDT resets the uC.
		}
		giFlag = (SREG >> 7) & 1;				// check to make sure the global interrupt flag is set
		if(giFlag != 1) { while(1); }			// if not set, then it is likely the memory has been corrupted by a disruptive power condition (attaching a speaker). Wait until the WDT resets the uC.

		lastClockMode = clockMode;

		// Kick the dog AKA reset the watchdog timer back to zero thereby avoiding an undesired reset
		// Note that if our code hangs in execution somewhere, we won't reach this command, and the watchdog timer
		// will max out, thereby causing a reset and fixing our frozen uC...
		wdt_reset();
	}
}

void setup(void)
{
	INIT_LEDS();
	INIT_RUN_MODE();
	INIT_CLOCK_MODE();
	INIT_CLOCKOUT();
	INIT_PULSE();
	
	// initialize timer for millis() function keep track of system time
	init_millis_timer();

	// Initialize the ADC. It will take readings automatically, calling ISR(ADC_vect) to do so.
	// Use a clock frequency of 1/128nd of sysclock (so 62.5 kHz)
	// A given ADC reading takes 13.5 cycles, so the ADC will have a sample rate of 4.629 kHz.
	init_adc(128);  // initialize ADC

	// select the ADC2 channel (floating, adc pin) and enable the ADC interrupt
	cli();
	ADMUX = 0x42;	// select ADC2
	ENABLE_ADC_INT();
	sei();
	
	ready_for_adc_sample = 1;
	while(ready_for_adc_sample);	// wait for ADC reading to seed the random number generator
	srand(sample_in_int16);			// initialize pseudo-random number generator
	
	// disable the ADC interrupt and select the ADC0 channel (bitsnap input)
	cli();
	DISABLE_ADC_INT();
	ADMUX = 0x40;	// select ADC0
	sei();

	/*
	Note: If the watchdog is accidentally enabled, for example by a runaway pointer or brown-out
	condition, the device will be reset and the watchdog timer will stay enabled. If the code is not set
	up to handle the watchdog, this might lead to an eternal loop of time-out resets. To avoid this sit-
	uation, the application software should always clear the watchdog system reset flag (WDRF)
	and the WDE control bit in the initialization routine, even if the watchdog is not in use.
	*/
	wdt_enable(WDTO_60MS);
	wdt_reset();

	ready_for_adc_sample = 1;
	SET_LED_HIGH(curStep);
}