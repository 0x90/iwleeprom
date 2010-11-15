/*
****************************************************************************
*
* iwleeprom - EEPROM reader/writer for intel wifi cards.
* Copyright (C) 2010, Alexander "ittrium" Kalinichenko <alexander@kalinichenko.org>
* ICQ: 152322, Skype: ittr1um		
* Copyright (C) 2010, Gennady "ShultZ" Kozlov <qpxtool@mail.ru>
*
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
****************************************************************************
*/

#ifndef iwleeprom_h_included
#define iwleeprom_h_included

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#if BYTE_ORDER == BIG_ENDIAN
#define cpu2le16(x) __bswap_16(x)
#define cpu2be16(x) x
#define le2cpu16(x) __bswap_16(x)
#define be2cpu16(x) x
#elif BYTE_ORDER == LITTLE_ENDIAN
#define cpu2le16(x) x
#define cpu2be16(x) __bswap_16(x)
#define le2cpu16(x) x
#define be2cpu16(x) __bswap_16(x)
#else
#error Unsupported BYTE_ORDER!
#endif

struct pcidev
{
	unsigned int class,
				ven,  dev,
				sven, sdev;
	int 			idx;
	size_t 			eeprom_size;
	char			*device;

	struct dev_ops	*ops;
	unsigned char   *mem;
	bool 			eeprom_locked;
};

struct pci_id
{
	unsigned int	ven, dev;
	bool			writable;
	size_t			eeprom_size;
	struct dev_ops	*ops;
	char name[64];
};

enum byte_order
{
	order_unknown = 0,
	order_be,
	order_le
};

extern unsigned int  debug;
#define EEPROM_SIZE_MIN  0x400
#define EEPROM_SIZE_MAX  0x800

extern uint16_t buf[EEPROM_SIZE_MAX/2];
extern enum byte_order dump_order;
extern bool preserve_mac;
extern bool preserve_calib;

void die(  const char* format, ... ); 


extern const uint16_t buf_read16(unsigned int addr);
extern void buf_write16(unsigned int addr, uint16_t value);

typedef void (*eeprom_lock_t)(struct pcidev *dev);
typedef void (*eeprom_unlock_t)(struct pcidev *dev);
typedef const uint16_t (*eeprom_read16_t)(struct pcidev *dev, unsigned int addr);
typedef void (*eeprom_write16_t)(struct pcidev *dev, unsigned int addr, uint16_t value);

typedef void (*eeprom_patch11n_t)(struct pcidev *dev);

struct dev_ops {
	uint32_t		  mmap_size;
	uint16_t		  eeprom_signature;

	eeprom_lock_t     eeprom_lock;
	eeprom_unlock_t   eeprom_unlock;
	eeprom_read16_t   eeprom_read16;
	eeprom_write16_t  eeprom_write16;
	eeprom_patch11n_t eeprom_patch11n;
};

#endif

