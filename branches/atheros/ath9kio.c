/*
****************************************************************************
*
* iwleeprom - EEPROM reader/writer for intel wifi cards.
* Copyright (C) 2010, Alexander "ittrium" Kalinichenko <alexander@kalinichenko.org>
* ICQ: 152322, Skype: ittr1um		
* Copyright (C) 2010, Gennady "ShultZ" Kozlov <qpxtool@mail.ru>
*
*
* some values and HW identify code got from Atheros ath9k Linux driver
* Copyright (c) 2008-2010 Atheros Communications Inc.
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

#define ATH9K_EEPROM_SIZE           0x1000
#define ATH_EEPROM_SIGNATURE        0x7801
#define ATH_MMAP_LENGTH            0x10000


#define AR5416_EEPROM_S             2
#define AR5416_EEPROM_OFFSET        0x2000


#define AR_SREV_VERSION_5416_PCI    0x00D
#define AR_SREV_VERSION_5416_PCIE   0x00C
#define AR_SREV_VERSION_9100        0x014
#define AR_SREV_VERSION_9160        0x040
#define AR_SREV_VERSION_9280        0x080
#define AR_SREV_VERSION_9285        0x0C0
#define AR_SREV_VERSION_9287        0x180
#define AR_SREV_VERSION_9271        0x140
#define AR_SREV_VERSION_9300        0x1c0
#define AR_SREV_REVISION_9300_20    2 /* 2.0 and 2.1 */

#define AR_SREV_9100 \
	(macVer == AR_SREV_VERSION_9100)
#define AR_SREV_9300_20_OR_LATER \
	((macVer > AR_SREV_VERSION_9300) || \
	 ((macVer == AR_SREV_VERSION_9300) && \
	  (macRev >= AR_SREV_REVISION_9300_20)))

#define AR_SREV                     ((AR_SREV_9100) ? 0x0600 : 0x4020)
#define AR_SREV_ID                  ((AR_SREV_9100) ? 0x00000FFF : 0x000000FF)
#define AR_SREV_VERSION             0x000000F0
#define AR_SREV_VERSION_S           4
#define AR_SREV_REVISION            0x00000007
#define AR_SREV_VERSION2            0xFFFC0000
#define AR_SREV_VERSION2_S          18
#define AR_SREV_TYPE2               0x0003F000
#define AR_SREV_TYPE2_S             12
#define AR_SREV_TYPE2_CHAIN         0x00001000
#define AR_SREV_TYPE2_HOST_MODE     0x00002000
#define AR_SREV_REVISION2           0x00000F00
#define AR_SREV_REVISION2_S         8


#define AR_GPIO_IN_OUT              0x4048
#define AR_GPIO_OE_OUT              (AR_SREV_9300_20_OR_LATER ? 0x4050 : 0x404c)
#define AR_GPIO_OE_OUT_DRV          0x3
#define AR_GPIO_OE_OUT_DRV_NO       0x0
#define AR_GPIO_OE_OUT_DRV_LOW      0x1
#define AR_GPIO_OE_OUT_DRV_HI       0x2
#define AR_GPIO_OE_OUT_DRV_ALL      0x3

#define AR_GPIO_OUTPUT_MUX1                      (AR_SREV_9300_20_OR_LATER ? 0x4068 : 0x4060)

#define AR_EEPROM_STATUS_DATA                    (AR_SREV_9300_20_OR_LATER ? 0x4084 : 0x407c)
#define AR_EEPROM_STATUS_DATA_VAL                0x0000ffff
#define AR_EEPROM_STATUS_DATA_VAL_S              0
#define AR_EEPROM_STATUS_DATA_BUSY               0x00010000
#define AR_EEPROM_STATUS_DATA_BUSY_ACCESS        0x00020000
#define AR_EEPROM_STATUS_DATA_PROT_ACCESS        0x00040000
#define AR_EEPROM_STATUS_DATA_ABSENT_ACCESS      0x00080000

