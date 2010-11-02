SBINDIR=/usr/local/sbin

all: iwleeprom

iwleeprom: iwleeprom.c
		gcc -o iwleeprom iwleeprom.c

clean:
		rm -f iwleeprom

install: iwleeprom
		install -m 755 iwleeprom $(SBINDIR)

uninstall:
		rm -f $(SBINDIR)/iwleeprom

.PHONY: all clean install uninstall
