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

#include "iwlio.h"

#define IWL_EEPROM_SIGNATURE   0x5a40
#define IWL_MMAP_LENGTH 0x1000

#define CSR_WH_IF_CONFIG_REG 0x000
#define CSR_EEPROM_REG       0x02c

struct iwl_regulatory_item
{
	unsigned int addr;
	uint16_t	 data;
	uint16_t	 chn;
};


#define HT40 0x100
struct iwl_regulatory_item iwl_regulatory[] =
{
/*
	BAND 2.4GHz (@15e-179 with regulatory base @156)
*/
// enabling channels 12-14 (1-11 should be enabled on all cards)
	{ 0x1E, 0x0f21, 12 },
	{ 0x20, 0x0f21, 13 },
	{ 0x22, 0x0f21, 14 },

/*
	BAND 5GHz
*/
// subband 5170-5320 MHz (@198-1af)
//	{ 0x42, 0x0fe1, 34 },
	{ 0x44, 0x0fe1, 36 },
//	{ 0x46, 0x0fe1, 38 },
	{ 0x48, 0x0fe1, 40 },
//	{ 0x4a, 0x0fe1, 42 },
	{ 0x4c, 0x0fe1, 44 },
//	{ 0x4e, 0x0fe1, 46 },
	{ 0x50, 0x0fe1, 48 },
	{ 0x52, 0x0f31, 52 },
	{ 0x54, 0x0f31, 56 },
	{ 0x56, 0x0f31, 60 },
	{ 0x58, 0x0f31, 64 },

// subband 5500-5700 MHz (@1b2-1c7)
	{ 0x5c, 0x0f31, 100 },
	{ 0x5e, 0x0f31, 104 },
	{ 0x60, 0x0f31, 108 },
	{ 0x62, 0x0f31, 112 },
	{ 0x64, 0x0f31, 116 },
	{ 0x66, 0x0f31, 120 },
	{ 0x68, 0x0f31, 124 },
	{ 0x6a, 0x0f31, 128 },
	{ 0x6c, 0x0f31, 132 },
	{ 0x6e, 0x0f31, 136 },
	{ 0x70, 0x0f31, 140 },

// subband 5725-5825 MHz (@1ca-1d5)
//	{ 0x74, 0x0fa1, 145 },
	{ 0x76, 0x0fa1, 149 },
	{ 0x78, 0x0fa1, 153 },
	{ 0x7a, 0x0fa1, 157 },
	{ 0x7c, 0x0fa1, 161 },
	{ 0x7e, 0x0fa1, 165 },

/*
	BAND 2.4GHz, HT40 channels (@1d8-1e5)
*/
	{ 0x82, 0x0e6f, HT40 + 1 },
	{ 0x84, 0x0f6f, HT40 + 2 },
	{ 0x86, 0x0f6f, HT40 + 3 },
	{ 0x88, 0x0f6f, HT40 + 4 },
	{ 0x8a, 0x0f6f, HT40 + 5 },
	{ 0x8c, 0x0f6f, HT40 + 6 },
	{ 0x8e, 0x0f6f, HT40 + 7 },

/*
	BAND 5GHz, HT40 channels (@1e8-1fd)
*/
	{ 0x92, 0x0fe1, HT40 +  36 },
	{ 0x94, 0x0fe1, HT40 +  44 },
	{ 0x96, 0x0f31, HT40 +  52 },
	{ 0x98, 0x0f31, HT40 +  60 },
	{ 0x9a, 0x0f31, HT40 + 100 },
	{ 0x9c, 0x0f31, HT40 + 108 },
	{ 0x9e, 0x0f31, HT40 + 116 },
	{ 0xa0, 0x0f31, HT40 + 124 },
	{ 0xa2, 0x0f31, HT40 + 132 },
	{ 0xa4, 0x0f61, HT40 + 149 },
	{ 0xa6, 0x0f61, HT40 + 157 },

	{ 0, 0}
};

void iwl_eeprom_lock(struct pcidev *dev)
{
	unsigned long data;
	if (!dev->mem) return;
	memcpy(&data, dev->mem, 4);
	data |= 0x00200000;
	memcpy(dev->mem, &data, 4);
	usleep(5);
	memcpy(&data, dev->mem, 4);
	if ((data & 0x00200000) != 0x00200000)
		die("err! ucode is using eeprom!\n");
	dev->eeprom_locked = 1;
}

void iwl_eeprom_unlock(struct pcidev *dev)
{
	unsigned long data;
	if (!dev->mem) return;
	memcpy(&data, dev->mem, 4);
	data &= ~0x00200000;
	memcpy(dev->mem, &data, 4);
	usleep(5);
	memcpy(&data, dev->mem, 4);
	if ((data & 0x00200000) == 0x00200000)
		die("err! software is still using eeprom!\n");
	dev->eeprom_locked = 0;
}

