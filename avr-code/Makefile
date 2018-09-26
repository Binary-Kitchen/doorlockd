# Doorlock AVR - The AVR part of doorlockd
#
# Copyright (c) Binary Kitchen e.V., 2018
#
# Authors:
#  Ralf Ramsauer <ralf@binary-kitchen.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.


TARGET = doorlock

MCU ?= attiny2313a
F_OSC ?= 4000000
UART_BAUD ?= 9600
AVRDUDE_MCU ?= t2313

PROGRAMMER=linuxspi
PORT=/dev/spidev0.0:/dev/gpiochip0:25
SPEED=125000

OBJS = main.o uart.o

CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

CFLAGS := -g -O2 -mmcu=$(MCU)
CFLAGS += -Wall -Wextra -Wstrict-prototypes
CFLAGS += -DF_OSC=$(F_OSC) -DF_CPU=F_OSC -DUART_BAUD=$(UART_BAUD)UL -DMCU=$(MCU)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET).elf $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $^ $@

program: $(TARGET).hex
	$(AVRDUDE) -p $(AVRDUDE_MCU) -c $(PROGRAMMER) -P $(PORT) -U flash:w:$^

fuse:
	$(AVRDUDE) -p $(AVRDUDE_MCU) -c $(PROGRAMMER) -P $(PORT) -b $(SPEED) -U lfuse:w:0xfd:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

clean:
	rm -f $(OBJS)
	rm -f $(TARGET).elf $(TARGET).hex
