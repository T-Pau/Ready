/* fdd.h: Routines for emulating floppy disk drives
   Copyright (c) 2007-2016 Gergely Szasz, Philip Kendall
   Copyright (c) 2015 Stuart Brady

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

*/

#ifndef FUSE_FDD_H
#define FUSE_FDD_H

#include "event.h"
#include "disk.h"

typedef enum fdd_error_t {
  FDD_OK = 0,
  FDD_GEOM,
  FDD_DATA,
  FDD_RDONLY,
  FDD_NONE,		/* FDD not exist (disabled) */

  FDD_LAST_ERROR,
} fdd_error_t;

typedef enum fdd_type_t {
  FDD_TYPE_NONE = 0,	/* FDD not exist/disabled */
  FDD_SHUGART,		/* head load when selected */
  /* 
     .. In a single drive system (program shunt position
        "MX" shorted), with program shunt position "HL"
	shorted, Drive Select when activated to a logical
	zero level, will load the R/W head against the
	diskette enabling contact of the R/W head against
	the media. ...
	
	In a multiple drive system (program shunt position
        "MX" open), the three input lines (Drive Select 1,
	Drive Select 2 and Drive select 3) are provided so
	that the using system may select which drive on
	the interface is to be used. In this mode of opera-
	tion only the drive with its Drive Select line active
	will respond to the input lines and gate the output
	lines. In addition, the selected drive will load its
	R/W head if program shunt position "HL" is
	shorted. ...
  */
  FDD_IBMPC,
} fdd_type_t;

typedef enum fdd_dir_t {
  FDD_STEP_OUT = 0,
  FDD_STEP_IN = 1,
} fdd_dir_t;

typedef struct fdd_t {
  fdd_type_t type;	/* fdd type: Shugart or IBMPC */
  int auto_geom;	/* change geometry according to loading disk */
  int fdd_heads;	/* 1 or 2 */
  int fdd_cylinders;	/* 40/40+/80/80+ */

  int tr00;		/* track 0 mark */
  int index;		/* index hole */
  int wrprot;		/* write protect */
  int data;		/* read/write to data byte 0x00nn or 0xffnn */
  int marks;		/* read/write other marks 0x01 -> FM 0x02 -> WEAK */

  disk_t disk;		/* disk */
  int loaded;		/* disk loaded */
  int upsidedown;	/* flipped disk */
  int selected;		/* Drive Select line active */
  int ready;		/* some disk drive offer a ready signal */
  int dskchg;		/* disk change signal */
  int hdout;		/* High Density signal */

  fdd_error_t status;

/* WD/FD 177X may wait for an index or RDY->/RDY or /RDY->RDY 
   we do not need more, just a subroutine and a pointer to fdc_struct 
*/
  void ( *fdc_index ) ( void *fdc );
  void *fdc;		/* if not NULL FDC wait for an index pulse */

/*--private section, fdc may never use it */
  int unreadable;	/* disk unreadable in this drive */
  int do_read_weak;
  int c_head;		/* current head (side) */
  int c_cylinder;	/* current cylinder number (0 -> TR00) */
  int c_bpt;		/* current track length in bytes */
  int motoron;		/* motor on */
  int loadhead;		/* head loaded */
  int index_pulse;	/* 'second' index hole, for index status */
} fdd_t;

typedef struct fdd_params_t {
  int enabled;
  int heads;
  int cylinders;
} fdd_params_t;

extern const fdd_params_t fdd_params[];

void fdd_register_startup( void );

const char *fdd_strerror( int error );
/* initialize the fdd_t struct, and set fdd_heads and cylinders (e.g. 2/83 ) */
int fdd_init( fdd_t *d, fdd_type_t type, const fdd_params_t *dt, int reinit );
/* load the given disk into the fdd. if upsidedown = 1, floppy upsidedown in drive :) */
int fdd_load( fdd_t *d, int upsidedown );
/* unload the disk from fdd */
void fdd_unload( fdd_t *d );
/* set fdd head */
void fdd_set_head( fdd_t *d, int head );
/* step one track according to d->direction direction. set d->tr00 if reach track 0 */
void fdd_step( fdd_t *d, fdd_dir_t direction );
/* set floppy position ( upsidedown or not )*/
void fdd_motoron( fdd_t *d, int on );
/* start (1) or stop (0) spindle motor */
void fdd_head_load( fdd_t *d, int load );
/* load (1) or unload (0) head */
void fdd_select( fdd_t *d, int select );
/* select (1) or unselect (0) FDD */
void fdd_flip( fdd_t *d, int upsidedown );
/* Read the next byte from disk. The byte will go into d->data.
   If d->data = 0xffnn then this byte was recorded with different clock 'mark'.
   d->idx is set if we reach the 'index hole'.  0x0100 is read if the disk is
   unreadable, the motor is not on, or the head is not loaded.
*/
int fdd_read_data( fdd_t *d );
/* Write the next byte to disk. The byte is taken from d->data.
   If d->data = 0xffnn then this byte recorded with different clock 'mark'.
   d->idx is set if we reach the 'index hole'.
*/
int fdd_write_data( fdd_t *d );
/* set write protect status on loaded disk */
void fdd_wrprot( fdd_t *d, int wrprot );
/* to reach index hole */
void fdd_wait_index_hole( fdd_t *d );
/* set floppy position ( upsidedown or not )*/
void fdd_flip( fdd_t *d, int upsidedown );

#endif 	/* FUSE_FDD_H */
