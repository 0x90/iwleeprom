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

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>
#include <endian.h>

//#define DEBUG
#define MMAP_LENGTH 4096
#define EEPROM_SIZE 2048

static struct option long_options[] = {
	{"device",    1, NULL, 'd'},
	{"read",      1, NULL, 'r'},
	{"write",     1, NULL, 'w'},
	{"help",      0, NULL, 'h'},
	{"patch11n",  0, NULL, 'p'}
};

struct pcidev_id
{
	unsigned int class,
				ven,  dev,
				sven, sdev;
	int idx;
	char *device;
};

struct pci_id
{
	unsigned int   ven, dev;
	unsigned char  writable;
	char name[64];
};

enum byte_order
{
	order_unknown = 0,
	order_be,
	order_le
};

int mem_fd;
char *mappedAddress;
unsigned int offset;
unsigned char eeprom_locked;

void die(  const char* format, ... ); 
char	*ifname = NULL,
		*ofname = NULL;
bool patch11n;

struct pcidev_id dev;

#define DEVICES_PATH "/sys/bus/pci/devices"

struct pci_id valid_ids[] = {
	{ 0x8086, 0x0082, 0, "6000 Series Gen2"},
	{ 0x8086, 0x0083, 0, "Centrino Wireless-N 1000"},
/*		8086 1205  Centrino Wireless-N 1000 BGN
		8086 1206  Centrino Wireless-N 1000 BG
		8086 1225  Centrino Wireless-N 1000 BGN
		8086 1226  Centrino Wireless-N 1000 BG
		8086 1305  Centrino Wireless-N 1000 BGN
		8086 1306  Centrino Wireless-N 1000 BG
		8086 1325  Centrino Wireless-N 1000 BGN
		8086 1326  Centrino Wireless-N 1000 BG*/
	{ 0x8086, 0x0084, 0, "Centrino Wireless-N 1000"},
/*		8086 1215  Centrino Wireless-N 1000 BGN
		8086 1216  Centrino Wireless-N 1000 BG
		8086 1315  Centrino Wireless-N 1000 BGN
		8086 1316  Centrino Wireless-N 1000 BG*/
	{ 0x8086, 0x0085, 0, "6000 Series Gen2"},
	{ 0x8086, 0x0087, 0, "Centrino Advanced-N + WiMAX 6250"},
/*		8086 1301  Centrino Advanced-N + WiMAX 6250 2x2 AGN
		8086 1306  Centrino Advanced-N + WiMAX 6250 2x2 ABG
		8086 1321  Centrino Advanced-N + WiMAX 6250 2x2 AGN
		8086 1326  Centrino Advanced-N + WiMAX 6250 2x2 ABG*/
	{ 0x8086, 0x0089, 0, "Centrino Advanced-N + WiMAX 6250"},
/*		8086 1311  Centrino Advanced-N + WiMAX 6250 2x2 AGN
		8086 1316  Centrino Advanced-N + WiMAX 6250 2x2 ABG*/
	{ 0x8086, 0x0885, 0, "WiFi+WiMAX 6050 Series Gen2"},
	{ 0x8086, 0x0886, 0, "WiFi+WiMAX 6050 Series Gen2"},
	{ 0x8086, 0x4229, 1, "PRO/Wireless 4965 AG or AGN [Kedron] Network Connection"},
/*		8086 1100  Vaio VGN-SZ79SN_C
		8086 1101  PRO/Wireless 4965 AG or AGN*/
	{ 0x8086, 0x422b, 0, "Centrino Ultimate-N 6300"},
/*		8086 1101  Centrino Ultimate-N 6300 3x3 AGN
		8086 1121  Centrino Ultimate-N 6300 3x3 AGN*/
	{ 0x8086, 0x422c, 0, "Centrino Advanced-N 6200"},
/*		8086 1301  Centrino Advanced-N 6200 2x2 AGN
		8086 1306  Centrino Advanced-N 6200 2x2 ABG
		8086 1307  Centrino Advanced-N 6200 2x2 BG
		8086 1321  Centrino Advanced-N 6200 2x2 AGN
		8086 1326  Centrino Advanced-N 6200 2x2 ABG*/
	{ 0x8086, 0x4230, 1, "PRO/Wireless 4965 AG or AGN [Kedron] Network Connection"},
/*		8086 1110  Lenovo ThinkPad T51
		8086 1111  Lenovo ThinkPad T61*/
	{ 0x8086, 0x4232, 1, "WiFi Link 5100"},
/*		8086 1201  WiFi Link 5100 AGN
		8086 1204  WiFi Link 5100 AGN
		8086 1205  WiFi Link 5100 BGN
		8086 1206  WiFi Link 5100 ABG
		8086 1221  WiFi Link 5100 AGN
		8086 1224  WiFi Link 5100 AGN
		8086 1225  WiFi Link 5100 BGN
		8086 1226  WiFi Link 5100 ABG
		8086 1301  WiFi Link 5100 AGN
		8086 1304  WiFi Link 5100 AGN
		8086 1305  WiFi Link 5100 BGN
		8086 1306  WiFi Link 5100 ABG
		8086 1321  WiFi Link 5100 AGN
		8086 1324  WiFi Link 5100 AGN
		8086 1325  WiFi Link 5100 BGN
		8086 1326  WiFi Link 5100 ABG*/
	{ 0x8086, 0x4235, 1, "Ultimate N WiFi Link 5300"},
	{ 0x8086, 0x4236, 1, "Ultimate N WiFi Link 5300"},
	{ 0x8086, 0x4237, 1, "PRO/Wireless 5100 AGN [Shiloh] Network Connection"},
/*		8086 1211  WiFi Link 5100 AGN
		8086 1214  WiFi Link 5100 AGN
		8086 1215  WiFi Link 5100 BGN
		8086 1216  WiFi Link 5100 ABG
		8086 1311  WiFi Link 5100 AGN
		8086 1314  WiFi Link 5100 AGN
		8086 1315  WiFi Link 5100 BGN
		8086 1316  WiFi Link 5100 ABG*/
	{ 0x8086, 0x4238, 0, "Centrino Ultimate-N 6300"},
/*		8086 1111  Centrino Ultimate-N 6300 3x3 AGN*/
	{ 0x8086, 0x4239, 0, "Centrino Advanced-N 6200"},
/*		8086 1311  Centrino Advanced-N 6200 2x2 AGN
		8086 1316  Centrino Advanced-N 6200 2x2 ABG*/
	{ 0x8086, 0x423a, 1, "PRO/Wireless 5350 AGN [Echo Peak] Network Connection"},
	{ 0x8086, 0x423b, 1, "PRO/Wireless 5350 AGN [Echo Peak] Network Connection"},
	{ 0x8086, 0x423c, 1, "WiMAX/WiFi Link 5150"},
/*		8086 1201  WiMAX/WiFi Link 5150 AGN
		8086 1206  WiMAX/WiFi Link 5150 ABG
		8086 1221  WiMAX/WiFi Link 5150 AGN
		8086 1301  WiMAX/WiFi Link 5150 AGN
		8086 1306  WiMAX/WiFi Link 5150 ABG
		8086 1321  WiMAX/WiFi Link 5150 AGN*/
	{ 0x8086, 0x423d, 1, "WiMAX/WiFi Link 5150"},
/*		8086 1211  WiMAX/WiFi Link 5150 AGN
		8086 1216  WiMAX/WiFi Link 5150 ABG
		8086 1311  WiMAX/WiFi Link 5150 AGN
		8086 1316  WiMAX/WiFi Link 5150 ABG*/
	{ 0, 0, 0, "" }
};

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

