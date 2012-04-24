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

#define _GNU_SOURCE

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>
#include <getopt.h>

//#define DEBUG

#include "iwlio.h"
#include "ath5kio.h"
#include "ath9kio.h"

extern uint16_t buf[EEPROM_SIZE_MAX/2];

static struct option long_options[] = {
	{"device",    1, NULL, 'd'},
	{"nodev",     0, NULL, 'n'},
	{"preserve-mac", 0, NULL, 'm'},
	{"preserve-calib", 0, NULL, 'c'},
	{"read",      0, NULL, 'r'},
	{"write",     0, NULL, 'w'},
	{"ifile",     1, NULL, 'i'},
	{"ofile",     1, NULL, 'o'},
	{"bigendian", 0, NULL, 'b'},
	{"help",      0, NULL, 'h'},
	{"list",      0, NULL, 'l'},
	{"debug",     1, NULL, 'D'},
	{"parse",     0, NULL, 'P'},
	{"init",      0, NULL, 'I'},
	{"patch11n",  0, NULL, 'p'}
};

int mem_fd;
unsigned int offset;
bool eeprom_locked;
enum byte_order dump_order;
uid_t ruid,euid,suid;

void die(  const char* format, ... ); 

char	*ifname = NULL,
		*ofname = NULL;
bool patch11n = false,
	 init_device = false,
	 parse = false,
	 nodev = false,
	 preserve_mac = false,
	 preserve_calib = false;

unsigned int  debug = 0;

uint16_t buf[EEPROM_SIZE_MAX/2];

struct pcidev dev;

#define DEVICES_PATH "/sys/bus/pci/devices"

