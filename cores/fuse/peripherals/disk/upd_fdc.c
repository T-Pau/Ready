/* upd_fdc.c: NEC floppy disk controller emulation
   Copyright (c) 2007-2015 Gergely Szasz
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

*/

#include <config.h>

#include <libspectrum.h>

#include "crc.h"
#include "event.h"
#include "fdd.h"
#include "fuse.h"
#include "ui/ui.h"
#include "upd_fdc.h"

#define MAX_SIZE_CODE 8

/* static const int UPD_FDC_MAIN_DRV_0_SEEK = 0x01; */
/* static const int UPD_FDC_MAIN_DRV_1_SEEK = 0x02; */
/* static const int UPD_FDC_MAIN_DRV_2_SEEK = 0x04; */
/* static const int UPD_FDC_MAIN_DRV_3_SEEK = 0x08; */
static const int UPD_FDC_MAIN_BUSY       = 0x10;
static const int UPD_FDC_MAIN_EXECUTION  = 0x20;
static const int UPD_FDC_MAIN_DATADIR    = 0x40;
static const int UPD_FDC_MAIN_DATA_READ  = 0x40;	/* computer should read */
static const int UPD_FDC_MAIN_DATA_WRITE = 0x00;	/* computer should write */
static const int UPD_FDC_MAIN_DATAREQ    = 0x80;

static const int UPD_FDC_ST0_NOT_READY   = 0x08;
static const int UPD_FDC_ST0_EQUIP_CHECK = 0x10;
static const int UPD_FDC_ST0_SEEK_END    = 0x20;
/* static const int UPD_FDC_ST0_INT_NORMAL  = 0x00; */    /* normal termination */
static const int UPD_FDC_ST0_INT_ABNORM  = 0x40;     /* abnormal termination */
/* static const int UPD_FDC_ST0_INT_INVALID = 0x80; */    /* invalid command */
static const int UPD_FDC_ST0_INT_READY   = 0xc0;     /* ready signal change */

static const int UPD_FDC_ST1_MISSING_AM  = 0x01;
static const int UPD_FDC_ST1_NOT_WRITEABLE=0x02;
static const int UPD_FDC_ST1_NO_DATA     = 0x04;

static const int UPD_FDC_ST1_OVERRUN     = 0x10;
static const int UPD_FDC_ST1_CRC_ERROR   = 0x20;
static const int UPD_FDC_ST1_EOF_CYLINDER= 0x80;

static const int UPD_FDC_ST2_MISSING_DM  = 0x01;     /* missing data mark */
static const int UPD_FDC_ST2_BAD_CYLINDER= 0x02;     /* bad cylinder number */
static const int UPD_FDC_ST2_SCAN_NOT_SAT= 0x04;     /* seek not satisfied */
static const int UPD_FDC_ST2_SCAN_HIT    = 0x08;     /* seek equal match */
static const int UPD_FDC_ST2_WRONG_CYLINDER=0x10;
static const int UPD_FDC_ST2_DATA_ERROR  = 0x20;     /* CRC error in data field */
static const int UPD_FDC_ST2_CONTROL_MARK= 0x40;

/* static const int UPD_FDC_ST3_TWO_SIDE    = 0x08; */
static const int UPD_FDC_ST3_TR00        = 0x10;
static const int UPD_FDC_ST3_READY       = 0x20;
static const int UPD_FDC_ST3_WRPROT      = 0x40;

static upd_cmd_t cmd[] = {/*    mask  value  cmd / res length */
  { UPD_CMD_READ_DATA,		0x1f, 0x06, 0x08, 0x07 },
  { UPD_CMD_READ_DATA,		0x1f, 0x0c, 0x08, 0x07 },	/* deleted data */
  { UPD_CMD_READ_DIAG,		0x9f, 0x02, 0x08, 0x07 },
  { UPD_CMD_RECALIBRATE,	0xff, 0x07, 0x01, 0x00 },
  { UPD_CMD_SEEK,		0xff, 0x0f, 0x02, 0x00 },
  { UPD_CMD_WRITE_DATA,		0x3f, 0x05, 0x08, 0x07 },
  { UPD_CMD_WRITE_DATA,		0x3f, 0x09, 0x08, 0x07 },	/* deleted data */
  { UPD_CMD_WRITE_ID,		0xbf, 0x0d, 0x05, 0x07 },
  { UPD_CMD_SCAN,		0x1f, 0x11, 0x08, 0x07 },
  { UPD_CMD_SCAN,		0x1f, 0x19, 0x08, 0x07 },	/* low or equal */
  { UPD_CMD_SCAN,		0x1f, 0x1d, 0x08, 0x07 },	/* high or equal */
  { UPD_CMD_READ_ID,		0xbf, 0x0a, 0x01, 0x07 },
  { UPD_CMD_SENSE_INT,		0xff, 0x08, 0x00, 0x02 },
  { UPD_CMD_SPECIFY,		0xff, 0x03, 0x02, 0x00 },
  { UPD_CMD_SENSE_DRIVE,	0xff, 0x04, 0x01, 0x01 },
  { UPD_CMD_VERSION,		0x1f, 0x10, 0x00, 0x01 },
  { UPD_CMD_INVALID,		0x00, 0x00, 0x00, 0x01 },
};

static int fdc_event, head_event, timeout_event;

static void
upd_fdc_event( libspectrum_dword last_tstates, int event, void *user_data );

void
upd_fdc_init_events( void )
{
  fdc_event = event_register( upd_fdc_event, "UPD FDC event" );
  head_event = event_register( upd_fdc_event, "UPD FDC head (un)load" );
  timeout_event = event_register( upd_fdc_event, "UPD FDC timeout" );
}

static void
cmd_identify( upd_fdc *f )
{
  upd_cmd_t *r = cmd;

  while( r->id != UPD_CMD_INVALID ) {
    if( ( f->command_register & r->mask ) == r->value )
      break;
    r++;
  }
  f->mt = f->command_register >> 7;		/* Multi track READ/WRITE */
  f->mf = ( f->command_register >> 6 ) & 0x01;	/* MFM format */
  f->sk = ( f->command_register >> 5 ) & 0x01;	/* Skip DELETED/NONDELETED sectors */
  f->cmd = r;

  return;
}

void
upd_fdc_master_reset( upd_fdc *f )
{
  int i;

  f->current_drive = f->drive[0];

  /* Caution with mirrored drives! The plus3 only use the US0 pin to select
     drives, so drive 2 := drive 0 and drive 3 := drive 1 */
  for( i = 0; i < 4; i++ )
    if( f->drive[i] != NULL )
      fdd_select( f->drive[i], f->drive[i] == f->current_drive ? 1 : 0 );

  f->main_status = UPD_FDC_MAIN_DATAREQ;
  for( i = 0; i < 4; i++ )
    f->status_register[i] = f->pcn[i] = f->seek[i] = f->seek_age[i] = 0;
  f->stp_rate = 16;
  f->hut_time = 240;
  f->hld_time = 254;
  f->non_dma = 1;
  f->direction = 0;
  f->head_load = 0;
  f->intrq = UPD_INTRQ_NONE;
  f->state = UPD_FDC_STATE_CMD;
  f->cycle = 0;
  f->last_sector_read = 0;
  f->read_id = 0;
  /* preserve disabled state of speedlock_hack */
  if( f->speedlock != -1 ) f->speedlock = 0;
}

