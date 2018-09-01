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
static unsigned char state;

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

static void update_state(unsigned char new_state)
{
	reset_timeout();

	if (new_state == state)
		return;

	state = new_state;
	switch (state) {
	case RED:
		set_bolzen(false);
		set_schnapper(false);
		uart_putc('r');
		break;
	case YELLOW:
		set_bolzen(true);
		set_schnapper(false);
		uart_putc('y');
		break;
	case GREEN:
		set_bolzen(true);
		set_schnapper(true);
		uart_putc('g');
		break;
	}
}

ISR(USART_RX_vect)
{
	unsigned char c = UDR;
	bool respond = true;

	switch (c) {
	case 'r':
		update_state(RED);
		break;
	case 'y':
		update_state(YELLOW);
		break;
	case 'g':
		update_state(GREEN);
		break;
	default:
		respond = false;
		break;
	}

	if (respond)
		uart_putc(c);
}

ISR(TIMER1_OVF_vect)
{
	reset_timeout();
	update_state(RED);
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

	update_state(RED);
	reset_timeout();

	sei();

	for (;;) {
		i = get_keys();
		if (i & GREEN) {
			uart_putc('G');
			update_state(GREEN);
		} else if (i & YELLOW) {
			uart_putc('Y');
			update_state(YELLOW);
		} else if (i & RED) {
			uart_putc('R');
			update_state(RED);
		}
		while (get_keys())
			reset_timeout();
		set_leds();
	}
}
