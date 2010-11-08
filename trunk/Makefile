SBINDIR=/usr/local/sbin
MANDIR=/usr/share/man/man8

all: iwleeprom iwleeprom.8.gz

debug: iwleeprom_debug

iwleeprom_debug: iwleeprom.c
		gcc -Wall -g -o iwleeprom_debug iwleeprom.c

iwleeprom: iwleeprom.c
		gcc -Wall -o iwleeprom iwleeprom.c

iwleeprom.8.gz: iwleeprom.8
		gzip -c iwleeprom.8 > iwleeprom.8.gz

clean:
		rm -f iwleeprom
		rm -f iwleeprom_debug
		rm -f iwleeprom.8.gz

install: all
		install -m 4755 iwleeprom $(SBINDIR)
		install -m 644 iwleeprom.8.gz $(MANDIR)

uninstall:
		rm -f $(SBINDIR)/iwleeprom
		rm -f $(MANDIR)/iwleeprom.8.gz

.PHONY: all debug clean install uninstall
