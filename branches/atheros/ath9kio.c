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

#include "ath9kio.h"

#define ATH_EEPROM_SIGNATURE    0x7801
#define ATH_MMAP_LENGTH 0x10000

/* ATH9K defines */
#define AR5416_EEPROM_S             2
#define AR5416_EEPROM_OFFSET        0x2000

//#define AR_EEPROM_STATUS_DATA                    (AR_SREV_9300_20_OR_LATER(ah) ? 0x4084 : 0x407c)
#define AR_EEPROM_STATUS_DATA                    ((dev->dev > 0x0030) ? 0x4084 : 0x407c)
#define AR_EEPROM_STATUS_DATA_VAL                0x0000ffff
#define AR_EEPROM_STATUS_DATA_VAL_S              0
#define AR_EEPROM_STATUS_DATA_BUSY               0x00010000
#define AR_EEPROM_STATUS_DATA_BUSY_ACCESS        0x00020000
#define AR_EEPROM_STATUS_DATA_PROT_ACCESS        0x00040000
#define AR_EEPROM_STATUS_DATA_ABSENT_ACCESS      0x00080000

#define WAIT_TIMEOUT 10000 /* x10 us */

/* PCI R/W macros */
#define PCI_IN32(dev,a)    (*((volatile unsigned long int *)(dev->mem + (a))))
#define PCI_OUT32(dev,v,a) (*((volatile unsigned long int *)(dev->mem + (a))) = (v))

void ath9k_eeprom_lock(struct pcidev *dev) {}

void ath9k_eeprom_unlock(struct pcidev *dev) {}

const uint16_t ath9k_eeprom_read16(struct pcidev *dev, unsigned int addr)
{
	int32_t data;
	int i;
	memcpy(&data, dev->mem + AR5416_EEPROM_OFFSET + (addr << AR5416_EEPROM_S), 4);

	for(i = 0; i < WAIT_TIMEOUT; i++) {
		usleep(10);
		memcpy(&data, dev->mem + AR_EEPROM_STATUS_DATA, 4);
		if ( 0 == (data & (AR_EEPROM_STATUS_DATA_BUSY | AR_EEPROM_STATUS_DATA_PROT_ACCESS))) {
			data &= AR_EEPROM_STATUS_DATA_VAL;
			return data;
		}
	}
	printf("timeout reading ath9k eeprom at %04x!\n", addr);
	return 0;
}

void ath9k_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value)
{


}

void ath9k_eeprom_patch11n(struct pcidev *dev)
{
	printf("ath9k 802.11n patch not implemented yet.\n");
}

struct dev_ops dev_ops_ath9k = {
	.mmap_size        = ATH_MMAP_LENGTH,
	.eeprom_signature = ATH_EEPROM_SIGNATURE,

	.eeprom_lock     = &ath9k_eeprom_lock,
	.eeprom_unlock   = &ath9k_eeprom_unlock,
	.eeprom_read16   = &ath9k_eeprom_read16,
	.eeprom_write16  = &ath9k_eeprom_write16,
	.eeprom_patch11n = &ath9k_eeprom_patch11n
};