static void
crc_preset( upd_fdc *f )
{
  f->crc = 0xffff;
}

static void
crc_add( upd_fdc *f, fdd_t *d )
{
  f->crc = crc_fdc( f->crc, d->data & 0xff );
}

/* 
   Read next ID into f->id_*
   return 0 if found an ID 
   return 1 if found but with CRC error
   return 2 if not found ID
*/
static int
read_id( upd_fdc *f )
{
  int i;
  fdd_t *d = f->current_drive;

  f->status_register[1] &= ~( UPD_FDC_ST1_CRC_ERROR | 
    			      UPD_FDC_ST1_MISSING_AM |
    			      UPD_FDC_ST1_NO_DATA );
  f->id_mark = UPD_FDC_AM_NONE;
  i = f->rev;
  while( i == f->rev && d->ready ) {
    fdd_read_data( d ); if( d->index ) f->rev--;
    crc_preset( f );
    if( f->mf ) {	/* double density (MFM) */
      if( d->data == 0xffa1 ) {
        crc_add( f, d );
	fdd_read_data( d ); crc_add( f, d );
	if( d->index ) f->rev--;
	if( d->data != 0xffa1 )
	  continue;
	fdd_read_data( d ); crc_add( f, d );
	if( d->index ) f->rev--;
	if( d->data != 0xffa1 )
	  continue;
      } else {		/* no 0xa1 with missing clock... */
	continue;
      }
    }
    fdd_read_data( d );
    if( d->index ) f->rev--;
    if( f->mf ) {	/* double density (MFM) */
      if( d->data != 0x00fe )
	continue;
    } else {		/* single density (FM) */
      if( d->data != 0xfffe )
	continue;
    }
    crc_add( f, d );
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;
    f->id_track = d->data;
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;
    f->id_head = d->data;
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;
    f->id_sector = d->data;
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;
    f->id_length = d->data > MAX_SIZE_CODE ? MAX_SIZE_CODE : d->data;
    f->sector_length = 0x80 << f->id_length; 
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;
    fdd_read_data( d ); crc_add( f, d );
    if( d->index ) f->rev--;

    if( f->crc != 0x0000 ) {
      f->status_register[1] |= UPD_FDC_ST1_CRC_ERROR | UPD_FDC_ST1_NO_DATA;
      f->id_mark = UPD_FDC_AM_ID;
      return 1;		/* found but CRC error */
    } else {
      f->id_mark = UPD_FDC_AM_ID;
      return 0;		/* found and OK */
    }
  }
  if(!d->ready) f->rev = 0;
  f->status_register[1] |= UPD_FDC_ST1_MISSING_AM | UPD_FDC_ST1_NO_DATA;	/*FIXME _NO_DATA? */
  return 2;		/* not found */
}

/*
   READ DAM (Data Address Mark)
   0 - found
   1 - not found
*/

static int
read_datamark( upd_fdc *f )
{
  fdd_t *d = f->current_drive;
  int i;

  if( f->mf ) {	/* double density (MFM) */
    for( i = 40; i > 0; i-- ) {
      fdd_read_data( d );
      if( d->data == 0x4e )		/* read next */
	continue;

      if( d->data == 0x00 )		/* go to PLL sync */
	break;

      f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
      return 1;				/* something wrong... */
    } 
    for( ; i > 0; i-- ) {
      crc_preset( f );
      fdd_read_data( d ); crc_add( f, d );
      if( d->data == 0x00 )
	continue;

      if( d->data == 0xffa1 )	/* got to a1 mark */
	break;

      f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
      return 1;
    }
    for( i = d->data == 0xffa1 ? 2 : 3; i > 0; i-- ) {
      fdd_read_data( d ); crc_add( f, d );
      if( d->data != 0xffa1 ) {
        f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
	return 1;
      }
    } 
    fdd_read_data( d ); crc_add( f, d );
    if( d->data < 0x00f8 || d->data > 0x00fb ) { /* !fb deleted mark */
      f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
      return 1;
    }
    if( d->data != 0x00fb )
      f->ddam = 1;
    else
      f->ddam = 0;
    return 0;
  } else {		/* SD -> FM */
    for( i = 30; i > 0; i-- ) {
      fdd_read_data( d );
      if( d->data == 0xff )		/* read next */
	continue;

      if( d->data == 0x00 )		/* go to PLL sync */
	break;

      f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
      return 1;				/* something wrong... */
    } 
    for( ; i > 0; i-- ) {
      crc_preset( f );
      fdd_read_data( d ); crc_add( f, d );
      if( d->data == 0x00 )
	continue;

      if( d->data >= 0xfff8 && d->data <= 0xfffb )	/* !fb deleted mark */
	break;

      f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
      return 1;
    }
    if( i == 0 ) {
      fdd_read_data( d ); crc_add( f, d );
      if( d->data < 0xfff8 || d->data > 0xfffb ) {	/* !fb deleted mark */
        f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
	return 1;
      }
    }
    if( d->data != 0x00fb )
      f->ddam = 1;
    else
      f->ddam = 0;
    return 0;
  }
  f->status_register[2] |= UPD_FDC_ST2_MISSING_DM;
  return 1;
}

/*

 seek to a specified id

 0 - found
 1 - id with CRC error
 2 - not found any id
 3 - not found the specified id
*/

static int
seek_id( upd_fdc *f )
{
  int r;

  f->status_register[2] &= ~( UPD_FDC_ST2_WRONG_CYLINDER |
			      UPD_FDC_ST2_BAD_CYLINDER );
  r = read_id( f );
  if( r != 0 ) return r;		/* not found any good id */

  if( f->id_track != f->data_register[1] ) {
    f->status_register[2] |= UPD_FDC_ST2_WRONG_CYLINDER;
    if( f->id_track == 0xff )
      f->status_register[2] |= UPD_FDC_ST2_BAD_CYLINDER;
    return 3;
  }

  if( f->id_sector == f->data_register[3] && 
      f->id_head == f->data_register[2] ) {

    if( f->id_length != f->data_register[4] ) {
      f->status_register[1] |= UPD_FDC_ST1_NO_DATA;
      return 3;
    }

    return 0;
  }
  f->status_register[1] |= UPD_FDC_ST1_NO_DATA;
  return 3;
}