static struct {
	uint32_t version;
	const char * name;
} ath_mac_bb_names[] = {
	/* Devices with external radios */
	{ AR_SREV_VERSION_5416_PCI,  "5416" },
	{ AR_SREV_VERSION_5416_PCIE, "5418" },
	{ AR_SREV_VERSION_9100,      "9100" },
	{ AR_SREV_VERSION_9160,      "9160" },
	/* Single-chip solutions */
	{ AR_SREV_VERSION_9280,      "9280" },
	{ AR_SREV_VERSION_9285,      "9285" },
	{ AR_SREV_VERSION_9287,      "9287" },
	{ AR_SREV_VERSION_9271,      "9271" },
	{ AR_SREV_VERSION_9300,      "9300" },
	{ 0, ""}
};

static const char *ath9k_hw_name(uint32_t mac_bb_version)
{
	int i;
	for (i=0; ath_mac_bb_names[i].version; i++)
		if (ath_mac_bb_names[i].version == mac_bb_version)
			return ath_mac_bb_names[i].name;
	return "????";
}

#define WAIT_TIMEOUT 10000 /* x10 us */

uint32_t eeprom_base,
		 macVer,
		 macRev;
bool	 isPCIE;

static uint16_t ath9k_eeprom_read16(struct pcidev *dev, unsigned int addr);
static uint16_t ath9k_eeprom_read16_short(struct pcidev *dev, unsigned int addr);
static bool ath9k_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value);
static bool ath9k_eeprom_write16_short(struct pcidev *dev, unsigned int addr, uint16_t value);

static void ath9k_read_hw_version(struct pcidev *dev)
{
	uint32_t val;

	val = PCI_IN32(AR_SREV) & AR_SREV_ID;
	if (val == 0xFF) {
		val = PCI_IN32(AR_SREV);
		macVer = (val & AR_SREV_VERSION2) >> AR_SREV_TYPE2_S;
		macRev = (val & AR_SREV_REVISION2) >> AR_SREV_REVISION2_S;
		isPCIE = (val & AR_SREV_TYPE2_HOST_MODE) ? 0 : 1;
	} else {
		if (!AR_SREV_9100)
			macVer = (val & AR_SREV_VERSION) >> AR_SREV_VERSION_S;
		macRev = val & AR_SREV_REVISION;
		if (macVer == AR_SREV_VERSION_5416_PCIE)
			isPCIE = 1;
	}
}

static bool ath9k_eeprom_lock(struct pcidev *dev) {
// reading HW version, some register addresses depends on it
	ath9k_read_hw_version(dev);
	printf("HW ver AR%s (PCI%s) rev %04x", ath9k_hw_name(macVer), isPCIE ? "-E" : "", macRev);

// reading EEPROM size and setting it's base address
// thanks to Inv from forum.ixbt.com
	dev->ops->eeprom_size = 0;
	if (376  == ath9k_eeprom_read16(dev,  64))      { dev->ops->eeprom_size =  376; eeprom_base =  64; dev->ops->eeprom_signature = 0x0178; }
	else if (3256 == ath9k_eeprom_read16(dev, 256)) { dev->ops->eeprom_size = 3256; eeprom_base = 256; dev->ops->eeprom_signature = 0x0cb8; }
	else if (727  == ath9k_eeprom_read16(dev, 128)) { dev->ops->eeprom_size =  727; eeprom_base = 128; dev->ops->eeprom_signature = 0x02d7; }
	if (!dev->ops->eeprom_size) {
		printf("Can't get ath9k eeprom size!\n");
		return false;
	} else {
		printf("ath9k eeprom size: %d\n", dev->ops->eeprom_size);
		return true;
	}
}

static bool ath9k_eeprom_release(struct pcidev *dev) {
// CRC calc should be here
	return true;
}