void eeprom_lock()
{
	unsigned long data;	
	memcpy(&data, mappedAddress, 4);
	data |= 0x00200000;
	memcpy(mappedAddress, &data, 4);
	usleep(5);
	memcpy(&data, mappedAddress, 4);
	if (data & 0x00200000 != 0x00200000)
		die("err! ucode is using eeprom!\n");
	eeprom_locked = 1;
}

void eeprom_unlock()
{
	unsigned long data;	
	memcpy(&data, mappedAddress, 4);
	data &= ~0x00200000;
	memcpy(mappedAddress, &data, 4);
	usleep(5);
	memcpy(&data, mappedAddress, 4);
	if (data & 0x00200000 != 0x00200000)
		die("err! software is still using eeprom!\n");
	eeprom_locked = 0;
}

void init_card()
{
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("cannot open /dev/mem\n");
		exit(1);
	}

	mappedAddress = (char *)mmap(NULL, MMAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, mem_fd, offset);
	if (mappedAddress == MAP_FAILED) {
		perror("mmap failed");
		exit(1);
	}
}

void release_card()
{	
	if (mappedAddress != NULL)
		munmap(mappedAddress, MMAP_LENGTH);
}

uint16_t eeprom_read16(unsigned int addr)
{
	uint16_t value;
	unsigned int data = 0x0000FFFC & (addr << 1);
	memcpy(mappedAddress + 0x2c, &data, 4);
	usleep(50);
	memcpy(&data, mappedAddress + 0x2c, 4);
	if (data & 1 != 1)
		die("Read not complete! Timeout at %.4dx\n", addr);

	value = (data & 0xFFFF0000) >> 16;
	return value;
}