const uint16_t iwl_eeprom_read16(struct pcidev *dev, unsigned int addr)
{
	uint16_t value;
	unsigned int data = 0x0000FFFC & (addr << 1);
	if (!dev->mem)
		return buf_read16(addr);

	memcpy(dev->mem + CSR_EEPROM_REG, &data, 4);
	usleep(50);
	memcpy(&data, dev->mem + CSR_EEPROM_REG, 4);
	if ((data & 1) != 1)
		die("Read not complete! Timeout at %.4dx\n", addr);

	value = (data & 0xFFFF0000) >> 16;
	return value;
}

void iwl_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value)
{
	if (!dev->mem) {
		buf_write16(addr, value);
		return;
	}

	unsigned int data = value;

	if (preserve_mac && ((addr>=0x2A && addr<0x30) || (addr>=0x92 && addr<0x97)))
		return;
	if (preserve_calib && (addr >= 0x200))
		return;

	data <<= 16;
	data |= 0x0000FFFC & (addr << 1);
	data |= 0x2;

	memcpy(dev->mem + CSR_EEPROM_REG, &data, 4);
	usleep(5000);

	data = 0x0000FFC & (addr << 1);
	memcpy(dev->mem + CSR_EEPROM_REG, &data, 4);
	usleep(50);
	memcpy(&data, dev->mem + CSR_EEPROM_REG, 4);
	if ((data & 1) != 1)
		die("Read not complete! Timeout at %.4dx\n", addr);
	if (value != (data >> 16))
		die("Verification error at %.4x\n", addr);
	return;
}

void iwl_eeprom_patch11n(struct pcidev *dev)
{
	uint16_t value;
	unsigned int reg_offs;
	int idx;

	printf("Patching card EEPROM...\n");

	dev->ops->eeprom_lock(dev);	

	printf("-> Changing subdev ID\n");
	value = dev->ops->eeprom_read16(dev, 0x14);
	if ((value & 0x000F) == 0x0006) {
		dev->ops->eeprom_write16(dev, 0x14, (value & 0x000F) | 0x0001);
	}
/*
enabling .11n

W @8A << 00F0 (00B0) <- xxxx xxxx x1xx xxxx
W @8C << 103E (603F) <- x001 xxxx xxxx xxx0
*/

	printf("-> Enabling 11n mode\n");
// SKU_CAP
	value = dev->ops->eeprom_read16(dev, 0x8A);
	if ((value & 0x0040) != 0x0040) {
		printf("  SKU CAP\n");
		dev->ops->eeprom_write16(dev, 0x8A, value | 0x0040);
	}

// OEM_MODE
	value = dev->ops->eeprom_read16(dev, 0x8C);
	if ((value & 0x7001) != 0x1000) {
		printf("  OEM MODE\n");
		dev->ops->eeprom_write16(dev, 0x8C, (value & 0x9FFE) | 0x1000);
	}

/*
writing SKU ID - 'MoW' signature
*/
	if (dev->ops->eeprom_read16(dev, 0x158) != 0x6f4d)
		dev->ops->eeprom_write16(dev, 0x158, 0x6f4d);
	if (dev->ops->eeprom_read16(dev, 0x15A) != 0x0057)
		dev->ops->eeprom_write16(dev, 0x15A, 0x0057);

	printf("-> Checking and adding channels...\n");
// reading regulatory offset
	reg_offs = 2 * dev->ops->eeprom_read16(dev, 0xCC);
	printf("Regulatory base: %04x\n", reg_offs);
/*
writing channels regulatory...
*/
	for (idx=0; iwl_regulatory[idx].addr; idx++) {
		if (dev->ops->eeprom_read16(dev, reg_offs + iwl_regulatory[idx].addr) != iwl_regulatory[idx].data) {
			printf("  %d%s\n", iwl_regulatory[idx].chn & ~HT40, (iwl_regulatory[idx].chn & HT40) ? " (HT40)" : "");
			dev->ops->eeprom_write16(dev, reg_offs + iwl_regulatory[idx].addr, iwl_regulatory[idx].data);
		}
	}

	dev->ops->eeprom_unlock(dev);
	printf("\nCard EEPROM patched successfully\n");
}

struct dev_ops dev_ops_iwl = {
	.mmap_size        = IWL_MMAP_LENGTH,
	.eeprom_signature = IWL_EEPROM_SIGNATURE,

	.eeprom_lock     = &iwl_eeprom_lock,
	.eeprom_unlock   = &iwl_eeprom_unlock,
	.eeprom_read16   = &iwl_eeprom_read16,
	.eeprom_write16  = &iwl_eeprom_write16,
	.eeprom_patch11n = &iwl_eeprom_patch11n
};