upd_fdc *
upd_fdc_alloc_fdc( upd_type_t type, upd_clock_t clock )
{
  int i;
  
  upd_fdc *f = libspectrum_new( upd_fdc, 1 );

  f->type = type != UPD765B ? UPD765A : UPD765B;
  f->clock = clock != UPD_CLOCK_4MHZ ? UPD_CLOCK_8MHZ : UPD_CLOCK_4MHZ;
  for( i = 0; i < 4; i++ )
    f->drive[i] = NULL;
  f->current_drive = NULL;
  f->speedlock = 0;
  upd_fdc_master_reset( f );
  return f;
}

static void
cmd_result( upd_fdc *f )
{
  f->cycle = f->cmd->res_length;
  f->main_status &= ~UPD_FDC_MAIN_EXECUTION;
  f->main_status |= UPD_FDC_MAIN_DATAREQ;
  if( f->cycle > 0 ) {	/* result state */
    f->state = UPD_FDC_STATE_RES;
    f->intrq = UPD_INTRQ_RESULT;
    f->main_status |= UPD_FDC_MAIN_DATA_READ;
  } else {			/* NO result state */
    f->state = UPD_FDC_STATE_CMD;
    f->main_status &= ~UPD_FDC_MAIN_DATADIR;
    f->main_status &= ~UPD_FDC_MAIN_BUSY;
  }
  event_remove_type( timeout_event );		/* remove timeouts... */
  if( f->head_load && f->cmd->id <= UPD_CMD_READ_ID ) {
    event_add_with_data( tstates + f->hut_time * 
			 machine_current->timings.processor_speed / 1000,
			 head_event, f );
  }
}

static void
seek_step( upd_fdc *f, int start )
{
  int i, j;
  fdd_t *d;

  if( start ) {
    i = f->us;

    /* Drive already in seek state? */
    if( f->main_status & ( 1 << i ) ) return;   	

    /* Mark seek mode for fdd. It will be cleared by Sense Interrupt command */
    f->main_status |= 1 << i;
  } else {

    /* Get drive in seek state that has completed the positioning */
    i=0;
    for( j = 1; j < 4; j++) {
      if( f->seek_age[j] > f->seek_age[i] )
        i = j;
    }

    if( f->seek[i] == 0 || f->seek[i] >= 4 ) {
      return;
    }
  }

  d = f->drive[i];

  /* There is need to seek? */
  if( f->pcn[i] == f->ncn[i] &&
      f->seek[i] == 2 && !d->tr00 ) {	/* recalibrate fail */
    f->seek[i] = 5;				/* abnormal termination */
    f->seek_age[i] = 0;
    f->intrq = UPD_INTRQ_SEEK;
    f->status_register[0] |= UPD_FDC_ST0_EQUIP_CHECK;
    f->main_status &= ~( 1 << i );
    return;
  }

  /* There is need to seek? */
  if( f->pcn[i] == f->ncn[i] || 
    ( f->seek[i] == 2 && d->tr00 ) ) {	/* correct position */
    if( f->seek[i] == 2 )			/* recalibrate */
      f->pcn[i] = 0; 
    f->seek[i] = 4;				/* normal termination */
    f->seek_age[i] = 0;
    f->intrq = UPD_INTRQ_SEEK;
    f->main_status &= ~( 1 << i );
    return;
  }

  /* Drive not ready */
  if( !d->ready ) {
    if( f->seek[i] == 2 )			/* recalibrate */
      f->pcn[i] = f->rec[i] - ( 77 - f->pcn[i] ); 	/* restore PCN */
    f->seek[i] = 6;				/* drive not ready termination */
    f->seek_age[i] = 0;
    f->intrq = UPD_INTRQ_READY;		/* doesn't matter */
    f->main_status &= ~( 1 << i );
    return;
  }

  /* Send step */
  if( f->pcn[i] != f->ncn[i] ) {	/**FIXME if d->tr00 == 1 ??? */
    fdd_step( d, f->pcn[i] > f->ncn[i] ? 
                        FDD_STEP_OUT : FDD_STEP_IN );
    f->pcn[i] += f->pcn[i] > f->ncn[i] ? -1 : 1;

    /* Update age for active seek operations */
    for( j = 0; j < 4; j++) {
      if( f->seek_age[j] > 0 ) f->seek_age[j]++;
    }
    f->seek_age[i] = 1;

    /* wait step completion */
    event_add_with_data( tstates + f->stp_rate * 
                         machine_current->timings.processor_speed / 1000,
                         fdc_event, f );
  }

  return;
}

static void
start_read_id( upd_fdc *f )
{
  int i;

  if( !f->read_id ) {
    f->rev = 2;
    f->read_id = 1;
  }
  if( f->rev ) {
    i = f->current_drive->disk.i >= f->current_drive->disk.bpt ?
    	0 : f->current_drive->disk.i;			/* start position */
    if( read_id( f ) != 2 )
      f->rev = 0;
    i = f->current_drive->disk.bpt ? 
      ( f->current_drive->disk.i - i ) * 200 / f->current_drive->disk.bpt : 200;
    if( i > 0 ) {
      event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			 machine_current->timings.processor_speed / 1000,
			 fdc_event, f );
      return;
    }
  }
  f->read_id = 0;
  if( f->id_mark != UPD_FDC_AM_NONE ) {	/* found */
    f->data_register[1] = f->id_track;
    f->data_register[2] = f->id_head;
    f->data_register[3] = f->id_sector;
    f->data_register[4] = f->id_length;
  }
  if( f->id_mark != UPD_FDC_AM_ID || 
      f->status_register[1] & UPD_FDC_ST1_CRC_ERROR ) {	/* not found/crc error id mark */
    f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
  }
  f->intrq = UPD_INTRQ_RESULT;
  cmd_result( f );			/* set up result phase */
}

static void
start_read_diag( upd_fdc *f )
{
  int i;
  
  if( !f->read_id ) {
    f->rev = 2;
    f->read_id = 1;
  }
  if( f->rev ) {
    i = f->current_drive->disk.i >= f->current_drive->disk.bpt ?
    	0 : f->current_drive->disk.i;			/* start position */
    if( read_id( f ) != 2 )
      f->rev = 0;
    i = f->current_drive->disk.bpt ? 
      ( f->current_drive->disk.i - i ) * 200 / f->current_drive->disk.bpt : 200;
    if( i > 0 ) {
      event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			 machine_current->timings.processor_speed / 1000,
			 fdc_event, f );
      return;
    }
  }
  f->read_id = 0;
  if( f->id_mark == UPD_FDC_AM_NONE ) {
    f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
    f->status_register[1] |= UPD_FDC_ST1_EOF_CYLINDER;
    goto abort_read_diag;
  }
  if( f->id_track != f->data_register[1] || f->id_sector != f->data_register[3] || 
    f->data_register[2] != f->id_head ) {
    f->status_register[1] |= UPD_FDC_ST1_NO_DATA;
  }

  if( f->id_track != f->data_register[1] ) {		/*FIXME UPD765 set it always? */
    f->status_register[2] |= UPD_FDC_ST2_WRONG_CYLINDER;
    if( f->id_track == 0xff )
      f->status_register[2] |= UPD_FDC_ST2_BAD_CYLINDER;
  }

  if( read_datamark( f ) > 0 ) {	/* not found */
    f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
    goto abort_read_diag;
  }

  f->main_status |= UPD_FDC_MAIN_DATAREQ | UPD_FDC_MAIN_DATA_READ;
  f->data_offset = 0;
  event_remove_type( timeout_event );
  event_add_with_data( tstates + 4 *			/* 2 revolution: 2 * 200 / 1000  */
		       machine_current->timings.processor_speed / 10,
		       timeout_event, f );
  return;

