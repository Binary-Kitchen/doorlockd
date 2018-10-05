/*
 * doorlock-avr, AVR code of Binary Kitchen's doorlock
 *
 * Copyright (c) Binary Kitchen, 2018
 *
 * Authors:
 *  Ralf Ramsauer <ralf@binary-kitchen.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include "uart.h"

#include <ctype.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define RED	0x1
#define GREEN	0x2
#define YELLOW	0x4

#define SET_CONDITIONAL(predicate, port, pin) \
	if ((predicate)) \
		port |= (1 << pin); \
	else \
		port &= ~(1 << pin);

/* can either be red, green, or yellow */
static unsigned char state = RED;

enum state_source {
	BUTTON,
	COMM,
	TIMEOUT,
	EMERGENCY,
};

static inline void set_schnapper(bool state)
{
	SET_CONDITIONAL(state, PORTB, PB0);
}

static inline void set_bolzen(bool state)
{
	SET_CONDITIONAL(state, PORTB, PB1);
}

static inline void reset_timeout(void)
{
	TCNT1 = 0;
}

static void set_leds(void)
{
	static unsigned int counter = 0;
	bool pwm_cycle = ++counter % 20;

	if (pwm_cycle) {
		PORTD &= ~((1 << PD5) | (1 << PD6));
		PORTB &= ~(1 << PB4);
	}

	switch (state) {
	case RED:
		PORTD |= (1 << PD5);
		break;
	case YELLOW:
		PORTD |= (1 << PD6);
		break;
	case GREEN:
		PORTB |= (1 << PB4);
		break;
	}

	if (pwm_cycle)
		return;

	switch (state) {
	case RED:
		PORTD ^= (1 << PD6);
		PORTB ^= (1 << PB4);
		break;
	case YELLOW:
		PORTD ^= (1 << PD5);
		PORTB ^= (1 << PB4);
		break;
	case GREEN:
		PORTD ^= (1 << PD5);
		PORTD ^= (1 << PD6);
		break;
	}
}

static void update_state(unsigned char new_state, enum state_source source)
{
	char ret = 0;
	reset_timeout();

	if (new_state == state)
		return;
	state = new_state;

	switch (state) {
	case RED:
		set_bolzen(true);
		set_schnapper(false);
		ret = 'r';
		break;
	case YELLOW:
		set_bolzen(false);
		set_schnapper(false);
		ret = 'y';
		break;
	case GREEN:
		set_bolzen(false);
		set_schnapper(true);
		ret = 'g';
		break;
	}

	switch (source) {
	case BUTTON:
		uart_putc(toupper(ret));
		break;
	case EMERGENCY:
		uart_putc('E');
		break;
	case TIMEOUT:
		uart_putc(ret);
		break;
	case COMM:
	default:
		break;
	}
}

ISR(USART_RX_vect)
{
	unsigned char c = UDR;

	switch (c) {
	case 'r':
		update_state(RED, COMM);
		break;
	case 'y':
		update_state(YELLOW, COMM);
		break;
	case 'g':
		update_state(GREEN, COMM);
		break;
	}
}

ISR(TIMER1_OVF_vect)
{
	reset_timeout();
	update_state(RED, TIMEOUT);
}

static inline void timer_init(void)
{
	TIMSK |= (1 << TOIE1);
	TIFR |= (1 << TOV1);
	TCCR1A = 0;
	TCCR1B = (1 << CS12);
}

static inline void setup_ports(void)
{
	PORTB = 0;
	DDRB = (1 << PB4) | (1 << PB1) | (1 << PB0);

	PORTD = 0;
	DDRD = (1 << PD5) | (1 << PD6);
}

static inline bool is_emergency(void)
{
	return !(PINB & (1 << PB3));
}

static inline bool is_door_open(void)
{
	return !(PINB & (1 << PB2));
}

static unsigned char get_keys(void)
{
	unsigned char ret = 0;

	if (!(PIND & (1 << PD2))) ret |= RED;
	if (!(PIND & (1 << PD3))) ret |= YELLOW;
	if (!(PIND & (1 << PD4))) ret |= GREEN;

	return ret;
}

int main(void)
{
	unsigned char i;

	setup_ports();
	timer_init();
	uart_init();

	set_bolzen(true);
	reset_timeout();

	sei();

	for (;;) {
		if (is_emergency()) {
			update_state(GREEN, EMERGENCY);
		} else {
			i = get_keys();
			if (i & GREEN)
				update_state(GREEN, BUTTON);
			else if (i & YELLOW)
				update_state(YELLOW, BUTTON);
			else if (i & RED)
				update_state(RED, BUTTON);
		}
		set_leds();
	}
}