const struct pci_id valid_ids[] = {
/* Intel devices */
	{ INTEL_PCI_VID,   0x0082, &dev_ops_iwl6k, "6000 Series Gen2"},
	{ INTEL_PCI_VID,   0x0083, &dev_ops_iwl6k, "Centrino Wireless-N 1000"},
	{ INTEL_PCI_VID,   0x0084, &dev_ops_iwl6k, "Centrino Wireless-N 1000"},
	{ INTEL_PCI_VID,   0x0085, &dev_ops_iwl6k, "6000 Series Gen2"},
	{ INTEL_PCI_VID,   0x0087, &dev_ops_iwl6k, "Centrino Advanced-N + WiMAX 6250"},
	{ INTEL_PCI_VID,   0x0089, &dev_ops_iwl6k, "Centrino Advanced-N + WiMAX 6250"},
	{ INTEL_PCI_VID,   0x0885, &dev_ops_iwl6k, "WiFi+WiMAX 6050 Series Gen2"},
	{ INTEL_PCI_VID,   0x0886, &dev_ops_iwl6k, "WiFi+WiMAX 6050 Series Gen2"},
	{ INTEL_PCI_VID,   0x4229, &dev_ops_iwl4965, "PRO/Wireless 4965 AG or AGN [Kedron] Network Connection"},
	{ INTEL_PCI_VID,   0x422b, &dev_ops_iwl6k, "Centrino Ultimate-N 6300"},
	{ INTEL_PCI_VID,   0x422c, &dev_ops_iwl6k, "Centrino Advanced-N 6200"},
	{ INTEL_PCI_VID,   0x4230, &dev_ops_iwl4965, "PRO/Wireless 4965 AG or AGN [Kedron] Network Connection"},
	{ INTEL_PCI_VID,   0x4232, &dev_ops_iwl5k, "WiFi Link 5100"},
	{ INTEL_PCI_VID,   0x4235, &dev_ops_iwl5k, "Ultimate N WiFi Link 5300"},
	{ INTEL_PCI_VID,   0x4236, &dev_ops_iwl5k, "Ultimate N WiFi Link 5300"},
	{ INTEL_PCI_VID,   0x4237, &dev_ops_iwl5k, "PRO/Wireless 5100 AGN [Shiloh] Network Connection"},
	{ INTEL_PCI_VID,   0x4238, &dev_ops_iwl6k, "Centrino Ultimate-N 6300"},
	{ INTEL_PCI_VID,   0x4239, &dev_ops_iwl6k, "Centrino Advanced-N 6200"},
	{ INTEL_PCI_VID,   0x423a, &dev_ops_iwl5k, "PRO/Wireless 5350 AGN [Echo Peak] Network Connection"},
	{ INTEL_PCI_VID,   0x423b, &dev_ops_iwl5k, "PRO/Wireless 5350 AGN [Echo Peak] Network Connection"},
	{ INTEL_PCI_VID,   0x423c, &dev_ops_iwl5k, "WiMAX/WiFi Link 5150"},
	{ INTEL_PCI_VID,   0x423d, &dev_ops_iwl5k, "WiMAX/WiFi Link 5150"},

/* Atheros 5k devices */
	{ ATHEROS_PCI_VID, 0x0007, &dev_ops_ath5k, "AR5000 802.11a Wireless Adapter" },
	{ ATHEROS_PCI_VID, 0x0011, &dev_ops_ath5k, "AR5210 802.11a NIC" },
	{ ATHEROS_PCI_VID, 0x0012, &dev_ops_ath5k, "AR5211 802.11ab NIC" },
	{ ATHEROS_PCI_VID, 0x0013, &dev_ops_ath5k, "Atheros AR5001X+ Wireless Network Adapter" },
	{ ATHEROS_PCI_VID, 0x001a, &dev_ops_ath5k, "AR2413 802.11bg NIC" },
	{ ATHEROS_PCI_VID, 0x001b, &dev_ops_ath5k, "AR5413 802.11abg NIC" },
	{ ATHEROS_PCI_VID, 0x001c, &dev_ops_ath5k, "AR5001 Wireless Network Adapter" },
	{ ATHEROS_PCI_VID, 0x001d, &dev_ops_ath5k, "AR5007G Wireless Network Adapter" },
	{ ATHEROS_PCI_VID, 0x0020, &dev_ops_ath5k, "AR5513 802.11abg Wireless NIC" },
	{ ATHEROS_PCI_VID, 0x0207, &dev_ops_ath5k, "AR5210 802.11abg" },
	{ ATHEROS_PCI_VID, 0x1014, &dev_ops_ath5k, "AR5212 802.11abg" },

/* Atheros 9k devices */
	{ ATHEROS_PCI_VID, 0x0023, &dev_ops_ath9k, "AR5008 Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x0024, &dev_ops_ath9k, "AR5008 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0027, &dev_ops_ath9k, "AR9160 802.11abgn Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x0029, &dev_ops_ath9k, "AR922X Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x002A, &dev_ops_ath9k, "AR928X Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x002B, &dev_ops_ath9k, "AR9285 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x002C, &dev_ops_ath9k, "AR2427 Wireless Adapter (PCI-E)" }, /* PCI-E 802.11n bonded out */
	{ ATHEROS_PCI_VID, 0x002D, &dev_ops_ath9k, "AR9287 Wireless Adapter (PCI)" },
	{ ATHEROS_PCI_VID, 0x002E, &dev_ops_ath9k, "AR9287 Wireless Adapter (PCI-E)" },
	{ ATHEROS_PCI_VID, 0x0030, &dev_ops_ath9k, "AR9300 Wireless Adapter (PCI-E)" },
//	{ ATHEROS_PCI_VID, 0x0033, &dev_ops_ath9k, "11a/b/g/n Wireless LAN Mini-PCI Express Adapter" },

	{ 0, 0, NULL, "" }
};

void init_card()
{
	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("cannot open /dev/mem\n");
		exit(1);
	}

	dev.mem = (unsigned char *)mmap(NULL, dev.ops->mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, mem_fd, offset);
	if (dev.mem == MAP_FAILED) {
		perror("mmap failed");
		exit(1);
	}
}

void release_card()
{
	if (dev.mem != NULL)
		munmap(dev.mem, dev.ops->mmap_size);
}