abort_read_diag:
    f->state = UPD_FDC_STATE_RES;	/* end of execution phase */
    f->cycle = f->cmd->res_length;
    f->main_status &= ~UPD_FDC_MAIN_EXECUTION;
    f->intrq = UPD_INTRQ_RESULT;
    cmd_result( f );
}

static void
start_read_data( upd_fdc *f )
{
  int i;
skip_deleted_sector:
multi_track_next:
  if( f->first_rw || f->read_id || 
      f->data_register[5] > f->data_register[3] ) {
    if( !f->read_id ) {
      if( !f->first_rw )
        f->data_register[3]++;
      f->first_rw = 0;

      f->rev = 2;
      f->read_id = 1;
    }
    while( f->rev ) {
      i = f->current_drive->disk.i >= f->current_drive->disk.bpt ?
    	  0 : f->current_drive->disk.i;			/* start position */
      if( seek_id( f ) == 0 )
        f->rev = 0;
      else
        f->id_mark = UPD_FDC_AM_NONE;
      i = f->current_drive->disk.bpt ? 
          ( f->current_drive->disk.i - i ) * 200 / f->current_drive->disk.bpt : 200;
      if( i > 0 ) {
        event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			     machine_current->timings.processor_speed / 1000,
			     fdc_event, f );
        return;
      }
    }

    f->read_id = 0;
    if( f->id_mark == UPD_FDC_AM_NONE ) {	/* not found/crc error */
      f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
      goto abort_read_data;
    }

    if( read_datamark( f ) > 0 ) {	/* not found */
      f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
      goto abort_read_data;
    }
    if( f->ddam != f->del_data ) {
      f->status_register[2] |= UPD_FDC_ST2_CONTROL_MARK;
      if( f->sk ) {
	f->data_register[3]++;
    	goto skip_deleted_sector;		/* or not deleted but we want to read deleted */
      }
    }
  } else {
    if( f->mt ) {
      f->data_register[1]++;		/* next track */
      f->data_register[3] = 1;		/* first sector */
      goto multi_track_next;
    }
abort_read_data:
    f->state = UPD_FDC_STATE_RES;	/* end of execution phase */
    f->cycle = f->cmd->res_length;
/* end of cylinder is set if:
* 1. sector data is read completely
*    (i.e. no other errors occur like no data.
* 2. sector being read is same specified by EOT
* 3. terminal count is not received
* note: in +3 uPD765 never got TC
*/
    if( !f->status_register[0] && !f->status_register[1] ) {
      f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
      f->status_register[1] |= UPD_FDC_ST1_EOF_CYLINDER;
    }
    
    if( !( f->status_register[0] & 
	    ( UPD_FDC_ST0_INT_ABNORM | UPD_FDC_ST0_INT_READY ) ) ) {
      f->data_register[1]++;		/* next track */
      f->data_register[3] = 1;		/* first sector */
    }
    
    f->main_status &= ~UPD_FDC_MAIN_EXECUTION;
    f->intrq = UPD_INTRQ_RESULT;
    cmd_result( f );
    return;
  }
  f->main_status |= UPD_FDC_MAIN_DATAREQ;
  if( f->cmd->id != UPD_CMD_SCAN )
    f->main_status |= UPD_FDC_MAIN_DATA_READ;
  f->data_offset = 0;
  event_remove_type( timeout_event );
  event_add_with_data( tstates + 4 *			/* 2 revolution: 2 * 200 / 1000  */
		       machine_current->timings.processor_speed / 10,
		       timeout_event, f );
}

static void
start_write_data( upd_fdc *f )
{
  int i;
  fdd_t *d = f->current_drive;

multi_track_next:
  if( f->first_rw || f->read_id ||
      f->data_register[5] > f->data_register[3] ) {
    if( !f->read_id ) {
      if( !f->first_rw )
        f->data_register[3]++;
      f->first_rw = 0;

      f->rev = 2;
      f->read_id = 1;
    }
    while( f->rev ) {
      i = f->current_drive->disk.i >= f->current_drive->disk.bpt ?
    	  0 : f->current_drive->disk.i;			/* start position */
      if( seek_id( f ) == 0 )
        f->rev = 0;
      else
        f->id_mark = UPD_FDC_AM_NONE;
      i = f->current_drive->disk.bpt ? 
          ( f->current_drive->disk.i - i ) * 200 / f->current_drive->disk.bpt : 200;
      if( i > 0 ) {
        event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			     machine_current->timings.processor_speed / 1000,
			     fdc_event, f );
        return;
      }
    }

    f->read_id = 0;
    if( f->id_mark == UPD_FDC_AM_NONE ) {	/* not found/crc error */
      f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
      goto abort_write_data;
    }

    for( i = 11; i > 0; i-- )	/* "delay" 11 GAP byte */
      fdd_read_data( d );
    if( f->mf )					/* MFM */
      for( i = 11; i > 0; i-- )	/* "delay" another 11 GAP byte */
	fdd_read_data( d );

    d->data = 0x00;
    for( i = f->mf ? 12 : 6; i > 0; i-- )	/* write 6/12 zero */
      fdd_write_data( d );
    crc_preset( f );
    if( f->mf ) {				/* MFM */
      d->data = 0xffa1;
      for( i = 3; i > 0; i-- ) {		/* write 3 0xa1 with clock mark */
	fdd_write_data( d ); crc_add( f, d );
      }
    }
    d->data = ( f->del_data ? 0x00f8 : 0x00fb ) |
    			( f->mf ? 0x0000 : 0xff00 );	/* write data mark */
    fdd_write_data( d ); crc_add( f, d );
  } else {
    f->data_register[1]++;		/* next track */
    f->data_register[3] = 1;		/* first sector */
    if( f->mt ) {
      goto multi_track_next;
    }
abort_write_data:
    f->state = UPD_FDC_STATE_RES;	/* end of execution phase */
    f->cycle = f->cmd->res_length;
/* end of cylinder is set if:
* 1. sector data is read completely
*    (i.e. no other errors occur like no data.
* 2. sector being read is same specified by EOT
* 3. terminal count is not received
* note: in +3 uPD765 never got TC
*/
    f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
    f->status_register[1] |= UPD_FDC_ST1_EOF_CYLINDER;
    f->main_status &= ~UPD_FDC_MAIN_EXECUTION;
    f->intrq = UPD_INTRQ_RESULT;
    cmd_result( f );
    return;
  }
  f->main_status |= UPD_FDC_MAIN_DATAREQ | UPD_FDC_MAIN_DATA_WRITE;
  f->data_offset = 0;
  event_remove_type( timeout_event );
  event_add_with_data( tstates + 4 *			/* 2 revolution: 2 * 200 / 1000 */
		       machine_current->timings.processor_speed / 10,
		       timeout_event, f );
}

