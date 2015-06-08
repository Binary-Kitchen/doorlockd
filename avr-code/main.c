/* 2015, Ralf Ramsauer
 * ralf@binary-kitchen.de
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <string.h>

#define GREEN_ON (PORTB &= ~(1<<PB1))
#define GREEN_OFF (PORTB |= (1<<PB1))

#define RED_ON (PORTB &= ~(1<<PB0))
#define RED_OFF (PORTB |= (1<<PB0))

#define DOORLIGHT_ON (PORTD &= ~(1<<PD6))
#define DOORLIGHT_OFF (PORTD |= (1<<PD6))

#define OPEN_BOLZEN (PORTD &= ~(1<<PD4))
#define CLOSE_BOLZEN (PORTD |= (1<<PD4))

#define OPEN_SCHNAPPER (PORTD |= (1<<PD5))
#define CLOSE_SCHNAPPER (PORTD &= ~(1<<PD5))

#define IS_RESCUE (!(PIND & (1<<PD3)))
#define IS_SWITCH (!(PIND & (1<<PD2)))

static uint8_t open = 0;
static uint8_t klacker = 0;

void open_door()
{
    RED_OFF;
    GREEN_ON;
    OPEN_BOLZEN;
    DOORLIGHT_ON;
}

void close_door()
{
    RED_ON;
    GREEN_OFF;
    DOORLIGHT_OFF;
    CLOSE_SCHNAPPER;
    CLOSE_BOLZEN;
}

void schnapper(unsigned int i)
{
    OPEN_SCHNAPPER;
    while (i--) {
        _delay_ms(1000);
    }
    CLOSE_SCHNAPPER;
    _delay_ms(100);
}

ISR(USI_OVERFLOW_vect)
{
    // Clear interrupt flag
    USISR |= (1<<USIOIF);
    // Reset counter
    TCNT1 = 0;
    open = 1;

    uint8_t in = USIDR;
    // Depending on the clock, a klacker either can be 10101010 or 01010101
    if (in == 0x55 || in == 0xAA)
    {
        klacker = 1;
    }
}

ISR(TIMER1_OVF_vect)
{
    open = 0;
}

int main(void) {
    // disable all interrupts
    cli();

    // PD4-6 as Output
    DDRD = (1<<PD5)|(1<<PD4)|(1<<PD6);
    // PD2,3 Pullup
    PORTD |= (1<<PD2)|(1<<PD3);

    // PB0-1 as Output
    DDRB = (1<<PB0)|(1<<PB1);
    // PB5,7 Pullup
    PORTB |= (1<<PB7)|(1<<PB5);

    // first action: close door
    close_door();

    // Configure USI
    USICR = (1<<USICS1) | (1<<USIOIE) | (1<<USIWM0);

    // Configure Timer
    TIMSK |= (1<<TOIE1);
    TIFR |= (1<<TOV1);
    TCNT1 = 0;
    TCCR1A = 0;
    TCCR1B = (1<<CS01)|(1<<CS00);

    // wait a bit to settle down
    _delay_ms(1000);

    // enable interrupts
    sei();

    for(;;) {
	if (open == 0) {
	    close_door();
	} else if (open == 1) {
	    open_door();
	    if (klacker == 1)
  	    {
	        schnapper(3);
	        klacker = 0;
	    }
        }

        if (IS_RESCUE) {
            cli();
            open_door();
	    schnapper(3);
            sei();
        } else if (IS_SWITCH && open == 0) {
            _delay_ms(300);
            if(IS_SWITCH && open == 0) {
	        cli();
		open_door();
		schnapper(5);
		sei();
            }
	} 
        _delay_ms(50);
    }

    return 0;
}
