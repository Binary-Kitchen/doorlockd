/* 2015, Ralf Ramsauer
 * ralf@binary-kitchen.de
 */

#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "io.h"
#include "uart.h"

#include "../doorcmds.h"

static volatile enum {LOCKED, UNLOCKED} state = LOCKED;
static volatile bool schnapper = false;

static inline void timer_init(void)
{
	// Config the timer
	// The 16bit Timer1 is used for resetting the lock state,
	// if the UART stops receiving the unlock command
	TIMSK |= (1<<TOIE1);
	TIFR |= (1<<TOV1);
	TCCR1A = 0;
	TCCR1B = (1<<CS12);
}

static inline void reset_timeout(void)
{
	TCNT1 = 0;
}

static inline void extint_init(void)
{
	// Configure external interrupts
	// External interrupts are used for Button Unlock an Lock
	MCUCR = (1<<ISC11)|(1<<ISC01);
	GIMSK |= (1<<INT0)|(1<<INT1);
}

void uart_handler(const unsigned char c)
{
	char retval = c;
	switch ((char)c) {
		case DOOR_CMD_UNLOCK:
			state = UNLOCKED;
			reset_timeout();
			break;

		case DOOR_CMD_LOCK:
			state = LOCKED;
			break;

		case DOOR_CMD_PING:
			break;

		case DOOR_CMD_SCHNAPER:
			if (state == UNLOCKED)
				schnapper = true;
			else
				retval = '?';
			break;

		case DOOR_CMD_STATUS:
			retval = (state == LOCKED) ? 'l' : 'u';
			break;

		default:
			retval = '?';
			break;
	}

	uart_putc(retval);
}

// Timeroverflow interrupts occurs each 1.137 seconds
// UART receive interrupts is used to prevent timer overflows
ISR(TIMER1_OVF_vect)
{
	state = LOCKED;
}

// Button Lock
ISR(INT0_vect)
{
	cli();

	// This code is used to prevent spurious interrupts
	_delay_ms(50);
	if (!is_button_lock())
		goto out;

	uart_putc(DOOR_BUTTON_LOCK);
	state = LOCKED;

out:
	sei();
}

// Button Unlock
ISR(INT1_vect)
{
	cli();

	// This code is used to prevent spurious interrupts
	_delay_ms(50);
	if (!is_button_unlock())
		goto out;

	uart_putc(DOOR_BUTTON_UNLOCK);

    if (state == LOCKED) {
		bolzen_off();
		_delay_ms(3000);
	}

out:
	sei();
}

int main(void)
{
	// Disable all interrupts
	cli();
	// Init IO
	io_init();

	// Wait a bit to settle down
	_delay_ms(1000);

	// Init Uart
	uart_init();
	uart_set_recv_handler(uart_handler);

	// Init Timer
	timer_init();
	reset_timeout();

	// Init external interrupts
	extint_init();

	// Enable all interrupts
	sei();

	for(;;) {
		if (state == LOCKED) {
			bolzen_on();
			status_off();
			schnapper = false;

			// Check if someone used the emergency unlock
			if (is_emergency_unlock()) {

				// If so, wait 200ms and double check
				_delay_ms(200);
				if (is_emergency_unlock()) {
					uart_putc(DOOR_EMERGENCY_UNLOCK);
					cli();

					bolzen_off();
					schnapper_on();

					_delay_ms(3000);

					schnapper_off();
					bolzen_on();

					sei();
				}
			}
		} else if (state == UNLOCKED) {
			bolzen_off();
			status_on();
			if (schnapper == true) {
				schnapper = false;
				schnapper_on();
				_delay_ms(2000);
				schnapper_off();
			}
		}
	}

	return 0;
}
