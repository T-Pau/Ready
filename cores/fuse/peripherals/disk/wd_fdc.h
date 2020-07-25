/* wd_fdc.h: Western Digital floppy disk controller emulation
   Copyright (c) 2003-2015 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Gergely Szasz

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

   Philip: philip-fuse@shadowmagic.org.uk

   Stuart: stuart.brady@gmail.com

*/

#ifndef FUSE_WD_FDC_H
#define FUSE_WD_FDC_H

#include <libspectrum.h>

#include "event.h"
#include "fdd.h"
#include "fuse.h"

/* Status register bits */
enum {
  WD_FDC_SR_MOTORON = 1<<7, /* Motor on or READY */
  WD_FDC_SR_WRPROT  = 1<<6, /* Write-protect */
  WD_FDC_SR_SPINUP  = 1<<5, /* Record type / Spin-up complete */
  WD_FDC_SR_RNF     = 1<<4, /* Record Not Found or SEEK Error */
  WD_FDC_SR_CRCERR  = 1<<3, /* CRC error */
  WD_FDC_SR_LOST    = 1<<2, /* Lost data or TRACK00 */
  WD_FDC_SR_IDX_DRQ = 1<<1, /* Index pulse / Data request */
  WD_FDC_SR_BUSY    = 1<<0  /* Busy (command under execution) */
};

/* Configuration flags (interface-specific) */
enum {
  WD_FLAG_NONE      = 0,
  /* The Beta 128 connects the HLD output pin to the READY input pin and
   * the MOTOR ON output pin on the FDD interface. */
  WD_FLAG_BETA128   = 1<<0,
  /* The Opus Discovery needs a pulse of the DRQ (data request) line for every
   * byte transferred. */
  WD_FLAG_DRQ       = 1<<1,
  /* The MB-02+ provides a READY signal to FDC instead of FDD, so we use
   * 'extra_signal' for this */
  WD_FLAG_RDY       = 1<<2,
  /* HLT (input) pin not connected at all, so we assume it is always 1. */
  WD_FLAG_NOHLT     = 1<<3
};

typedef enum wd_type_t {
  WD1773 = 0,
  FD1793,
  WD1770,
  WD1772,
  WD2797
} wd_type_t;

typedef struct wd_fdc {
  fdd_t *current_drive;

  wd_type_t type;	/* WD1770, FD1793, WD1772, WD1773, WD2797 */

  int rates[ 4 ];
  int spin_cycles;
  fdd_dir_t direction;	/* 0 = spindlewards, 1 = rimwards */
  int dden;		/* SD/DD -> FM/MFM */
  int intrq;		/* INTRQ line status */
  int datarq;		/* DRQ line status */
  int head_load;	/* WD1773/FD1793 */
  int hlt;		/* WD1773/FD1793 Head Load Timing input pin */
  int hlt_time;		/* "... When a logic high is found on the HLT input
			   the head is assumed to be enganged. It is typically
			   derived from a 1 shot triggered by HLD ..."
			   if hlt_time > 0 it means trigger time in ms, if = 0
			   then hlt should be set with wd_fdc_set_hlt()  */
  unsigned int flags;	/* Configuration flags (interface-specific) */
  int extra_signal;	/* Extra line for boards with non-standard wiring */

  enum wd_fdc_state {
    WD_FDC_STATE_NONE = 0,
    WD_FDC_STATE_SEEK,
    WD_FDC_STATE_SEEK_DELAY,
    WD_FDC_STATE_VERIFY,
    WD_FDC_STATE_READ,
    WD_FDC_STATE_WRITE,
    WD_FDC_STATE_READTRACK,
    WD_FDC_STATE_WRITETRACK,
    WD_FDC_STATE_READID,
  } state;

  int read_id;		/* FDC try to read a DAM */

  enum wd_fdc_status_type {
    WD_FDC_STATUS_TYPE1,
    WD_FDC_STATUS_TYPE2,
  } status_type;

  enum wd_fdc_am_type {
    WD_FDC_AM_NONE = 0,
    WD_FDC_AM_INDEX,
    WD_FDC_AM_ID,
    WD_FDC_AM_DATA,
  } id_mark;

  int id_track;
  int id_head;
  int id_sector;
  int id_length;	/* sector length code 0, 1, 2, 3 */
  int non_ibm_len_code;	/* WD2797 can use alternative sector len code set */
  int sector_length;	/* sector length from length code */
  int ddam;		/* read a deleted data mark */
  int rev;		/* revolution counter */

  /* state during transfer */
  int data_check_head;	/* -1 no check, 0/1 wait side 0 or 1 */
  int data_multisector;
  int data_offset;

  libspectrum_byte command_register;    /* command register */
  libspectrum_byte status_register;     /* status register */
  libspectrum_byte track_register;      /* track register */
  libspectrum_byte sector_register;     /* sector register */
  libspectrum_byte data_register;       /* data register */

  libspectrum_word crc;			/* to hold crc */

  void ( *set_intrq ) ( struct wd_fdc *f );
  void ( *reset_intrq ) ( struct wd_fdc *f );
  void ( *set_datarq ) ( struct wd_fdc *f );
  void ( *reset_datarq ) ( struct wd_fdc *f );

} wd_fdc;

void wd_fdc_init_events( void );

/* allocate an fdc */
wd_fdc *wd_fdc_alloc_fdc( wd_type_t type, int hlt_time, unsigned int flags );
void wd_fdc_master_reset( wd_fdc *f );

libspectrum_byte wd_fdc_sr_read( wd_fdc *f );
void wd_fdc_cr_write( wd_fdc *f, libspectrum_byte b );

libspectrum_byte wd_fdc_tr_read( wd_fdc *f );
void wd_fdc_tr_write( wd_fdc *f, libspectrum_byte b );

libspectrum_byte wd_fdc_sec_read( wd_fdc *f );
void wd_fdc_sec_write( wd_fdc *f, libspectrum_byte b );

libspectrum_byte wd_fdc_dr_read( wd_fdc *f );
void wd_fdc_dr_write( wd_fdc *f, libspectrum_byte b );

void wd_fdc_set_intrq( wd_fdc *f );
void wd_fdc_reset_intrq( wd_fdc *f );
void wd_fdc_set_datarq( wd_fdc *f );
void wd_fdc_reset_datarq( wd_fdc *f );
void wd_fdc_set_hlt( wd_fdc *f, int hlt );

#endif                  /* #ifndef FUSE_WD_FDC_H */
