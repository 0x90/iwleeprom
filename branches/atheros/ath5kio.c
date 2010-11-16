
 /*
  * This program is derived from code bearing the following Copyright(s)
  */

 /*                                                        -*- linux-c -*-
  *   _  _ ____ __ _ ___ ____ ____ __ _ _ _ _ |
  * .  \/  |--| | \|  |  |--< [__] | \| | _X_ | s e c u r e  s y s t e m s
  *
  * .vt|ar5k - PCI/CardBus 802.11a WirelessLAN driver for Atheros AR5k chipsets
  *
  * Copyright (c) 2002, .vantronix | secure systems
  *                     and Reyk Floeter <reyk_(_at_)_va_(_dot_)__(_dot_)__(_dot_)_>
  *
  * This program is free software ; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation ; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY ; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program ; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

 /*
  * Modified by Jan Krupa for EEPROM read/write/repair
  * Version 1.0
  */


/*
* Copyright (C) 2010, Gennady "ShultZ" Kozlov <qpxtool@mail.ru>
*/

#include "ath5kio.h"

#define ATH_EEPROM_SIGNATURE    0x7801
#define ATH_MMAP_LENGTH 0x10000


/* ATH5K defines */
#define AR5K_PCICFG 0x4010 
#define AR5K_PCICFG_EEAE 0x00000001 
#define AR5K_PCICFG_CLKRUNEN 0x00000004 
#define AR5K_PCICFG_LED_PEND 0x00000020 
#define AR5K_PCICFG_LED_ACT 0x00000040 
#define AR5K_PCICFG_SL_INTEN 0x00000800 
#define AR5K_PCICFG_BCTL		 0x00001000 
#define AR5K_PCICFG_SPWR_DN 0x00010000 

#define AR5211_EEPROM_ADDR 0x6000 
#define AR5211_EEPROM_DATA 0x6004
#define AR5211_EEPROM_COMD 0x6008
#define AR5211_EEPROM_COMD_READ 0x0001
#define AR5211_EEPROM_COMD_WRITE 0x0002
#define AR5211_EEPROM_COMD_RESET 0x0003
#define AR5211_EEPROM_STATUS 0x600C
#define AR5211_EEPROM_STAT_RDERR 0x0001
#define AR5211_EEPROM_STAT_RDDONE 0x0002
#define AR5211_EEPROM_STAT_WRERR 0x0003
#define AR5211_EEPROM_STAT_WRDONE 0x0004
#define AR5211_EEPROM_CONF 0x6010

/* PCI R/W macros */
#define VT_WLAN_IN32(a)  (*((volatile unsigned long int *)(dev->mem + (a))))
#define VT_WLAN_OUT32(v,a) (*((volatile unsigned long int *)(dev->mem + (a))) = (v))

void ath5k_eeprom_lock(struct pcidev *dev) {}

void ath5k_eeprom_unlock(struct pcidev *dev) {}

const uint16_t ath5k_eeprom_read16(struct pcidev *dev, unsigned int addr)
{
	int timeout = 10000 ;
 	unsigned long int status ;
	if (!dev->mem)
		return buf_read16(addr);

 	VT_WLAN_OUT32( 0, AR5211_EEPROM_CONF ),
 	usleep( 5 ) ;
 
 	/** enable eeprom read access */
 	VT_WLAN_OUT32( VT_WLAN_IN32(AR5211_EEPROM_COMD)
 		     | AR5211_EEPROM_COMD_RESET, AR5211_EEPROM_COMD) ;
 	usleep( 5 ) ;
 
 	/** set address */
 	VT_WLAN_OUT32( addr, AR5211_EEPROM_ADDR) ;
 	usleep( 5 ) ;
 
 	VT_WLAN_OUT32( VT_WLAN_IN32(AR5211_EEPROM_COMD)
 		     | AR5211_EEPROM_COMD_READ, AR5211_EEPROM_COMD) ;
 
 	while (timeout > 0) {
 		usleep(1) ;
 		status = VT_WLAN_IN32(AR5211_EEPROM_STATUS) ;
 		if (status & AR5211_EEPROM_STAT_RDDONE) {
 			if (status & AR5211_EEPROM_STAT_RDERR) {
 				die( "eeprom read access failed!\n");
 			}
 			status = VT_WLAN_IN32(AR5211_EEPROM_DATA) ;
 			return (status & 0x0000ffff);
 		}
 		timeout-- ;
 	}
 	die( "eeprom read timeout!\n");
	return 0;
}

void ath5k_eeprom_write16(struct pcidev *dev, unsigned int addr, uint16_t value)
{
 	int timeout = 10000 ;
 	unsigned long int status ;
 	unsigned long int pcicfg ;
 	int i ;
 	unsigned short int sdata ;

	if (!dev->mem) {
		buf_write16(addr, value);
		return;
	}


 	/** enable eeprom access */
 	pcicfg = VT_WLAN_IN32( AR5K_PCICFG ) ;
	VT_WLAN_OUT32( ( pcicfg & ~AR5K_PCICFG_SPWR_DN ), AR5K_PCICFG ) ;
	usleep( 500 ) ;
 	VT_WLAN_OUT32( pcicfg | AR5K_PCICFG_EEAE /* | 0x2 */, AR5K_PCICFG) ;
 	usleep( 50 ) ;
 
 	VT_WLAN_OUT32( 0, AR5211_EEPROM_STATUS );
 	usleep( 50 ) ;
 
 	/* VT_WLAN_OUT32( 0x1, AR5211_EEPROM_CONF ) ; */
 	VT_WLAN_OUT32( 0x0, AR5211_EEPROM_CONF ) ;
 	usleep( 50 ) ;
 
 	i = 100 ;
 retry:
 	/** enable eeprom write access */
 	VT_WLAN_OUT32( AR5211_EEPROM_COMD_RESET, AR5211_EEPROM_COMD);
 	usleep( 500 ) ;
 
 	/* Write data */
 	VT_WLAN_OUT32( value, AR5211_EEPROM_DATA );
 	usleep( 5 ) ;
 
 	/** set address */
 	VT_WLAN_OUT32( addr, AR5211_EEPROM_ADDR);
 	usleep( 5 ) ;
 
 	VT_WLAN_OUT32( AR5211_EEPROM_COMD_WRITE, AR5211_EEPROM_COMD);
 	usleep( 5 ) ;
 
 	for ( timeout = 10000 ; timeout > 0 ; --timeout ) {
 		status = VT_WLAN_IN32( AR5211_EEPROM_STATUS );
 		if ( status & 0xC ) {
 			if ( status & AR5211_EEPROM_STAT_WRERR ) {
 				die("eeprom write access failed!\n");
 			}
 			VT_WLAN_OUT32( 0, AR5211_EEPROM_STATUS );
 			usleep( 10 ) ;
 			break ;
 		}
 		usleep( 10 ) ;
 		timeout--;
 	}
 	sdata = dev->ops->eeprom_read16( dev, addr) ;
 	if ( ( sdata != value ) && i ) {
 		--i ;
 		fprintf( stderr, "Retrying eeprom write!\n");
 		goto retry ;
 	}
}

struct dev_ops dev_ops_ath5k = {
	.mmap_size        = ATH_MMAP_LENGTH,
	.eeprom_signature = ATH_EEPROM_SIGNATURE,

	.eeprom_lock     = &ath5k_eeprom_lock,
	.eeprom_unlock   = &ath5k_eeprom_unlock,
	.eeprom_read16   = &ath5k_eeprom_read16,
	.eeprom_write16  = &ath5k_eeprom_write16,
	.eeprom_patch11n = NULL
};