void eeprom_write16(unsigned int addr, uint16_t value)
{
	unsigned int data = value;
	data <<= 16;
	data |= 0x0000FFFC & (addr << 1);
	data |= 0x2;

	memcpy(mappedAddress + 0x2c, &data, 4);
	usleep(5000);

	data = 0x0000FFC & (addr << 1);
	memcpy(mappedAddress + 0x2c, &data, 4);
	usleep(50);

	memcpy(&data, mappedAddress + 0x2c, 4);
	if (data & 1 != 1)
		die("Read not complete! Timeout at %.4dx\n", addr);
	if (value != (data >> 16))
		die("Verification error at %.4x\n", addr);
}

void eeprom_read(char *filename)
{
	unsigned int addr = 0;
	uint16_t buf[EEPROM_SIZE/2];

	FILE *fd = fopen(filename, "wb");
	if (!fd)
		die("Can't create file %s\n", filename);

	eeprom_lock();	

	for (addr = 0; addr < EEPROM_SIZE; addr += 2)
	{
		buf[addr/2] = cpu2le16( eeprom_read16(addr) );
		printf(".");
		fflush(stdout);
	}
	fwrite(buf, EEPROM_SIZE, 1, fd);

	eeprom_unlock();

	fclose(fd);

	printf("\nEEPROM has been dumped to %s\n", filename);
}

void eeprom_write(char *filename)
{
	unsigned int addr = 0;
	unsigned int data;
	enum byte_order order = order_unknown;
	uint16_t buf[EEPROM_SIZE/2];
	uint16_t value;
	size_t   size;
	FILE *fd = fopen(filename, "rb");
	if (!fd)
		die("Can't read file %s\n", filename);

	eeprom_lock();	

	size = 2 * fread(buf, 2, EEPROM_SIZE/2, fd);
	for(addr=0; addr<size;addr+=2)
	{
		if (order == order_unknown) {
			value = le2cpu16(buf[addr/2]);
			if(value == 0x5a40) {
				order = order_le;
			} else if(value == 0x405a) {
				order = order_be;
			} else {
				die("Invalid EEPROM signature!\n");
			}
			printf("Dump file byte order: %s ENDIAN\n", (order == order_le) ? "LITTLE":"BIG");
		}
		if (order == order_be)
			value = be2cpu16( buf[addr/2] );
		else
			value = le2cpu16( buf[addr/2] );

		eeprom_write16(addr, value);
		printf(".");
		fflush(stdout);
	}

	eeprom_unlock();

	fclose(fd);

	printf("\nEEPROM has been written from %s\n", filename);
}


struct patch_item
{
	unsigned int addr;
	uint16_t	 data;
};

