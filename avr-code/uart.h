#ifndef UART_H
#define UART_H

#define BAUD 9600UL

void uart_set_recv_handler(void (*handler)(unsigned char c));

void uart_init(void);

void uart_putc(const char c);
void uart_puts(const char* str);

#endif