static uint16_t ath9k_eeprom_read16(struct pcidev *dev, unsigned int addr)
{
	int32_t data;
	int i;

// requesting data
	data = PCI_IN32(AR5416_EEPROM_OFFSET + (addr << AR5416_EEPROM_S));

// waiting...
	for(i = WAIT_TIMEOUT; i; i--) {
		usleep(10);
		data = PCI_IN32(AR_EEPROM_STATUS_DATA);
		if ( 0 == (data & (AR_EEPROM_STATUS_DATA_BUSY | AR_EEPROM_STATUS_DATA_PROT_ACCESS))) {
			data &= AR_EEPROM_STATUS_DATA_VAL;
			return data;
		}
	}
	printf("timeout reading ath9k eeprom at %04x!\n", addr);
	return 0;
}

static uint16_t ath9k_eeprom_read16_short(struct pcidev *dev, unsigned int addr)
{
	if (!dev->mem)
		return buf_read16(addr);

	return ath9k_eeprom_read16(dev, addr + eeprom_base);
}

static bool ath9k_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value)
{
	int i;
	uint32_t data;
	uint32_t
			gpio_out_mux1,
			gpio_oe_out,
			gpio_in_out;

// selecting GPIO
	gpio_out_mux1 = PCI_IN32(AR_GPIO_OUTPUT_MUX1);
	PCI_OUT32(AR_GPIO_OUTPUT_MUX1, gpio_out_mux1 & 0xFFF07FFF);
	usleep(10);

// setting GPIO pin direction
	gpio_oe_out = PCI_IN32(AR_GPIO_OE_OUT);
	PCI_OUT32(AR_GPIO_OE_OUT, (gpio_oe_out & 0xFFFFFF3F) | 0x000000C0);
	usleep(10);

// setting GPIO pin level
	gpio_in_out = PCI_IN32(AR_GPIO_IN_OUT);
	PCI_OUT32(AR_GPIO_IN_OUT, gpio_in_out & 0xFFFFFFF7);
	usleep(10);

// sending data
	PCI_OUT16(AR5416_EEPROM_OFFSET + (addr << AR5416_EEPROM_S), value);

// waiting...
	for(i = WAIT_TIMEOUT; i; i--) {
		usleep(10);
		memcpy(&data, dev->mem + AR_EEPROM_STATUS_DATA, 4);
		if ( 0 == (data & ( AR_EEPROM_STATUS_DATA_BUSY | 
							AR_EEPROM_STATUS_DATA_BUSY_ACCESS | 
							AR_EEPROM_STATUS_DATA_PROT_ACCESS | 
							AR_EEPROM_STATUS_DATA_ABSENT_ACCESS)) )
			break;
	}

// restoring GPIO state...
	PCI_OUT32(AR_GPIO_IN_OUT, gpio_in_out);
	PCI_OUT32(AR_GPIO_OE_OUT, gpio_oe_out);
	PCI_OUT32(AR_GPIO_OUTPUT_MUX1, gpio_out_mux1);

	if (!i)
		printf("timeout writing ath9k eeprom at %04x!\n", addr);
	return !!i;
}

static bool ath9k_eeprom_write16_short(struct pcidev *dev, unsigned int addr, uint16_t value)
{
	if (!dev->mem)
		return buf_write16(addr, value);

	return ath9k_eeprom_write16(dev, addr + eeprom_base, value);
}

static void ath9k_eeprom_patch11n(struct pcidev *dev)
{
	printf("ath9k 802.11n patch not implemented yet.\n");
}

struct dev_ops dev_ops_ath9k = {
	.name			 = "ath9k",
	.mmap_size        = ATH_MMAP_LENGTH,
	//.eeprom_size	  = ATH9K_EEPROM_SIZE
	.eeprom_size	  = 0,
	.eeprom_signature = 0,
	.eeprom_writable  = true,

	.init_device	 = NULL,
	.eeprom_check    = NULL,
	.eeprom_lock     = &ath9k_eeprom_lock,
	.eeprom_release  = &ath9k_eeprom_release,
	.eeprom_read16   = &ath9k_eeprom_read16_short,
	.eeprom_write16  = &ath9k_eeprom_write16_short,
	.eeprom_patch11n = &ath9k_eeprom_patch11n,
	.eeprom_parse    = NULL
};