struct patch_item regulatory[] =
{
/*
	BAND 2.4GHz (@15e-179 with regulatory base @156)
*/
// enabling channels 12-14 (1-11 should be enabled on all cards)
	{ 0x1E, 0x0f21 }, // 12
	{ 0x20, 0x0f21 }, // 13
	{ 0x22, 0x0f21 }, // 14

/*
	BAND 5GHz
*/
// subband 5170-5320 MHz (@198-1af)
//	{ 0x42, 0x0fe1 }, // 34
	{ 0x44, 0x0fe1 }, // 36
//	{ 0x46, 0x0fe1 }, // 38
	{ 0x48, 0x0fe1 }, // 40
//	{ 0x4a, 0x0fe1 }, // 42
	{ 0x4c, 0x0fe1 }, // 44
//	{ 0x4e, 0x0fe1 }, // 46
	{ 0x50, 0x0fe1 }, // 48
	{ 0x52, 0x0f31 }, // 52
	{ 0x54, 0x0f31 }, // 56
	{ 0x56, 0x0f31 }, // 60
	{ 0x58, 0x0f31 }, // 64

// subband 5500-5700 MHz (@1b2-1c7)
	{ 0x5c, 0x0f31 }, // 100
	{ 0x5e, 0x0f31 }, // 104
	{ 0x60, 0x0f31 }, // 108
	{ 0x62, 0x0f31 }, // 112
	{ 0x64, 0x0f31 }, // 116
	{ 0x66, 0x0f31 }, // 120
	{ 0x68, 0x0f31 }, // 124
	{ 0x6a, 0x0f31 }, // 128
	{ 0x6c, 0x0f31 }, // 132
	{ 0x6e, 0x0f31 }, // 136
	{ 0x70, 0x0f31 }, // 140

// subband 5725-5825 MHz (@1ca-1d5)
//	{ 0x74, 0x0fa1 }, // 145
	{ 0x76, 0x0fa1 }, // 149
	{ 0x78, 0x0fa1 }, // 153
	{ 0x7a, 0x0fa1 }, // 157
	{ 0x7c, 0x0fa1 }, // 151
	{ 0x7e, 0x0fa1 }, // 165

/*
	BAND 2.4GHz, HT40 channels (@1d8-1e5)
*/
	{ 0x82, 0x0e6f }, // 1
	{ 0x84, 0x0f6f }, // 2
	{ 0x86, 0x0f6f }, // 3
	{ 0x88, 0x0f6f }, // 4
	{ 0x8a, 0x0f6f }, // 5
	{ 0x8c, 0x0f6f }, // 6
	{ 0x8e, 0x0f6f }, // 7

/*
	BAND 5GHz, HT40 channels (@1e8-1fd)
*/
	{ 0x92, 0x0fe1 }, // 36
	{ 0x94, 0x0fe1 }, // 44
	{ 0x96, 0x0f31 }, // 52
	{ 0x98, 0x0f31 }, // 60
	{ 0x9a, 0x0f31 }, // 100
	{ 0x9c, 0x0f31 }, // 108
	{ 0x9e, 0x0f31 }, // 116
	{ 0xa0, 0x0f31 }, // 124
	{ 0xa2, 0x0f31 }, // 132
	{ 0xa4, 0x0f61 }, // 149
	{ 0xa6, 0x0f61 }, // 157

	{ 0, 0}
};

void eeprom_patch11n()
{
	uint16_t value;
	unsigned int reg_offs;

	eeprom_lock();	
/*
enabling .11n

W @8A << 00F0 (00B0) <- xxxx xxxx x1xx xxxx
W @8C << 103E (603F) <- x110 xxxx xxxx xxx0
*/

// SKU_CAP
	value = eeprom_read16(0x8A);
	value |= 0x0040;
	eeprom_write16(0x8A, value);

// OEM_MODE
	value = eeprom_read16(0x8C);
	value &= 0xEFFE;
	value |= 0x6000;
	eeprom_write16(0x8C, value);

/*
writing SKU ID - 'MoW' signature
*/
	eeprom_write16(0x158, 0x6f4d);
	eeprom_write16(0x15A, 0x0057);

// reading regulatory offset
	reg_offs = 2 * eeprom_read16(0xCC);
	printf("Regulatory base: %04x", reg_offs);
/*
writing channels regulatory...
*/
	int idx=0;
	for ( ;regulatory[idx].addr; idx++)
		eeprom_write16(reg_offs + regulatory[idx].addr, regulatory[idx].data);

	eeprom_unlock();
	printf("\nCard EEPROM patched successfully\n");
}

void die(  const char* format, ... ) {
	va_list args;
	printf("\n");    
	va_start( args, format );
	vfprintf( stderr, format, args );
	va_end( args );

	release_card();
	exit(1);
}

unsigned int read_id(const char *device, const char* param)
{
	FILE *f;
	unsigned int id;
	char path[512];
	sprintf(path, DEVICES_PATH "/%s/%s", device, param);
	if (!(f = fopen(path, "r")))
		return 0;
	fscanf(f,"%x", &id);
	fclose(f);
	return id;
}

void check_device(struct pcidev_id *id)
{
	int i;

	id->idx = -1;
	id->class = (read_id(id->device,"class") >> 8);
	if (!id->class) {
		printf("No such PCI device: %s\n", id->device);
	}
	id->ven   = read_id(id->device,"vendor");
	id->dev   = read_id(id->device,"device");
	id->sven  = read_id(id->device,"subsystem_vendor");
	id->sdev  = read_id(id->device,"subsystem_device");

	for(i=0; id->idx<0 && valid_ids[i].ven; i++)
		if(id->ven==valid_ids[i].ven && id->dev==valid_ids[i].dev)
			id->idx = i;
}

void map_device()
{
	FILE *f;
	unsigned int id;
	char path[512];
	unsigned char data[64];
	int i;
	unsigned int addr;
	sprintf(path, DEVICES_PATH "/%s/%s", dev.device, "config");
	if (!(f = fopen(path, "r")))
		return;
	fread(data, 64, 1, f);
	fclose(f);

	for (i=0x10; !offset && i<0x28;i+=4) {
		addr = ((unsigned int*)data)[i/4];
		if ((addr & 0xF) == 4)
			offset = addr & 0xFFFFFFF0;
	}
}

