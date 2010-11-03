SBINDIR=/usr/local/sbin

all: iwleeprom

iwleeprom: iwleeprom.c
		gcc -Wall -o iwleeprom iwleeprom.c

clean:
		rm -f iwleeprom

install: iwleeprom
		install -m 4755 iwleeprom $(SBINDIR)

uninstall:
		rm -f $(SBINDIR)/iwleeprom

.PHONY: all clean install uninstall
