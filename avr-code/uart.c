#include <stddef.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "uart.h"

#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD)
 
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Choose another crystal. Baud error too high.
#endif

static void (*recv_handler)(unsigned char c) = NULL;

ISR(USART_RX_vect)
{
	cli();
	if(recv_handler)
		recv_handler(UDR);
	sei();
}

void uart_init()
{
	// Enable receive and transmit
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);

	// Asynchronous mode, 8N1
	UCSRC = (1<<UCSZ1)|(1<<UCSZ0);

	UBRRH = UBRR_VAL >> 8;
	UBRRL = UBRR_VAL & 0xFF;	
}

void uart_set_recv_handler(void (*handler)(unsigned char c))
{
	recv_handler = handler;
}

void uart_putc(const char c)
{
	while ( !(UCSRA & (1<<UDRE)) );
	UDR = (const unsigned char)c;
}

void uart_puts(const char* str)
{
	do {
		uart_putc(*str);
	} while(*++str);
}
