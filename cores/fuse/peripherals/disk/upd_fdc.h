/* upd_fdc.h: NEC floppy disk controller emulation
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

#ifndef FUSE_UPD_FDC_H
#define FUSE_UPD_FDC_H

#include <libspectrum.h>

#include "event.h"
#include "fdd.h"
#include "fuse.h"

typedef enum upd_type_t {
  UPD765A = 0,
  UPD765B,
} upd_type_t;

typedef enum upd_clock_t {
  UPD_CLOCK_4MHZ = 0,
  UPD_CLOCK_8MHZ,
} upd_clock_t;

typedef enum upd_scan_t {
  UPD_SCAN_EQ = 0,
  UPD_SCAN_LO,
  UPD_SCAN_HI,
} upd_scan_t;

typedef enum upd_cmd_id_t {
				/* ----v computer READ at execution phase*/
  UPD_CMD_READ_DATA = 0,		/* (non)deleted */
  UPD_CMD_READ_DIAG,
				/* ----v computer WRITE at execution phase*/
  UPD_CMD_WRITE_DATA,			/* (non)deleted */
  UPD_CMD_WRITE_ID,			/* :) format */
  UPD_CMD_SCAN,				/* equal/low-or-equal/high-or-equal */
				/* ----v NO data transfer at execution phase  */
  UPD_CMD_READ_ID,
				/* ----v NO RW head contact  */
  UPD_CMD_RECALIBRATE,
  UPD_CMD_SENSE_INT,
  UPD_CMD_SPECIFY,
  UPD_CMD_SENSE_DRIVE,
  UPD_CMD_VERSION,
  UPD_CMD_SEEK,
  UPD_CMD_INVALID,
} upd_cmd_id_t;

typedef struct upd_cmd_t {
  upd_cmd_id_t id;			/* command ID */
  libspectrum_byte mask;		/* && mask */
  libspectrum_byte value;		/* == value */
  int cmd_length;			/* command length */
  int res_length;			/* result length */
} upd_cmd_t;

typedef enum upd_intrq_t {
  UPD_INTRQ_NONE = 0,
  UPD_INTRQ_RESULT,
  UPD_INTRQ_EXE,
  UPD_INTRQ_READY,
  UPD_INTRQ_SEEK,
} upd_intrq_t;

typedef struct upd_fdc {
  fdd_t *current_drive;
  fdd_t *drive[4];		/* UPD765 control 4 drives */

  upd_type_t type;		/* UPD765A UPD765B */
  upd_clock_t clock;		/* clock rate ( 4/8 MHz ) */

  int stp_rate;			/* stepping rate ms */
  int hut_time;			/* head unload time ms */
  int hld_time;			/* head load time ms */
  int non_dma;			/* operating mode */
  int first_rw;			/* first sector alway read/write even EOT < R */

  int spin_cycles;
  fdd_dir_t direction;		/* 0 = spindlewards, 1 = rimwards */
  upd_intrq_t intrq;		/* last INTRQ status */
  int datarq;			/* DRQ line status */

  enum upd_fdc_state {
    UPD_FDC_STATE_CMD = 0,
    UPD_FDC_STATE_EXE,
    UPD_FDC_STATE_RES,
  } state;

  int id_track;
  int id_head;
  int id_sector;
  int id_length;		/* sector length code 0, 1, 2, 3 */
  int sector_length;		/* sector length from length code */
  int ddam;			/* read a deleted data mark */
  int rev;			/* revolution counter */
  int head_load;		/* head state */
  int read_id;			/* searching an IDAM */
  enum upd_fdc_am_type {
    UPD_FDC_AM_NONE = 0,
    UPD_FDC_AM_ID,
  } id_mark;

  unsigned int last_sector_read;/* for Speedlock 'random' sector hack */
  int speedlock;		/* for Speedlock 'random' sector hack, -1 -> disable */

  /* state during transfer */
  int data_offset;

  int cycle;			/* read/write cycle num */
  int del_data;			/* READ/WRITE deleted data */
  int mt;			/* multitrack operations */
  int mf;			/* MFM mode */
  int sk;			/* skip deleted/not deleted data */
  int hd;			/* physical head address */
  int us;			/* unit select 0-3 */
  int pcn[4];			/* present cylinder numbers */
  int ncn[4];			/* new cylinder numbers */
  int rec[4];			/* recalibrate store pcns */
  int seek[4];			/* seek status for 4 drive */
  int seek_age[4];		/* order of overlapped seeks for 4 drive */
  int rlen;			/* expected record length */
  upd_scan_t scan;		/* SCAN type: eq/lo/hi */
  
  upd_cmd_t *cmd;			/* current command */

  libspectrum_byte command_register;    /* command register */
  libspectrum_byte data_register[9];    /* data registers */
  libspectrum_byte main_status;		/* main status register */
  libspectrum_byte status_register[4];  /* status registers */

  libspectrum_byte sense_int_res[2];	/* result bytes for SENSE INTERRUPT */

  libspectrum_word crc;			/* to hold crc */

  void ( *set_intrq ) ( struct upd_fdc *f );
  void ( *reset_intrq ) ( struct upd_fdc *f );
  void ( *set_datarq ) ( struct upd_fdc *f );
  void ( *reset_datarq ) ( struct upd_fdc *f );

} upd_fdc;

void upd_fdc_init_events( void );

/* allocate an fdc */
upd_fdc *upd_fdc_alloc_fdc( upd_type_t type, upd_clock_t clock );
void upd_fdc_master_reset( upd_fdc *f );

libspectrum_byte upd_fdc_read_status( upd_fdc *f );

libspectrum_byte upd_fdc_read_data( upd_fdc *f );
void upd_fdc_write_data( upd_fdc *f, libspectrum_byte b );

#endif                  /* #ifndef FUSE_UPD_FDC_H */
