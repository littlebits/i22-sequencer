/*
 * seq_io.c
 *
 * Created: 11/6/2013 11:28:18 AM
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

#include <avr/io.h>

// STEP LED PINS
const uint8_t stepPins[] = {
	PD5, PD6, PD7, PB0, PC4, PC5, PD0, PD1
};
// STEP LED PORTS
volatile uint8_t* stepPinsPort[] = {
	&PORTD, &PORTD, &PORTD, &PORTB, &PORTC, &PORTC, &PORTD, &PORTD
};
// STEP LED PIN DIRECTION
volatile uint8_t* stepPinsDir[] = {
	&DDRD, &DDRD, &DDRD, &DDRB, &DDRC, &DDRC, &DDRD, &DDRD
};