static void
start_write_id( upd_fdc *f )
{
  int i;
  fdd_t *d = f->current_drive;

  d->data = f->mf ? 0x4e : 0xff;
  for( i = 40; i > 0; i-- )	/* write 40 GAP byte */
    fdd_write_data( d );
  if( f->mf )					/* MFM */
    for( i = 40; i > 0; i-- )	/* write another 40 GAP byte */
      fdd_write_data( d );
  d->data = 0x00;
  for( i = f->mf ? 12 : 6; i > 0; i-- )	/* write 6/12 zero */
    fdd_write_data( d );
  crc_preset( f );
  if( f->mf ) {				/* MFM */
    d->data = 0xffc2;
    for( i = 3; i > 0; i-- ) {		/* write 3 0xc2 with clock mark */
      fdd_write_data( d );
    }
  }
  d->data = 0x00fc | ( f->mf ? 0x0000 : 0xff00 );	/* write index mark */
  fdd_write_data( d );

  d->data = f->mf ? 0x4e : 0xff;	/* postindex GAP */
  for( i = 26; i > 0; i-- )	/* write 26 GAP byte */
    fdd_write_data( d );
  if( f->mf )					/* MFM */
    for( i = 24; i > 0; i-- )	/* write another 24 GAP byte */
      fdd_write_data( d );

  f->main_status |= UPD_FDC_MAIN_DATAREQ | UPD_FDC_MAIN_DATA_WRITE;
  f->data_offset = 0;
  event_add_with_data( tstates + 2 *			/* 1/10 revolution: 1 * 200 / 1000 */
		       machine_current->timings.processor_speed / 100,
		       timeout_event, f );
}

static void
head_load( upd_fdc *f )
{
  event_remove_type( head_event );
  if( f->head_load ) {		/* head already loaded */
    if( f->cmd->id == UPD_CMD_READ_DATA || f->cmd->id == UPD_CMD_SCAN )
      start_read_data( f );
    else if( f->cmd->id == UPD_CMD_READ_ID )
      start_read_id( f );
    else if( f->cmd->id == UPD_CMD_READ_DIAG ) {
      fdd_wait_index_hole( f->current_drive );		/* start reading from index hole */
      start_read_diag( f );
    } else if( f->cmd->id == UPD_CMD_WRITE_DATA )
      start_write_data( f );
    else if( f->cmd->id == UPD_CMD_WRITE_ID ) {
      fdd_wait_index_hole( f->current_drive );		/* start writing from index hole */
      start_write_id( f );
    }
  } else {
    fdd_head_load( f->current_drive, 1 );
    f->head_load = 1;
    event_add_with_data( tstates + f->hld_time * 
			 machine_current->timings.processor_speed / 1000,
			 fdc_event, f );
  }
}

static void
upd_fdc_event( libspectrum_dword last_tstates GCC_UNUSED, int event,
	       void *user_data ) 
{
  upd_fdc *f = user_data;

  if( event == timeout_event ) {
    f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
    f->status_register[1] |= UPD_FDC_ST1_OVERRUN;
    cmd_result( f );
    return;
  }

  if( event == head_event ) {
    fdd_head_load( f->current_drive, 0 );
    f->head_load = 0;
    return;
  }
  
  if( f->read_id ) {
    if( f->cmd->id == UPD_CMD_READ_DATA ) {
      start_read_data( f );
    } else if( f->cmd->id == UPD_CMD_READ_ID ) {
      start_read_id( f );
    } else if( f->cmd->id == UPD_CMD_READ_DIAG ) {
      start_read_diag( f );
    } else if( f->cmd->id == UPD_CMD_WRITE_DATA ) {
      start_write_data( f );
    }
  } else if( f->main_status & 0x03 ) {		/* seek/recalibrate active */
    seek_step( f, 0 );
  } else if( f->cmd->id == UPD_CMD_READ_DATA || f->cmd->id == UPD_CMD_SCAN ) {
    start_read_data( f );
  } else if( f->cmd->id == UPD_CMD_READ_ID ) {
    start_read_id( f );
  } else if( f->cmd->id == UPD_CMD_READ_DIAG ) {
    fdd_wait_index_hole( f->current_drive );		/* start reading from index hole */
    start_read_diag( f );
  } else if( f->cmd->id == UPD_CMD_WRITE_DATA ) {
    start_write_data( f );
  } else if( f->cmd->id == UPD_CMD_WRITE_ID ) {
    fdd_wait_index_hole( f->current_drive );		/* start writing from index hole */
    start_write_id( f );
  }

  return;
}

libspectrum_byte
upd_fdc_read_status( upd_fdc *f )
{
  return f->main_status;
}

