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

#define OPEN_BOLZEN (PORTD &= ~(1<<PD4))
#define CLOSE_BOLZEN (PORTD |= (1<<PD4))

#define OPEN_SCHNAPPER (PORTD |= (1<<PD5))
#define CLOSE_SCHNAPPER (PORTD &= ~(1<<PD5))

#define IS_RESCUE (!(PIND & (1<<PD3)))
#define IS_SWITCH (!(PIND & (1<<PD2)))

#define DE_RX (PORTD &= ~(1<<PD2))
#define DE_TX (PORTD |= (1<<PD2))

#define IS_SCK (!(PINB & (1<<PB7)))

uint8_t open = 0;

void open_door()
{
    RED_OFF;
    GREEN_ON;
    OPEN_BOLZEN;
    _delay_ms(200);
}

void schnapper(unsigned int i)
{
    while(i--)
    {
       OPEN_SCHNAPPER;
       _delay_ms(125);
       CLOSE_SCHNAPPER;
       _delay_ms(125);
    }
}

void close_door()
{
    RED_ON;
    GREEN_OFF;
    CLOSE_SCHNAPPER;
    CLOSE_BOLZEN;
    RED_ON;
}

uint8_t klacker = 0;

ISR(USI_OVERFLOW_vect)
{
    USISR = (1<<USIOIF);
    open = 1;
    TCNT1 = 0;
    klacker = USIDR ? 0 : 1;
}

ISR(TIMER1_OVF_vect)
{
    open = 0;
}

int main(void) {
    // disable all interrupts
    cli();

    // PD5, PD4 Output
    DDRD = (1<<PD5)|(1<<PD4);

    DDRB = (1<<PB0)|(1<<PB1);
    PORTB |= (1<<PB7)|(1<<PB5);

    // PD2,3 as input + pull up
    DDRD &= ~((1<<PD2)|(1<<PD3));
    PORTD |= (1<<PD3)|(1<<PD3);

    USICR = (1<<USICS1) | (1<<USIOIE) | (1<<USIWM0);

    TIMSK |= (1<<TOIE1);
    TIFR |= (1<<TOV1);
    TCNT1 = 0;
    TCCR1A = 0;
    TCCR1B = (1<<CS01)|(1<<CS00);

    _delay_ms(1000);

    CLOSE_BOLZEN;
    CLOSE_SCHNAPPER;
    GREEN_OFF;
    RED_ON;

    _delay_ms(1000);

    sei();

    for(;;) {
	if (open == 0) {
	    close_door();
	} else if (open == 1) {
	    open_door();
	}

	if (open == 1 && klacker == 1)
	{
	    schnapper(20);
	    klacker = 0;
	}

        if (IS_RESCUE) {
            cli();
            open_door();
	    schnapper(30);
            _delay_ms(2000);
            sei();
        } else if (IS_SWITCH && open == 0) {
            _delay_ms(300);
            if(IS_SWITCH && open == 0) {
	        cli();
		open_door();
		schnapper(20);
		_delay_ms(3000);
		sei();
            }
	} 
        _delay_ms(50);
    }

    return 0;
}
