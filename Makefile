# AVR-GCC Makefile
PROJECT=tiny_alarm
SOURCES=tiny_alarm.c
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=attiny45

CFLAGS=-mmcu=$(MMCU) -Wall -Os


$(PROJECT).hex: $(PROJECT).out
	$(OBJCOPY) -j .text -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES) 
	$(CC) $(CFLAGS) -o $(PROJECT).out $(SOURCES)

program: $(PROJECT).hex
	avrdude -p t45 -c buspirate -P /dev/ttyUSB0 -e -U flash:w:$(PROJECT).hex
clean:
	rm -f $(PROJECT).o
	rm -f $(PROJECT).out
	rm -f $(PROJECT).map
	rm -f $(PROJECT).hex
