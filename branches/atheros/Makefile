SBINDIR=/usr/local/sbin
MANDIR=/usr/share/man/man8
#CXXFLAGS+=-g

all: iwleeprom iwleeprom.8.gz

debug: iwleeprom_debug

iwleeprom_debug: iwlio_debug.o ath5kio_debug.o ath9kio_debug.o iwleeprom_debug.o
		gcc -Wall -g -o iwleeprom_debug iwleeprom_debug.o iwlio_debug.o ath5kio_debug.o ath9kio_debug.o

iwleeprom_debug.o: iwleeprom.h iwlio.h ath5kio.h ath9kio.h iwleeprom.c
		gcc -Wall -g -c -o iwleeprom_debug.o iwleeprom.c

iwlio_debug.o: iwleeprom.h iwlio.h iwlio.c
		gcc -Wall -g -c -o iwlio_debug.o iwlio.c

ath5kio_debug.o: iwleeprom.h ath5kio.h ath5kio.c
		gcc -Wall -g -c -o ath5kio_debug.o ath5kio.c

ath9kio_debug.o: iwleeprom.h ath9kio.h ath9kio.c
		gcc -Wall -g -c -o ath9kio_debug.o ath9kio.c

iwleeprom: iwlio.o ath5kio.o ath9kio.o iwleeprom.o
		gcc -Wall -o iwleeprom iwleeprom.o iwlio.o ath5kio.o ath9kio.o

iwleeprom.o: iwleeprom.h iwlio.h ath5kio.h ath9kio.h iwleeprom.c
		gcc -Wall -c -o iwleeprom.o iwleeprom.c

iwlio.o: iwleeprom.h iwlio.h iwlio.c
		gcc -Wall -c -o iwlio.o iwlio.c

ath5kio.o: iwleeprom.h ath5kio.h ath5kio.c
		gcc -Wall -c -o ath5kio.o ath5kio.c

ath9kio.o: iwleeprom.h ath9kio.h ath9kio.c
		gcc -Wall -c -o ath9kio.o ath9kio.c

iwleeprom.8.gz: iwleeprom.8
		gzip -c iwleeprom.8 > iwleeprom.8.gz

clean:
		rm -f iwleeprom
		rm -f iwleeprom_debug
		rm -f iwleeprom.8.gz
		rm -f *.o
		rm -f *~

install: all
		install -m 4755 iwleeprom $(SBINDIR)
		install -m 644 iwleeprom.8.gz $(MANDIR)

uninstall:
		rm -f $(SBINDIR)/iwleeprom
		rm -f $(MANDIR)/iwleeprom.8.gz

.PHONY: all debug clean install uninstall

