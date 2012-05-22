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
#define ATH9K_EEPROM_SIGNATURE      0xA55A
#define ATH9K_MMAP_LENGTH          0x10000

#define AR9300_EEPROM_SIZE			0x4000

#define AR5416_EEPROM_S             2
#define AR5416_EEPROM_OFFSET        0x2000

#define AR5416_OPFLAGS_11A          0x01
#define AR5416_OPFLAGS_11G          0x02
#define AR5416_OPFLAGS_N_5G_HT40    0x04
#define AR5416_OPFLAGS_N_2G_HT40    0x08
#define AR5416_OPFLAGS_N_5G_HT20    0x10
#define AR5416_OPFLAGS_N_2G_HT20    0x20


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
#define AR_SREV_VERSION_9330        0x200
#define AR_SREV_VERSION_9485        0x240
#define AR_SREV_VERSION_9340        0x300
#define AR_SREV_VERSION_9580        0x1c0
#define AR_SREV_VERSION_9462        0x280

#define AR_RADIO_SREV_MAJOR         0xf0
#define AR_RAD5133_SREV_MAJOR       0x30
#define AR_RAD2133_SREV_MAJOR       0xb0
#define AR_RAD5122_SREV_MAJOR       0x70
#define AR_RAD2122_SREV_MAJOR       0xf0

#define AR_PHY_BASE     0x9800
#define AR_PHY(_n)      (AR_PHY_BASE + ((_n)<<2))


#define AR_SREV_9100 \
	(macVer == AR_SREV_VERSION_9100)
#define AR_SREV_9280_10_OR_LATER \
	(macVer >= AR_SREV_VERSION_9280)
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


/* Atheros 9k devices */
const struct pci_id ath9k_ids[] = {
	{ ATHEROS_PCI_VID, 0x0023, "AR5416 (AR5008 family) Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x0024, "AR5416 (AR5008 family) Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0027, "AR9160 802.11abgn Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x0029, "AR922X Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x002A, "AR928X Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x002B, "AR9285 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x002C, "AR2427 Wireless Adapter (PCI-E)" }, /* PCI-E 802.11n bonded out */
	{ ATHEROS_PCI_VID, 0x002D, "AR9287 Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x002E, "AR9287 Wireless Adapter (PCI-E)" },
	{ 0, 0, "" }
};

/* AR9300 devices */
const struct pci_id ath9300_ids[] = {
	{ ATHEROS_PCI_VID, 0x0030, "AR9300 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0031, "AR9340 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0032, "AR9485 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0033, "AR9580 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0034, "AR9462 Wireless Adapter (PCI-E)" },
	{ 0, 0, "" }
};

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
	{ AR_SREV_VERSION_9330,      "9330" },
	{ AR_SREV_VERSION_9485,      "9485" },
	{ AR_SREV_VERSION_9340,      "9340" },
	{ AR_SREV_VERSION_9462,      "9462" },
	{ 0, ""}
};

static const char *ath9k_hw_name(uint16_t mac_bb_version)
{
	int i;
	for (i=0; ath_mac_bb_names[i].version; i++)
		if (ath_mac_bb_names[i].version == mac_bb_version)
			return ath_mac_bb_names[i].name;
	return "????";
}

/* For devices with external radios */
static struct {
	uint16_t version;
	const char * name;
} ath_rf_names[] = {
	{ AR_RAD5133_SREV_MAJOR,	"5133" },
	{ AR_RAD5122_SREV_MAJOR,	"5122" },
	{ AR_RAD2133_SREV_MAJOR,	"2133" },
	{ AR_RAD2122_SREV_MAJOR,	"2122" },
	{ 0, ""}
};

static const char *ath9k_rf_name(uint16_t rf_version)
{
	int i;
	for (i=0; ath_rf_names[i].version; i++)
		if (ath_rf_names[i].version == rf_version)
			return ath_rf_names[i].name;
	return "????";
}

#define WAIT_TIMEOUT 10000 /* x10 us */

uint32_t short_eeprom_base,
		 short_eeprom_size;
uint16_t macVer,
		 macRev,
		 rfVer,
		 rfRev;
bool	 isPCIE;

static bool ath9k_eeprom_read16(struct pcidev *dev, unsigned int addr, uint16_t *value);
static bool ath9k_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value);

static void ath9k_get_hw_version(struct pcidev *dev)
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

// ************************************

static void ath9k_get_rf_version(struct pcidev *dev)
{
	int i;
	PCI_OUT32(AR_PHY(0), 0x00000007);
//	ENABLE_REGWRITE_BUFFER(ah);
	PCI_OUT32(AR_PHY(0x36), 0x00007058);
	for (i = 0; i < 8; i++)
		PCI_OUT32(AR_PHY(0x20), 0x00010000);
//	REGWRITE_BUFFER_FLUSH(ah);
//	DISABLE_REGWRITE_BUFFER(ah);
	rfVer = (PCI_IN32(AR_PHY(0x100)) >> 24) & 0xff;
	if (!(rfVer & AR_RADIO_SREV_MAJOR))
		rfVer = AR_RAD5133_SREV_MAJOR;
}

