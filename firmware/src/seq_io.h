/*
 * seq_io.h
 *
 * Created: 11/1/2013 2:19:24 PM
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

#ifndef SEQ_IO_H_
#define SEQ_IO_H_

// STEP LED PINS
extern const uint8_t stepPins[];
// STEP LED PORTS
extern volatile uint8_t* stepPinsPort[];
// STEP LED PIN DIRECTION
extern volatile uint8_t* stepPinsDir[];

#define CLOCKOUT				PB2
#define clockModePin			PD4
#define runModePin1				PC3
#define runModePin2				PD2
#define PULSE_IN				PD3

#define HIGH					1
#define LOW						0

#define ENABLE_STEP_INT()		{EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (1 << ISC10); EIMSK |= (1 << INT1);}
#define DISABLE_STEP_INT()		(EIMSK &= ~(1 << INT1))

#define ENABLE_ADC_INT()		(ADCSRA |= (1 << ADIE))
#define DISABLE_ADC_INT()		(ADCSRA &= ~(1 << ADIE))

#define INIT_RUN_MODE()			{DDRC &= ~(1 << runModePin1); DDRD &= ~(1 << runModePin2); PORTC |= (1 << runModePin1); PORTD |= (1 << runModePin2);}
#define READ_RUN_MODE()			((((PIND >> runModePin2) & 1) << 1) | ((PINC >> runModePin1) & 1))

#define ENABLE_COMP_INT()		(TIMSK1 |= (1 << OCIE1A))
#define DISABLE_COMP_INT()		(TIMSK1 &= ~(1 << OCIE1A))

#define INIT_CLOCK_MODE()		{DDRD &= ~(1 << clockModePin); PORTD |= (1 << clockModePin);}	// sets pull-up resistor
#define READ_CLOCK_MODE()		((PIND >> clockModePin) & 1)

#define INIT_CLOCKOUT()			(DDRB |= (1 << CLOCKOUT))
#define SET_CLOCKOUT_HIGH()		(PORTB |= (1 << CLOCKOUT))
#define SET_CLOCKOUT_LOW()		(PORTB &= ~(1 << CLOCKOUT))

#define INIT_PULSE()			(DDRD &= ~(1 << PULSE_IN))
#define READ_PULSE()			(PIND & (1 << PULSE_IN))	

#define INIT_LEDS()				for(int i = 0; i < 8; i++) *stepPinsDir[i] |= (1 << stepPins[i])
#define SET_LED_LOW(INDEX)		(*stepPinsPort[INDEX] &= ~(1 << stepPins[INDEX]))
#define SET_LED_HIGH(INDEX)		(*stepPinsPort[INDEX] |= (1 << stepPins[INDEX]))

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#endif /* SEQ_IO_H_ */