libspectrum_byte
upd_fdc_read_data( upd_fdc *f )
{
  libspectrum_byte r;

  fdd_t *d = f->current_drive;

  if( !( f->main_status & UPD_FDC_MAIN_DATAREQ ) || 
      !( f->main_status & UPD_FDC_MAIN_DATA_READ ) )
    return 0xff;

  if( f->state == UPD_FDC_STATE_EXE ) {		/* READ_DATA/READ_DIAG */
    f->data_offset++;				/* count read bytes */
    fdd_read_data( d ); crc_add( f, d );	/* read a byte */

    /* Speedlock hack */
    if( f->speedlock > 0 && !d->do_read_weak ) {	/* do not conflict with fdd weak reads */
      if( f->data_offset < 64 && d->data != 0xe5 )
        f->speedlock = 2;		/* W.E.C Le Mans type ... */
      else if( ( f->speedlock > 1 || f->data_offset < 64 ) &&
	       !( f->data_offset % 29 ) ) {
	d->data ^= f->data_offset;	/* mess up data */
	crc_add( f, d );			/* mess up crc */
      }
    }
    /* EOSpeedlock hack */

    r = d->data & 0xff;
    if( f->data_offset == f->rlen ) {	/* send only rlen byte to host */
      while( f->data_offset < f->sector_length ) {
	fdd_read_data( d ); crc_add( f, d );
	f->data_offset++;
      }
    }
    if( ( f->cmd->id == UPD_CMD_READ_DIAG || f->cmd->id == UPD_CMD_READ_DATA )
        && f->data_offset == f->sector_length ) {       /* read the CRC */
      fdd_read_data( d ); crc_add( f, d );
      fdd_read_data( d ); crc_add( f, d );
      if( f->crc != 0x000 ) {
	f->status_register[2] |= UPD_FDC_ST2_DATA_ERROR;
	f->status_register[1] |= UPD_FDC_ST1_CRC_ERROR;
	if( f->cmd->id == UPD_CMD_READ_DATA ) {		/* READ DIAG not aborted! */
	  f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
	  cmd_result( f );			/* set up result phase */
	  return r;
        }
      }
	
      if( f->cmd->id == UPD_CMD_READ_DATA ) {
	if( f->ddam != f->del_data ) {	/* we read a not 'wanted' sector... so */
	  if( f->data_register[5] > f->data_register[3] )	/* if we want to read more... */
            f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
          cmd_result( f );			/* set up result phase */
    	  return r;
	}
	f->rev = 2;
        f->main_status &= ~UPD_FDC_MAIN_DATAREQ;
	start_read_data( f );
      } else {				/* READ DIAG */
	f->data_register[3]++;		/*FIXME ??? */
	f->data_register[5]--;
        if( f->data_register[5] == 0 ) {	/* no more */
          cmd_result( f );			/* set up result phase */
          return r;
        }
        f->main_status &= ~UPD_FDC_MAIN_DATAREQ;
        start_read_diag( f );
      }
    }
    return r;
  }

  if( f->state != UPD_FDC_STATE_RES )
    return 0xff;

  if( f->cmd->id == UPD_CMD_SENSE_DRIVE ) {	/* 1 */
    r = f->status_register[3];
  } else if( f->cmd->id == UPD_CMD_SENSE_INT ) { /* 2 */
    r = f->sense_int_res[f->cmd->res_length - f->cycle];
  } else if( f->cmd->res_length - f->cycle < 3 ) {
    r = f->status_register[f->cmd->res_length - f->cycle];
  } else {
    r = f->data_register[f->cmd->res_length - f->cycle - 2];
  }
  f->cycle--;
  if( f->cycle == 0 ) {
    f->state = UPD_FDC_STATE_CMD;
    f->main_status |= UPD_FDC_MAIN_DATAREQ;
    f->main_status &= ~UPD_FDC_MAIN_DATADIR;
    f->main_status &= ~UPD_FDC_MAIN_BUSY;
    if( f->intrq < UPD_INTRQ_READY )
      f->intrq = UPD_INTRQ_NONE;
  }
  return r;
}