// ************************************

static bool ath9k_eeprom_lock(struct pcidev *dev) {
	uint16_t data;
// reading HW version, some register addresses depends on it
	ath9k_get_hw_version(dev);
	printf("HW: AR%s (PCI%s) rev %04x\n", ath9k_hw_name(macVer), isPCIE ? "-E" : "", macRev);
	if (AR_SREV_9280_10_OR_LATER) {
		printf("RF: integrated\n");
	} else {
		ath9k_get_rf_version(dev);
		printf("RF: AR%s\n", ath9k_rf_name(rfVer & AR_RADIO_SREV_MAJOR));
	}

// reading EEPROM size and setting it's base address
// thanks to Inv from forum.ixbt.com
	if (ath9k_eeprom_read16(dev, 128, &data) && (376 == data)) {
		short_eeprom_base = 128;
		short_eeprom_size = 376;
		goto ssize_ok;
	}
	if (ath9k_eeprom_read16(dev, 512, &data) && (3256 == data)) {
		short_eeprom_base =  512;
		short_eeprom_size = 3256;
		goto ssize_ok;
	}
	if (ath9k_eeprom_read16(dev, 256, &data) && (727 == data)) {
		short_eeprom_base = 256;
		short_eeprom_size = 727;
		goto ssize_ok;
	}

	short_eeprom_base = 0;
	short_eeprom_size = 0;
	printf("Can't get ath9k eeprom size!\n");
//	return false;
ssize_ok:
	printf("ath9k short eeprom base: %d  size: %d\n",
		short_eeprom_base,
		short_eeprom_size);
	return true;
}

static bool ath9k_eeprom_crc_calc(struct pcidev *dev, uint16_t *crcp)
{
	uint16_t crc = 0, data;
	int i;

	if (!short_eeprom_size)
		return false;

	for (i=0; i<short_eeprom_size; i+=2) {
		if (2 == i) continue;
		if (!ath9k_eeprom_read16(dev, short_eeprom_base + i, &data))
			return false;
		crc ^= data;
	}
	crc ^= 0xFFFF;
	if (crcp)
		*crcp = crc;
	return true;
}

static bool ath9k_eeprom_crc_update(struct pcidev *dev)
{
	uint16_t crc, crc_n;
	if (short_eeprom_base) {
		dev->ops->eeprom_read16(dev, short_eeprom_base+2, &crc);
		ath9k_eeprom_crc_calc(dev, &crc_n);
		if (crc != crc_n) {
			printf("Updating CRC: %04x -> %04x\n", crc, crc_n);
			dev->ops->eeprom_write16(dev, short_eeprom_base+2, crc_n);
		}
	}
	return true;
}

static bool ath9k_eeprom_release(struct pcidev *dev) {
	return true;
}

static bool ath9k_eeprom_read16(struct pcidev *dev, uint32_t addr, uint16_t *value)
{
	int32_t data;
	int i;

// requesting data
	data = PCI_IN32(AR5416_EEPROM_OFFSET + ((addr >> 1) << AR5416_EEPROM_S));

// waiting...
	for(i = WAIT_TIMEOUT; i; i--) {
		usleep(10);
		data = PCI_IN32(AR_EEPROM_STATUS_DATA);
		if ( 0 == (data & (AR_EEPROM_STATUS_DATA_BUSY | AR_EEPROM_STATUS_DATA_PROT_ACCESS))) {
			*value = data & AR_EEPROM_STATUS_DATA_VAL;
			return true;
		}
	}
	printf("timeout reading ath9k eeprom at %04x!\n", addr);
	return false;
}