void init_dump(struct pcidev *dev, char *filename)
{
	FILE *fd;
	uint32_t eeprom_size;

	seteuid(ruid);
	if (!(fd = fopen(filename, "rb")))
		die("Can't read file '%s'\n", filename);
	eeprom_size = 2 * fread(buf, 2, EEPROM_SIZE_MAX/2, fd);
	fclose(fd);
	seteuid(suid);

	printf("Dump file: '%s' (read %u bytes)\n", filename, eeprom_size);
	if(eeprom_size <dev_ops_iwl4965.eeprom_size)
		die("Too small file!\n");

	if(eeprom_size == dev_ops_iwl4965.eeprom_size)
		dev->ops = &dev_ops_iwl4965;	
	else
		dev->ops = &dev_ops_iwl5k;

	if ( dev->ops->eeprom_signature == le2cpu16(buf[0])) {
		dump_order = order_le;
	} else if ( dev->ops->eeprom_signature == be2cpu16(buf[0])) {
		dump_order = order_be;
	} else {
		die("Invalid EEPROM signature!\n");
	}
	printf("  byte order: %s ENDIAN\n", (dump_order == order_le) ? "LITTLE":"BIG");
}

void fixate_dump(struct pcidev *dev, char *filename)
{
	FILE *fd;
	seteuid(ruid);

	if (!(fd = fopen(filename, "wb")))
		die("Can't create file '%s'\n", filename);
	fwrite(buf, dev->ops->eeprom_size, 1, fd);
	printf("Dump file written: '%s'\n", filename);
	fclose(fd);

	seteuid(suid);
}

bool buf_read16(uint32_t addr, uint16_t *value)
{
	if (addr >= EEPROM_SIZE_MAX) return 0;
	if (dump_order == order_le)
		*value = le2cpu16(buf[addr >> 1]);
	else
		*value = be2cpu16(buf[addr >> 1]);
	return true;
}

bool buf_write16(uint32_t addr, uint16_t value)
{
	if (addr >= EEPROM_SIZE_MAX) return false;
	if (dump_order == order_le)
		buf[addr >> 1] = cpu2le16(value);
	else
		buf[addr >> 1] = cpu2be16(value);
	return true;
}

void eeprom_read(char *filename)
{
	uint32_t addr = 0;
	uint16_t data;
	FILE *fd;
	
	printf("Saving dump with byte order: %s ENDIAN\n", (dump_order == order_le) ? "LITTLE":"BIG");

	for (addr = 0; addr < dev.ops->eeprom_size; addr += 2)
	{
		if (!dev.ops->eeprom_read16(&dev, addr, &data)) return;

		if (dump_order == order_le)
			buf[addr/2] = cpu2le16( data );
		else
			buf[addr/2] = cpu2be16( data );
		if (0 ==(addr & 0x7F)) printf("%04x [", addr);
		printf("x");
		if (0x7E ==(addr & 0x7F)) printf("]\n");
		fflush(stdout);
	}

	seteuid(ruid);
	if (!(fd = fopen(filename, "wb")))
		die("Can't create file '%s'\n", filename);
	fwrite(buf, dev.ops->eeprom_size, 1, fd);
	fclose(fd);
	seteuid(suid);

	printf("\nEEPROM has been dumped to '%s'\n", filename);
}

