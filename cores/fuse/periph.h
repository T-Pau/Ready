/* periph.h: code for handling peripherals
   Copyright (c) 2004-2016 Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Gergely Szasz

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_PERIPH_H
#define FUSE_PERIPH_H

#include <libspectrum.h>

/* The types of peripheral Fuse knows about */
typedef enum periph_type {
  PERIPH_TYPE_UNKNOWN,

  PERIPH_TYPE_128_MEMORY,     /* 128K-style memory paging */
  PERIPH_TYPE_AY,             /* 128K-style AY chip */
  PERIPH_TYPE_AY_FULL_DECODE, /* 128K-style AY chip responding only to 0xfffd */
  PERIPH_TYPE_AY_PLUS3,       /* 128K-style AY chip with read from data port */
  PERIPH_TYPE_AY_TIMEX,       /* Timex-style AY chip */
  PERIPH_TYPE_AY_TIMEX_WITH_JOYSTICK, /* Timex-style AY chip with joystick */
  PERIPH_TYPE_BETA128,        /* Beta128 disk interface */
  PERIPH_TYPE_BETA128_PENTAGON, /* Beta128 disk interface as found on the original Pentagon */
  PERIPH_TYPE_BETA128_PENTAGON_LATE, /* Beta128 disk interface as found on later Pentagons */
  PERIPH_TYPE_COVOX_DD,       /* Covox interface on port #dd */
  PERIPH_TYPE_COVOX_FB,       /* Covox interface on port #fb */
  PERIPH_TYPE_DIVIDE,         /* DivIDE interface */
  PERIPH_TYPE_DIVMMC,         /* DivMMC interface */
  PERIPH_TYPE_PLUSD,          /* +D disk interface */
  PERIPH_TYPE_DIDAKTIK80,     /* Didaktik 40/80 disk interface */
  PERIPH_TYPE_DISCIPLE,       /* DISCiPLE disk interface */
  PERIPH_TYPE_FULLER,         /* Fuller box */
  PERIPH_TYPE_INTERFACE1,     /* Interface 1 */
  PERIPH_TYPE_INTERFACE2,     /* Interface 2 */

  /* A Kempston joystick which requires b5, b6 and b7 reset to be read */
  PERIPH_TYPE_KEMPSTON,
  /* A Kempston joystick which requires b5 only reset to be read */
  PERIPH_TYPE_KEMPSTON_LOOSE,

  PERIPH_TYPE_KEMPSTON_MOUSE, /* Kempston mouse */
  PERIPH_TYPE_MELODIK,        /* Melodik interface */
  PERIPH_TYPE_MULTIFACE_1,    /* Multiface 1 */
  PERIPH_TYPE_MULTIFACE_128,  /* Multiface 128 */
  PERIPH_TYPE_MULTIFACE_3,    /* Multiface 3 */
  PERIPH_TYPE_OPUS,           /* Opus disk interface */
  PERIPH_TYPE_PARALLEL_PRINTER, /* +2A/+3 parallel printer */
  PERIPH_TYPE_PENTAGON1024_MEMORY, /* Pentagon 1024-style memory paging */
  PERIPH_TYPE_PLUS3_MEMORY,   /* +2A/+3-style memory paging */
  PERIPH_TYPE_SCLD,           /* Timex SCLD */
  PERIPH_TYPE_SE_MEMORY,      /* Spectrum SE-style memory paging */
  PERIPH_TYPE_SIMPLEIDE,      /* Simple 8-bit IDE interface */
  PERIPH_TYPE_SPECCYBOOT,     /* SpeccyBoot interface */
  PERIPH_TYPE_SPECDRUM,       /* SpecDrum interface */
  PERIPH_TYPE_SPECTRANET,     /* Spectranet interface */
  PERIPH_TYPE_ULA,            /* Standard ULA */
  PERIPH_TYPE_ULA_FULL_DECODE,/* Standard ULA responding only to 0xfe */
  PERIPH_TYPE_UPD765,         /* +3 uPD765 FDC */
  PERIPH_TYPE_USOURCE,        /* Currah uSource interface */
  PERIPH_TYPE_ZXATASP,        /* ZXATASP IDE interface */
  PERIPH_TYPE_ZXCF,           /* ZXCF IDE interface */
  PERIPH_TYPE_ZXMMC,          /* ZXMMC interface */
  PERIPH_TYPE_ZXPRINTER,      /* ZX Printer */
  PERIPH_TYPE_ZXPRINTER_FULL_DECODE, /* ZX Printer responding only to 0xfb */
} periph_type;

/*
 * General peripheral list handling routines
 */

/* For indicating the (optional) presence or absence of a peripheral */
typedef enum periph_present {
  PERIPH_PRESENT_NEVER,		/* Never present */
  PERIPH_PRESENT_OPTIONAL,	/* Optionally present */
  PERIPH_PRESENT_ALWAYS,	/* Always present */
} periph_present;

typedef libspectrum_byte (*periph_port_read_function)( libspectrum_word port,
						       libspectrum_byte *attached );
typedef void (*periph_port_write_function)( libspectrum_word port,
					    libspectrum_byte data );

/* Information about a specific port response */
typedef struct periph_port_t {

  /* This peripheral responds to all port values where
     <port> & mask == value */
  libspectrum_word mask;
  libspectrum_word value;

  periph_port_read_function read;
  periph_port_write_function write;

} periph_port_t;

typedef void (*periph_activate_function)( void );

/* Information about a peripheral */
typedef struct periph_t {
  /* The preferences option which controls this peripheral */
  int *option;
  /* The list of ports this peripheral responds to */
  const periph_port_t *ports;
  /* Hard reset required when added/removed */
  int hard_reset;
  /* Function to be called when the peripheral is activated */
  periph_activate_function activate;
} periph_t;

/* Register a peripheral with the system */
void periph_register( periph_type type, const periph_t *periph );

/* Set whether a peripheral can be present on this machine or not */
void periph_set_present( periph_type type, periph_present present );

/* Mark a specific peripheral as (in)active, returns 1 if the enabled state has
   changed */
int periph_activate_type( periph_type type, int active );

/* Is a specific peripheral active at the moment? */
int periph_is_active( periph_type type );

/* Empty out the list of peripherals */
void periph_clear( void );

/* Free up peripherals */
void periph_end( void );

/*
 * The actual routines to read and write a port
 */

libspectrum_byte readport( libspectrum_word port );
libspectrum_byte readport_internal( libspectrum_word port );

void writeport( libspectrum_word port, libspectrum_byte b );
void writeport_internal( libspectrum_word port, libspectrum_byte b );

/*
 * The more Fuse-specific peripheral handling routines
 */

int periph_update( void );

void periph_posthook( void );

int periph_postcheck( void );

void periph_disable_optional( void );

/* Register debugger page/unpage events for a peripheral */
void periph_register_paging_events( const char *type_string, int *page_event,
				    int *unpage_event );

libspectrum_byte periph_merge_floating_bus( libspectrum_byte value,
                                            libspectrum_byte attached,
                                            libspectrum_byte floating_bus );

#endif				/* #ifndef FUSE_PERIPH_H */
