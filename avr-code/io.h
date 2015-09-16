#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <avr/io.h>

/*
 * Macros
 */
#define PIN(x) (*(&x - 2)) // Address Of Data Direction Register Of Port x 
#define DDR(x) (*(&x - 1)) // Address Of Input Register Of Port x

/*
 * Outputs
 */
#define PORT_BOLZEN    PORTB
#define PIN_BOLZEN     PB0

#define PORT_SCHNAPPER PORTB
#define PIN_SCHNAPPER  PB1

#define PORT_STATUS    PORTB
#define PIN_STATUS     PB2

/*
 * Inputs
 */
#define PORT_BUTTON_LOCK      PORTD
#define PIN_BUTTON_LOCK		  PD2

#define PORT_BUTTON_UNLOCK    PORTD
#define PIN_BUTTON_UNLOCK	  PD3

#define PORT_EMERGENCY_UNLOCK PORTD
#define PIN_EMERGENCY_UNLOCK  PD4

static inline bool is_emergency_unlock(void)
{
	return !(PIN(PORT_EMERGENCY_UNLOCK) & (1<<PIN_EMERGENCY_UNLOCK));
}

static inline bool is_button_unlock(void)
{
	return !(PIN(PORT_BUTTON_UNLOCK) & (1<<PIN_BUTTON_UNLOCK));
}

static inline bool is_button_lock(void)
{
	return !(PIN(PORT_BUTTON_LOCK) & (1<<PIN_BUTTON_LOCK));
}


static inline void schnapper_off(void)
{
	PORT_SCHNAPPER &= ~(1<<PIN_SCHNAPPER);
}

static inline void schnapper_on(void)
{
	PORT_SCHNAPPER |= (1<<PIN_SCHNAPPER);
}

static inline void bolzen_off(void)
{
	PORT_BOLZEN &= ~(1<<PIN_BOLZEN);
}

static inline void bolzen_on(void)
{
	PORT_BOLZEN |= (1<<PIN_BOLZEN);
}

static inline void status_off(void)
{
	PORT_STATUS &= ~(1<<PIN_STATUS);
}

static inline void status_on(void)
{
	PORT_STATUS |= (1<<PIN_STATUS);
}

static inline void io_init(void)
{
	// Set output directions
	DDR(PORT_SCHNAPPER) |= (1<<PIN_SCHNAPPER);
	schnapper_off();

	DDR(PORT_SCHNAPPER) |= (1<<PIN_BOLZEN);
	bolzen_off();

	DDR(PORT_STATUS) |= (1<<PIN_STATUS);
	status_off();

	// Set input directions and activate pull-ups
	DDR(PORT_BUTTON_UNLOCK) &= ~(1<<PIN_BUTTON_UNLOCK);
	PORT_BUTTON_UNLOCK |= (1<<PIN_BUTTON_UNLOCK);

	DDR(PORT_BUTTON_LOCK) &= ~(1<<PIN_BUTTON_LOCK);
	PORT_BUTTON_LOCK |= (1<<PIN_BUTTON_LOCK);
	
	DDR(PORT_EMERGENCY_UNLOCK) &= ~(1<<PIN_EMERGENCY_UNLOCK);
	PORT_EMERGENCY_UNLOCK |= (1<<PIN_EMERGENCY_UNLOCK);
}

#endif