void search_card()
{
	DIR  *dir;
	struct dirent *dentry;
	struct pcidev_id id;
	struct pcidev_id *ids = NULL;
	int i,cnt=0;

	dir = opendir(DEVICES_PATH);
	if (!dir)
		die("Can't list PCI devices\n");
#ifdef DEBUG
	printf("PCI devices:\n");
#endif
	id.device = (char*) malloc(256 * sizeof(char));
	do {
		dentry = readdir(dir);
		if (!dentry || !strncmp(dentry->d_name, ".", 1))
			continue;

		strcpy(id.device, dentry->d_name);
		check_device(&id);
#ifdef DEBUG
		printf("    %s: class %04x   id %04x:%04x   subid %04x:%04x\n",
			id.device,
			id.class,
			id.ven,  id.dev,
			id.sven, id.sdev
			);
#endif
		if (id.idx >=0 ) {
			if(!cnt)
				ids = (struct pcidev_id*) malloc(sizeof(id));
			else
				ids = (struct pcidev_id*) realloc(ids, (cnt+1)*sizeof(id));
			ids[cnt].device = (char*) malloc(256 * sizeof(char));
			ids[cnt].class = id.class;
			ids[cnt].ven = id.ven; ids[cnt].sven = id.sven;
			ids[cnt].dev = id.dev; ids[cnt].sdev = id.sdev;
			ids[cnt].idx = id.idx;
			memcpy(ids[cnt].device, id.device, 256);
			cnt++;
		}
	}
	while (dentry);
	printf("Supported devices detected: %s\n", cnt ? "" : "\n  NONE");
	if (!cnt) goto nodev;

	for (i=0; i<cnt; i++) {
		printf("  [%d] %s [%s] %s (%04x:%04x, %04x:%04x)\n", i+1,
			ids[i].device,
			valid_ids[ids[i].idx].writable ? "RW" : "RO",
			valid_ids[ids[i].idx].name,
			ids[i].ven,  ids[i].dev,
			ids[i].sven, ids[i].sdev);
	}
	i++;
	while(i<=0 || i>cnt) {
		if (!i)	goto out;
		printf("Select device [1-%d] (or 0 to quit): ", cnt);
		scanf("%d", &i);
	}
	i--;
	dev.device = (char*) malloc(256 * sizeof(char));
	memcpy(dev.device, ids[i].device, 256);

out:
	free(id.device);
	for (i=0; i<cnt; i++) free(ids[i].device);
	free(ids);
	return;
nodev:
	free(id.device);
	exit(1);
}

int main(int argc, char** argv)
{
	char c;
	dev.device = NULL;

	while (1) {
		c = getopt_long(argc, argv, "d:r:w:hp", long_options, NULL);
		if (c == -1)
			break;
		switch(c) {
			case 'd':
				dev.device = optarg;
				break;
			case 'r':
				ofname = optarg;
				break;
			case 'w':
				ifname = optarg;
				break;
			case 'p':
				patch11n = true;
				break;
			case 'h':
				die("EEPROM reader/writer for intel wifi cards\n\nUsage: iwleeprom [-d device] [-r filename] [-w filename] [-p]\n\n\t-d device\tdevice in format 0000:00:00.0 (domain:bus:dev.func)\n\t-r filename\tdump eeprom to binary file\n\t-w filename\twrite eeprom from binary file\n\t-p\t\tpatch device eeprom to enable 802.11n\n\n", argv[0]);	
			default:
				die("Unknown param '%c'\n", c);
		}
	}

	if (!dev.device) search_card();
	if (!dev.device) exit(1);
	check_device(&dev);

	if (!dev.class)	exit(2);
	if (dev.idx < 0)
		die("Selected device not supported\n");

	printf("Using device %s [%s] %s \n",
		dev.device,
		valid_ids[dev.idx].writable ? "RW" : "RO",
		valid_ids[dev.idx].name);

	map_device();

	if (!offset)
		die("Can't obtain memory address\n");

	printf("address: %08x\n", offset);

	if(!ifname && !ofname && !patch11n)
		printf("No file names given or patch option selected!\nNo EEPROM actions will be performed, just write-enable test\n");

	init_card();

	if (ofname)
		eeprom_read(ofname);

	if (ifname && valid_ids[dev.idx].writable)
		eeprom_write(ifname);

	if (patch11n && valid_ids[dev.idx].writable)
		eeprom_patch11n();

	release_card();
	return 0;
}