static bool ath9k_eeprom_write16(struct pcidev *dev, uint32_t addr, uint16_t value)
{
	int i;
	uint32_t data;
	uint32_t
			gpio_out_mux1,
			gpio_oe_out,
			gpio_in_out;

	if (preserve_mac && short_eeprom_base && (addr>=(short_eeprom_base+0x0c) && addr<(short_eeprom_base+0x12)))
		return true;

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
	PCI_OUT16(AR5416_EEPROM_OFFSET + ((addr >> 1) << AR5416_EEPROM_S), value);

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

static bool ath9k_eeprom_write16_short(struct pcidev *dev, uint32_t addr, uint16_t value)
{
	// just return, if address out of 'short' eeprom bounds
	if ((addr <= short_eeprom_base) || (addr >= (short_eeprom_base + short_eeprom_size)))
		return false;
	if (!dev->mem)
		return buf_write16(addr, value);

	return ath9k_eeprom_write16(dev, addr, value);
}

static void ath9k_eeprom_patch11n(struct pcidev *dev)
{
	uint16_t
		opCap,
		regDmn;
	if (!short_eeprom_size) {
		printf("Unknown short EEPROM size -> can't patch!\n");
		return;
	}

	printf("Patching card EEPROM...\n");

	dev->ops->eeprom_read16(dev, short_eeprom_base + 6, &opCap);
	dev->ops->eeprom_read16(dev, short_eeprom_base + 8, &regDmn);

	printf("Reg. domain : %04x\n", regDmn);
	printf("       Bands: %s%s\n",
		(opCap & AR5416_OPFLAGS_11A) ? " 5GHz" : "",
		(opCap & AR5416_OPFLAGS_11G) ? " 2.4GHz" : "");

	regDmn = 0x6A;
	if (opCap & AR5416_OPFLAGS_11G)
		opCap &= ~(AR5416_OPFLAGS_N_2G_HT20 | AR5416_OPFLAGS_N_2G_HT40);
	if (opCap & AR5416_OPFLAGS_11A)
		opCap &= ~(AR5416_OPFLAGS_N_5G_HT20 | AR5416_OPFLAGS_N_5G_HT40);

	dev->ops->eeprom_write16(dev, short_eeprom_base + 6, opCap);
	dev->ops->eeprom_write16(dev, short_eeprom_base + 8, regDmn);

	ath9k_eeprom_crc_update(dev);
}

static void ath9k_eeprom_parse(struct pcidev *dev)
{

	uint16_t
		crc, crc_n,
		opCap,
		regDmn,
		mac[3];

	dev->ops->eeprom_read16(dev, short_eeprom_base + 2, &crc);
	dev->ops->eeprom_read16(dev, short_eeprom_base + 6, &opCap);
	dev->ops->eeprom_read16(dev, short_eeprom_base + 8, &regDmn);

	dev->ops->eeprom_read16(dev, short_eeprom_base +12, mac);
	dev->ops->eeprom_read16(dev, short_eeprom_base +14, mac+1);
	dev->ops->eeprom_read16(dev, short_eeprom_base +16, mac+2);

	printf("MAC address : %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac[0] & 0xFF,  mac[0] >> 8, 
			mac[1] & 0xFF,  mac[1] >> 8, 
			mac[2] & 0xFF,  mac[2] >> 8);

	printf("Reg. domain : %04x\n", regDmn);
	printf("Capabilities: %04x\n"
		   "       Bands: %s%s\n", opCap,
		(opCap & AR5416_OPFLAGS_11A) ? " 5GHz" : "",
		(opCap & AR5416_OPFLAGS_11G) ? " 2.4GHz" : "");

	printf("       HT 2G: %s%s\n",
		(opCap & AR5416_OPFLAGS_N_2G_HT20) ? "":" HT20",
		(opCap & AR5416_OPFLAGS_N_2G_HT40) ? "":" HT40");
	printf("       HT 5G: %s%s\n",
		(opCap & AR5416_OPFLAGS_N_5G_HT20) ? "":" HT20",
		(opCap & AR5416_OPFLAGS_N_5G_HT40) ? "":" HT40");

	printf("CRC (stored): %04x\n", crc);

	if (ath9k_eeprom_crc_calc(dev, &crc_n))
		printf("CRC (eval)  : %04x\n", crc_n);
	else
		printf("error calculating CRC!\n");
}

struct io_driver io_ath9k = {
	.name             = "ath9k",
	.valid_ids        = (struct pci_id*) &ath9k_ids,
	.mmap_size        = ATH9K_MMAP_LENGTH,
	.eeprom_size	  = ATH9K_EEPROM_SIZE,
	.eeprom_signature = ATH9K_EEPROM_SIGNATURE,
	.eeprom_writable  = true,

	.init_device	 = NULL,
	.eeprom_check    = NULL,
	.eeprom_lock     = &ath9k_eeprom_lock,
	.eeprom_release  = &ath9k_eeprom_release,
	.eeprom_read16   = &ath9k_eeprom_read16,
	.eeprom_write16  = &ath9k_eeprom_write16_short,
	.eeprom_patch11n = &ath9k_eeprom_patch11n,
	.eeprom_parse    = &ath9k_eeprom_parse
};

struct io_driver io_ath9300 = {
	.name             = "ath9300",
	.valid_ids        = (struct pci_id*) &ath9300_ids,
	.mmap_size        = ATH9K_MMAP_LENGTH,
	.eeprom_size	  = AR9300_EEPROM_SIZE,
	.eeprom_signature = ATH9K_EEPROM_SIGNATURE,
	.eeprom_writable  = true,

	.init_device	 = NULL,
	.eeprom_check    = NULL,
	.eeprom_lock     = &ath9k_eeprom_lock,
	.eeprom_release  = &ath9k_eeprom_release,
	.eeprom_read16   = &ath9k_eeprom_read16,
	.eeprom_write16  = &ath9k_eeprom_write16_short,
	.eeprom_patch11n = &ath9k_eeprom_patch11n,
	.eeprom_parse    = &ath9k_eeprom_parse
};