void eeprom_write(char *filename)
{
	unsigned int addr = 0;
	enum byte_order order = order_unknown;
	uint16_t value;
	uint16_t evalue;
	size_t   size;
	FILE *fd;

	seteuid(ruid);
	if (!(fd = fopen(filename, "rb")))
		die("Can't read file '%s'\n", filename);
	size = 2 * fread(buf, 2, dev.ops->eeprom_size/2, fd);
	fclose(fd);
	seteuid(suid);

	printf("Writing data to EEPROM...\n  '.' = match, 'x' = write\n");
	for(addr=0; addr<size;addr+=2)
	{
		if (order == order_unknown) {
			if ( buf[addr/2] && dev.ops->eeprom_signature == le2cpu16(buf[addr/2]) ) {
				order = order_le;
			} else if ( buf[addr/2] && dev.ops->eeprom_signature == be2cpu16(buf[addr/2]) ) {
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
		if (!dev.ops->eeprom_read16(&dev, addr, &evalue)) return;

		if (0 ==(addr & 0x7F)) printf("%04x [", addr);
		if (evalue != value) {
			dev.ops->eeprom_write16(&dev, addr, value);
			printf("x");
		} else {
			printf(".");
		}
		if (0x7E ==(addr & 0x7F)) printf("]\n");
		fflush(stdout);
	}

	printf("\nEEPROM has been written from '%s'\n", filename);
}

void die(  const char* format, ... ) {
	va_list args;
	fprintf(stderr, "\n\E[31;60m");
	va_start( args, format );
	vfprintf( stderr, format, args );
	va_end( args );
	fprintf(stderr, "\E[0m");

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

void check_device(struct pcidev *id)
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
		if(id->ven==valid_ids[i].ven && id->dev==valid_ids[i].dev) {
			id->idx = i;
			id->ops = valid_ids[i].ops;
		}
}

void list_supported()
{
	int i;
	printf("Known devices:\n");
	for(i=0; valid_ids[i].ven; i++)
		printf("  %04x:%04x [%s, %s] %s \n", 
			valid_ids[i].ven,
			valid_ids[i].dev,
			valid_ids[i].ops->eeprom_writable ? "RW" : "RO",
			valid_ids[i].ops->name,
			valid_ids[i].name);
}

void map_device()
{
	FILE *f;
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
	struct pcidev id;
	struct pcidev *ids = NULL;
	int i,cnt=0;

	dir = opendir(DEVICES_PATH);
	if (!dir)
		die("Can't list PCI devices\n");
	if (debug)
		printf("PCI devices:\n");
	id.device = (char*) malloc(256 * sizeof(char));
	do {
		dentry = readdir(dir);
		if (!dentry || !strncmp(dentry->d_name, ".", 1))
			continue;

		strcpy(id.device, dentry->d_name);
		check_device(&id);
		if (debug) {
			printf("    %s: class %04x   id %04x:%04x   subid %04x:%04x",
				id.device,
				id.class,
				id.ven,  id.dev,
				id.sven, id.sdev
			);
			if (id.idx < 0)
				printf("\n");
			else
				printf(" [%s, %s] %s \n", 
					valid_ids[id.idx].ops->eeprom_writable ? "RW" : "RO",
					valid_ids[id.idx].ops->name,
					valid_ids[id.idx].name);
		}
		if (id.idx >=0 ) {
			if(!cnt)
				ids = (struct pcidev*) malloc(sizeof(id));
			else
				ids = (struct pcidev*) realloc(ids, (cnt+1)*sizeof(id));
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
			valid_ids[ids[i].idx].ops->eeprom_writable ? "RW" : "RO",
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
	dev.mem    = NULL;
	dump_order = order_le;
	getresuid(&ruid, &euid, &suid);

	while (1) {
		c = getopt_long(argc, argv, "rwld:mcni:o:bhpPID:", long_options, NULL);
		if (c == -1)
			break;
		switch(c) {
			case 'l':
				list_supported();
				exit(0);
			case 'd':
				dev.device = optarg;
				break;
			case 'n':
				nodev = true;
				break;
			case 'm':
				preserve_mac = true;
				break;
			case 'c':
				preserve_calib = true;
				break;
			case 'r':
				die("option -r deprecated. use -o instead\n");
				break;
			case 'o':
				ofname = optarg;
				break;
			case 'w':
				die("option -w deprecated. use -i instead\n");
				break;
			case 'i':
				ifname = optarg;
				break;
			case 'b':
				dump_order = order_be;
				break;
			case 'P':
				parse = true;
				break;
			case 'p':
				patch11n = true;
				break;
			case 'I':
				init_device = true;
				break;
			case 'D':
				debug = atoi(optarg);
				if (debug)
					printf("debug level: %s\n", optarg);
				break;
			case 'h':
				die("EEPROM reader/writer for intel wifi cards\n\n"
					"Usage:\n"
					"\t%s [-d device [-m] [-c] | -n] [-I] [-i filename ] [-o filename [-b] ] [-P] [-p] [-D debug_level]\n"
					"\t%s -l\n"
					"\t%s -h\n\n"
					"Options:\n"
					"\t-d <device> | --device <device>\t\t"
					"device in format 0000:00:00.0 (domain:bus:dev.func)\n"
					"\t-n | --nodev\t\t\t\t"
					"don't touch any device, file-only operations\n"
					"\t-m | --preserve-mac\t\t\t"
					"don't change card's MAC while writing full eeprom dump\n"
					"\t-c | --preserve-calib\t\t\t"
					"don't change calibration data while writing full eeprom dump\n"
					"\t\t\t\t\t\t(not supported by ath9k)\n"
					"\t-o <filename> | --ofile <filename>\t"
					"dump eeprom to binary file\n"
					"\t-i <filename> | --ifile <filename>\t"
					"write eeprom from binary file\n"
					"\t-b | --bigendian\t\t\t"
					"save dump in big-endian byteorder (default: little-endian)\n"
					"\t-p | --patch11n\t\t\t\t"
					"patch device eeprom to enable 802.11n\n"
					"\t-I | --init\t\t\t\t"
					"init device (useful if driver didn't it)\n"
					"\t-P | --parse\t\t\t\t"
					"parse eeprom (show available modes/channels)\n"
					"\t-l | --list\t\t\t\t"
					"list known cards\n"
					"\t-D <level> | --debug <level>\t\t"
					"set debug level (0-1, default 0)\n"
					"\t-h | --help\t\t\t\t"
					"show this info\n", argv[0], argv[0], argv[0]);
			default:
				return 1;
		}
	}

	if (nodev) goto _nodev;

	if (!dev.device) search_card();
	if (!dev.device) exit(1);
	check_device(&dev);

	if (!dev.class)	exit(2);
	if (dev.idx < 0)
		die("Selected device not supported\n");

	printf("Using device %s [%s] %s \n",
		dev.device,
		dev.ops->eeprom_writable ? "RW" : "RO",
		valid_ids[dev.idx].name);
	printf("IO driver: %s\n",
		dev.ops->name);

	if (debug)
		printf("Supported ops: %s%s%s%s\n",
			dev.ops->eeprom_read16 ? " read" : "",
			(dev.ops->eeprom_writable && dev.ops->eeprom_write16)  ? " write" : "",
			dev.ops->eeprom_parse ? " parse" : "",
			dev.ops->eeprom_patch11n ? " patch11n" : ""
		);


	map_device();

	if (!offset)
		die("Can't obtain memory address\n");

	if (debug)
		printf("address: %08x\n", offset);

	if(!ifname && !ofname && !patch11n && !parse)
		printf("No file names given nor actions selected!\nNo EEPROM actions will be performed, just write-enable test\n");

	init_card();

	if (init_device && dev.ops->init_device && !dev.ops->init_device(&dev))
		die("Device init failed!\n");

	if (dev.ops->eeprom_check)
		dev.ops->eeprom_check(&dev);

	if (dev.ops->eeprom_lock && !dev.ops->eeprom_lock(&dev))
		die("Failed to lock eeprom!\n");

	if (ofname)
		eeprom_read(ofname);

	if (parse && dev.ops->eeprom_parse)
		dev.ops->eeprom_parse(&dev);

	if (ifname && dev.ops->eeprom_writable)
		eeprom_write(ifname);

	if (patch11n && dev.ops->eeprom_writable)
		dev.ops->eeprom_patch11n(&dev);

	if (parse && dev.ops->eeprom_parse && (ifname || patch11n)) {
		printf("\n\ndevice capabilities after eeprom writing:\n");
		dev.ops->eeprom_parse(&dev);
	}

	if (dev.ops->eeprom_release && !dev.ops->eeprom_release(&dev))
		die("Failed to unlock eeprom!\n");

	release_card();
	return 0;

_nodev:
	if (dev.device)
		die("Don't use '-d' and '-n' options simultaneously\n");

	printf("Device-less operation... (iwl ONLY)\n");

	if (!ifname)
		die("No input file given!\n");
	if (patch11n && !ofname)
		die("Have to specify output file for 802.11n patch!\n");
	init_dump(&dev, ifname);

	if (parse && dev.ops && dev.ops->eeprom_parse)
		dev.ops->eeprom_parse(&dev);

	if (patch11n && dev.ops && dev.ops->eeprom_patch11n)
		dev.ops->eeprom_patch11n(&dev);

	if (ofname)
		fixate_dump(&dev, ofname);
	return 0;
}