void
upd_fdc_write_data( upd_fdc *f, libspectrum_byte data )
{
  int i, terminated = 0;
  unsigned int u;
  fdd_t *d;

  if( !( f->main_status & UPD_FDC_MAIN_DATAREQ ) || 
      ( f->main_status & UPD_FDC_MAIN_DATA_READ ) )
    return;

  if( f->main_status & UPD_FDC_MAIN_BUSY && 
      f->state == UPD_FDC_STATE_EXE ) {	/* execution phase WRITE/FORMAT */
    d = f->current_drive;
    if( f->cmd->id == UPD_CMD_WRITE_ID ) {	/* FORMAT */
		/* at the index hole... */
      f->data_register[f->data_offset + 5] = data;	/* read id fields */
      f->data_offset++;
      if( f->data_offset == 4 ) {			/* C, H, R, N done => format track */
	event_remove_type( timeout_event );

        d->data = 0x00;
        for( i = f->mf ? 12 : 6; i > 0; i-- )	/* write 6/12 zero */
          fdd_write_data( d );
        crc_preset( f );
        if( f->mf ) {				/* MFM */
          d->data = 0xffa1;
          for( i = 3; i > 0; i-- ) {		/* write 3 0xa1 with clock mark */
	    fdd_write_data( d ); crc_add( f, d );
          }
        }
        d->data = 0x00fe | ( f->mf ? 0x0000 : 0xff00 );	/* write id mark */
        fdd_write_data( d ); crc_add( f, d );
	for( i = 0; i < 4; i++ ) {
    	  d->data =  f->data_register[i + 5];	/* write id fields */
          fdd_write_data( d ); crc_add( f, d );
	}
        d->data = f->crc >> 8;
        fdd_write_data( d );			/* write crc1 */
        d->data = f->crc & 0xff;
        fdd_write_data( d );			/* write crc2 */

        d->data = f->mf ? 0x4e : 0xff;
        for( i = 11; i > 0; i-- )	/* write 11 GAP byte */
          fdd_write_data( d );
        if( f->mf )					/* MFM */
          for( i = 11; i > 0; i-- )	/* write another 11 GAP byte */
	    fdd_write_data( d );
        d->data = 0x00;
        for( i = f->mf ? 12 : 6; i > 0; i-- )	/* write 6/12 zero */
          fdd_write_data( d );
        crc_preset( f );
        if( f->mf ) {				/* MFM */
          d->data = 0xffa1;
          for( i = 3; i > 0; i-- ) {		/* write 3 0xa1 with clock mark */
	    fdd_write_data( d ); crc_add( f, d );
          }
        }
        d->data = 0x00fb | ( f->mf ? 0x0000 : 0xff00 );	/* write data mark */
        fdd_write_data( d ); crc_add( f, d );
	
    	d->data =  f->data_register[4];	/* write filler byte */
	for( i = f->rlen; i > 0; i-- ) {
          fdd_write_data( d ); crc_add( f, d );
	}
        d->data = f->crc >> 8;
        fdd_write_data( d );			/* write crc1 */
        d->data = f->crc & 0xff;
        fdd_write_data( d );			/* write crc2 */

        d->data = f->mf ? 0x4e : 0xff;
	for( i = f->data_register[3]; i > 0; i-- ) {	/* GAP */
          fdd_write_data( d );
	}
        f->data_offset = 0;
	f->data_register[2]--;		/* prepare next sector */
      }
      if( f->data_register[2] == 0 ) {	/* finish all sector */

        d->data = f->mf ? 0x4e : 0xff;	/* GAP3 as Intel call this GAP */
        while( !d->index )
          fdd_write_data( d );


	f->state = UPD_FDC_STATE_RES;	/* end of execution phase */
        f->cycle = f->cmd->res_length;
        f->main_status &= ~UPD_FDC_MAIN_EXECUTION;
    	f->intrq = UPD_INTRQ_RESULT;
        cmd_result( f );
	return;
      }
      event_add_with_data( tstates + 2 *		/* 1/10 revolution: 1 * 200 / 1000 */
		       machine_current->timings.processor_speed / 100,
		       timeout_event, f );
      return;
    } else if( f->cmd->id == UPD_CMD_WRITE_DATA ) {		/* WRITE DATA */
      f->data_offset++;
      d->data = data;
      fdd_write_data( d ); crc_add( f, d );
    
      if( f->data_offset == f->rlen ) {	/* read only rlen byte from host */
        d->data = 0x00;
        while( f->data_offset < f->sector_length ) {	/* fill with 0x00 */
	  fdd_read_data( d ); crc_add( f, d );
	  f->data_offset++;
        }
      }
      if( f->data_offset == f->sector_length ) {	/* write the CRC */
        d->data = f->crc >> 8;
        fdd_write_data( d );			/* write crc1 */
        d->data = f->crc & 0xff;
        fdd_write_data( d );			/* write crc2 */
        f->main_status &= ~UPD_FDC_MAIN_DATAREQ;
	start_write_data( f );
      }
      return;
    } else {						/* SCAN */
      f->data_offset++;
      fdd_read_data( d ); crc_add( f, d );
      if( f->data_offset == 0 && d->data == data )	/* `scan hit' */
        f->status_register[2] |= UPD_FDC_ST2_SCAN_HIT;

      if( d->data != data )				/* `scan _not_ hit' */
        f->status_register[2] &= ~UPD_FDC_ST2_SCAN_HIT;

      if( ( f->scan == UPD_SCAN_EQ && d->data != data ) ||	/* scan not satisfied */
          ( f->scan == UPD_SCAN_LO && d->data > data ) ||
          ( f->scan == UPD_SCAN_HI && d->data < data ) ) {
        f->status_register[2] |= UPD_FDC_ST2_SCAN_NOT_SAT;
      }
    
      if( f->data_offset == f->sector_length ) {	/* read the CRC */
	fdd_read_data( d ); crc_add( f, d );
	fdd_read_data( d ); crc_add( f, d );
	if( f->crc != 0x000 ) {
	  f->status_register[2] |= UPD_FDC_ST2_DATA_ERROR;
	  f->status_register[1] |= UPD_FDC_ST1_CRC_ERROR;
	}
	
	f->data_register[3] += f->data_register[7];	/*FIXME what about STP>2 or STP<1 */
	if( f->ddam != f->del_data ) {			/* we read a not 'wanted' sector... so */
	  if( f->data_register[5] >= f->data_register[3] )	/* if we want to read more... */
            f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
    	  cmd_result( f );			/* set up result phase */
    	  return;
	}
        if( ( f->status_register[2] & UPD_FDC_ST2_SCAN_HIT ) ||
	    ( f->status_register[2] & UPD_FDC_ST2_SCAN_NOT_SAT ) == 0x00 ) {	/*FIXME sure? */
    	  cmd_result( f );			/* set up result phase */
    	  return;
	}
	f->rev = 2;
        f->main_status &= ~UPD_FDC_MAIN_DATAREQ;
	start_read_data( f );
      }
      return;
    }
  }
/*----------- Command Phase ---------------*/
  if( f->cycle == 0 ) {			/* first byte -> command */
    f->command_register = data;
    cmd_identify( f );
    f->main_status |= UPD_FDC_MAIN_BUSY;
    /* ... A Sense Interrupt Status Command _must be sent after a Seek or
       Recalibrate interrupt_, otherwise the FDC will consider the next
       Command to be an invalid Command ... (i8272)
       Note: looks uPD765 should _NOT_, because The New Zealand Story
             does not work with this stuff
    
       ... If a SENSE INTERRUPT STATUS command is issued when no active
       interrupt condition is present, the status register ST0 will return
       a value of 80H (invalid command) ... (82078 44pin) */
    if( ( f->intrq == UPD_INTRQ_NONE && f->cmd->id == UPD_CMD_SENSE_INT )
    ) {
	/* this command will be INVALID */
      f->command_register = 0x00;
      cmd_identify( f );
    }
  } else {
    f->data_register[f->cycle - 1] = data;	/* store data register bytes */
  }
  if( f->cycle >= f->cmd->cmd_length ) { 	/* we already read all neccessery byte */
    f->state = UPD_FDC_STATE_EXE;		/* start execution of the command */
    f->main_status &= ~UPD_FDC_MAIN_DATAREQ;
    if( f->non_dma ) {				/* btw: only NON-DMA mode emulated */
      f->main_status |= UPD_FDC_MAIN_EXECUTION;
    }

    /* select current drive and head if needed */    
    if( f->cmd->id != UPD_CMD_SENSE_INT &&
	f->cmd->id != UPD_CMD_SPECIFY &&
	f->cmd->id != UPD_CMD_VERSION &&
	f->cmd->id != UPD_CMD_INVALID ) {
      f->us = f->data_register[0] & 0x03;
      if( f->current_drive != f->drive[ f->us ] ) {
        fdd_select( f->current_drive, 0 );
        f->current_drive = f->drive[ f->us ];
        fdd_select( f->current_drive, 1 );
      }

      f->hd = ( f->data_register[0] & 0x04 ) >> 2;
      fdd_set_head( f->current_drive, f->hd );

      /* identify READ_DELETED_DATA/WRITE_DELETED_DATA */
      if( f->cmd->id == UPD_CMD_READ_DATA || 
    	  f->cmd->id == UPD_CMD_WRITE_DATA ) {
        f->del_data = ( f->command_register & 0x08 ) >> 3;
        f->sk = ( f->data_register[0] & 0x20 ) >> 5;		/* SKIP deleted/non-deleted */
      }
    }
    /* ... During the Command Phase of the SEEK operation the FDC in the
       FDC BUSY state, but during the Execution Phase it is in the NON BUSY
       state. While the FDC is in the NON BUSY state, another Seek Command
       may be issued, and in this manner paralell seek operation may be done
       on up to 4 Drives at once ... 
       ... The ability to overlap RECALIBRATE Commands to Multiple FDDs, and
       the loss of the READY signal, as described in the SEEK Command, also
       applies to the RECALIBRATE Command ... (i8272)
    	 
       Note:
       For overlapped seeks, _only one step pulse per drive section is issued_.
       Non-overlapped seeks will issue all programmed step pulses.  (82078 44pin)
     */
    if( f->cmd->id == UPD_CMD_RECALIBRATE || 	
	f->cmd->id == UPD_CMD_SEEK ||
	f->cmd->id == UPD_CMD_SPECIFY ) {
      f->main_status &= ~UPD_FDC_MAIN_BUSY;		/* no result phase */
    }

    if( f->cmd->id < UPD_CMD_SENSE_INT ) {
      if( f->cmd->id < UPD_CMD_RECALIBRATE )			/* reset status registers */
        f->status_register[0] = f->status_register[1] = 
	  f->status_register[2] = 0x00;
      f->status_register[0] = f->us + ( f->hd << 2 );	/* set ST0 device/head */
    }

    d = f->current_drive;
    switch( f->cmd->id ) {
    case UPD_CMD_INVALID:
      f->status_register[0] = 0x80;
      break;
    case UPD_CMD_VERSION:
      f->status_register[0] = f->type == UPD765B ? 0x90 : 0x80;
      break;
    case UPD_CMD_SPECIFY:
      f->stp_rate = 0x10 - ( f->data_register[0] >> 4 );	/* ms */
      f->hut_time = ( f->data_register[0] & 0x0f ) << 4;	/* ms */
      if( f->hut_time == 0 ) f->hut_time = 128;
      f->hld_time = f->data_register[1] & 0xfe;		/* ms */
      if( f->hld_time == 0 ) f->hld_time = 256;
      f->non_dma = f->data_register[1] & 0x01;		/* non DMA mode */
      /* ... if the clock was reduced to 4MHz (mini-floppy application) then
         all time intervalls are increased by a factor of 2 ...*/
      if( f->clock == UPD_CLOCK_4MHZ ) {			
        f->stp_rate *= 2;
	f->hut_time *= 2;
	f->hld_time *= 2;
      }
      f->state = UPD_FDC_STATE_CMD;			/* no result phase... */
      break;
    case UPD_CMD_SENSE_DRIVE:
      f->status_register[3] = f->us + ( f->hd << 2 );
      /*???? the plus3 wiring cause that the double side signal is the
         same as the write protect signal... */
      f->status_register[3] |= d->wrprot ? UPD_FDC_ST3_WRPROT : 0;
      f->status_register[3] |= d->tr00 ? UPD_FDC_ST3_TR00 : 0;
      f->status_register[3] |= d->ready ? UPD_FDC_ST3_READY : 0;
      break;
    case UPD_CMD_SENSE_INT:
      for( i = 0; i < 4; i++ ) {
        if( f->seek[i] >= 4 ) {					/* seek interrupt */
          f->status_register[0] &= ~0xc0;			/* normal termination */
          f->status_register[0] |= UPD_FDC_ST0_SEEK_END;	/* end of seek */
	  if( f->seek[i] == 5 )
            f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
	  else if( f->seek[i] == 6 )
            f->status_register[0] |= UPD_FDC_ST0_INT_READY | UPD_FDC_ST0_NOT_READY;
          f->seek[i] = f->seek_age[i] = 0;			/* end of seek */
    	  f->sense_int_res[0] = f->status_register[0] & 0xfb;	/* return head always 0 (11111011) */
          f->sense_int_res[1] = f->pcn[i];
	  i = 4;						/* one interrupt o.k. */
	}
      }
      if( f->seek[0] < 4 && f->seek[1] < 4 && 
          f->seek[2] < 4 && f->seek[3] < 4 )
        f->intrq = UPD_INTRQ_NONE;				/* delete INTRQ state */
      break;
    case UPD_CMD_RECALIBRATE:
      if( f->main_status & ( 1 << f->us ) ) break;   	/* previous seek in progress? */
      f->rec[f->us] = f->pcn[f->us];			/* save PCN */
      f->pcn[f->us] = 77;
      f->data_register[1] = 0x00;			/* to track0 */
      f->ncn[f->us] = f->data_register[1];		/* save new cylinder number */
      f->seek[f->us] = 2;				/* recalibrate started */
      seek_step( f, 1 );
      break;
    case UPD_CMD_SEEK:
      if( f->main_status & ( 1 << f->us ) ) break;   	/* previous seek in progress? */
      f->ncn[f->us] = f->data_register[1];		/* save new cylinder number */
      f->seek[f->us] = 1;				/* seek started */
      seek_step( f, 1 );
      break;
    case UPD_CMD_READ_ID:
      head_load( f );
      return;
    case UPD_CMD_READ_DATA:
    /* Speedlock hack */
      if( f->speedlock != -1 && !d->do_read_weak ) {	/* do not conflict with fdd weak read */
	u = ( f->data_register[2] & 0x01 ) + ( f->data_register[1] << 1 ) +
	    ( f->data_register[3] << 8 );
	if( f->data_register[3] == f->data_register[5] && u == 0x200 ) {
	  if( u == f->last_sector_read ) {
	    f->speedlock++;
	  } else {
	    f->speedlock = 0;
	    f->last_sector_read = u;
	  }
	} else {
	  f->last_sector_read = f->speedlock = 0;
	}
      }
    /* EOSpeedlock hack */

      f->rlen = 0x80 << ( f->data_register[4] > MAX_SIZE_CODE ? MAX_SIZE_CODE : f->data_register[4] );
      if( f->data_register[4] == 0 && f->data_register[7] < 128 )
        f->rlen = f->data_register[7];
      f->first_rw = 1;		/* always read at least one sector */
      head_load( f );
      return;
    case UPD_CMD_READ_DIAG:		/* READ TRACK */
      f->rlen = 0x80 << ( f->data_register[4] > MAX_SIZE_CODE ? MAX_SIZE_CODE : f->data_register[4] );
      if( f->data_register[4] == 0 && f->data_register[7] < 128 )
        f->rlen = f->data_register[7];
      head_load( f );
      return;

    case UPD_CMD_WRITE_DATA:
      if( d->wrprot ) {
        f->status_register[1] |= UPD_FDC_ST1_NOT_WRITEABLE;
        f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
	terminated = 1;
        break;
      }
      f->rlen = 0x80 << ( f->data_register[4] > MAX_SIZE_CODE ? MAX_SIZE_CODE : f->data_register[4] );
      if( f->data_register[4] == 0 && f->data_register[7] < 128 )
        f->rlen = f->data_register[7];
      f->first_rw = 1;		/* always write at least one sector */
      head_load( f );
      return;
    case UPD_CMD_WRITE_ID:			/* FORMAT TRACK */
      if( d->wrprot ) {
        f->status_register[1] |= UPD_FDC_ST1_NOT_WRITEABLE;
        f->status_register[0] |= UPD_FDC_ST0_INT_ABNORM;
	terminated = 1;
        break;
      }
      f->rlen = 0x80 << ( f->data_register[1] > MAX_SIZE_CODE ? MAX_SIZE_CODE : f->data_register[1] ); /* max 8192 byte/sector */
      head_load( f );
      return;
    case UPD_CMD_SCAN:			/* & 0x0c >> 2 == 00 - equal, 10 - low, 11 - high */
      f->scan = ( f->command_register & 0x0c ) >> 2 == 0 ? UPD_SCAN_EQ : 
		( f->command_register & 0x0c ) >> 2 == 0x03 ? UPD_SCAN_HI :
							     UPD_SCAN_LO;

      f->rlen = 0x80 << ( f->data_register[4] > MAX_SIZE_CODE ? MAX_SIZE_CODE : f->data_register[4] );
      head_load( f );
      return;
    }

    if( f->cmd->id < UPD_CMD_READ_ID && !terminated ) {	/* we have execution phase */
        f->main_status |= UPD_FDC_MAIN_DATAREQ;
	if( f->cmd->id < UPD_CMD_WRITE_DATA )
	  f->main_status |= UPD_FDC_MAIN_DATA_READ;
    } else {				/* result phase */
      cmd_result( f );			/* set up result phase */
    }
  } else {
    f->cycle++;
  }
}
