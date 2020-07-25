/* wd_fdc.c: Western Digital floppy disk controller emulation
   Copyright (c) 2002-2016 Stuart Brady, Fredrick Meunier, Philip Kendall,
                           Dmitry Sanarin, Gergely Szasz
   Copyright (c) 2016 Sergio Baldoví

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

#include <config.h>

#include <libspectrum.h>

#include "crc.h"
#include "event.h"
#include "spectrum.h"
#include "ui/ui.h"
#include "wd_fdc.h"

static void crc_preset( wd_fdc *f );
static void crc_add( wd_fdc *f, fdd_t *d );
static int read_id( wd_fdc *f );
static int read_datamark( wd_fdc *f );
static void wd_fdc_seek_verify( wd_fdc *f );
static void wd_fdc_type_i( wd_fdc *f );
static void wd_fdc_type_ii( wd_fdc *f );
static void wd_fdc_type_iii( wd_fdc *f );
static int wd_fdc_spinup( wd_fdc *f, libspectrum_byte b );
static void wd_fdc_event( libspectrum_dword last_tstates, int event,
			  void *user_data );
static void wd_fdc_wait_index ( void *fdc );

static int fdc_event, motor_off_event, timeout_event;

void
wd_fdc_init_events( void )
{
  fdc_event = event_register( wd_fdc_event, "WD FDC event" );
  motor_off_event = event_register( wd_fdc_event, "WD FDC motor off" );
  timeout_event = event_register( wd_fdc_event, "WD FDC timeout" );
}

void
wd_fdc_master_reset( wd_fdc *f )
{
  fdd_t *d = f->current_drive;

  f->spin_cycles = 0;
  f->direction = 0;
  f->head_load = 0;
  if( d ) {
    if( f->flags & WD_FLAG_BETA128 )
      fdd_motoron( d, 0 );
    else
      fdd_head_load( d, 0 );
  }
  f->read_id = 0;
  f->hlt = 1;
  if( !( f->flags & WD_FLAG_NOHLT ) && f->hlt_time > 0 ) f->hlt = 0;
  f->intrq = 0;
  f->datarq = 0;

  f->state = WD_FDC_STATE_NONE;
  f->status_type = WD_FDC_STATUS_TYPE1;

  if( d != NULL ) {
    while( !d->tr00 )
      fdd_step( d, FDD_STEP_OUT );
  }

  f->track_register = 0;
  f->sector_register = 0;
  f->data_register = 0;
  f->status_register = WD_FDC_SR_LOST; /* track 0 */

}

wd_fdc *
wd_fdc_alloc_fdc( wd_type_t type, int hlt_time, unsigned int flags )
{
  wd_fdc *fdc = libspectrum_new( wd_fdc, 1 );

  switch( type ) {
  default:
    type = WD1770;			/* illegal type converted to wd_fdc */
  case FD1793:
  case WD1773:
  case WD1770:
  case WD2797:
    fdc->rates[ 0 ] = 6;
    fdc->rates[ 1 ] = 12;
    fdc->rates[ 2 ] = 20;
    fdc->rates[ 3 ] = 30;
    break;
  case WD1772:
    fdc->rates[ 0 ] = 2;
    fdc->rates[ 1 ] = 3;
    fdc->rates[ 2 ] = 5;
    fdc->rates[ 3 ] = 6;
    break;
  }
  fdc->type = type;
  fdc->current_drive = NULL;
  fdc->hlt_time = hlt_time;
  fdc->flags = flags;			/* Beta128 connect HLD out to READY in and MOTOR ON */
  wd_fdc_master_reset( fdc );
  return fdc;
}

void
wd_fdc_set_intrq( wd_fdc *f )
{
  if( ( f->type == WD1770 || f->type == WD1772 ) &&
      f->status_register & WD_FDC_SR_MOTORON        ) {
    event_add_with_data( tstates + 2 * 			/* 10 rev: 10 * 200 / 1000 */
			 machine_current->timings.processor_speed,
			 motor_off_event, f );
  }

  if( ( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) &&
      f->head_load				    ) {
    event_add_with_data( tstates + 3 * 			/* 15 revolution: 15 * 200 / 1000 */
			 machine_current->timings.processor_speed,
			 motor_off_event, f );
  }
  if( f->intrq != 1 ) {
    f->intrq = 1;
    if( f->set_intrq ) f->set_intrq( f );
  }
}

void
wd_fdc_reset_intrq( wd_fdc *f )
{
  if( f->intrq == 1 ) {
    f->intrq = 0;
    if( f->reset_intrq ) f->reset_intrq( f );
  }
}

void
wd_fdc_set_datarq( wd_fdc *f )
{
  if( f->datarq != 1 ) {
    f->status_register |= WD_FDC_SR_IDX_DRQ;
    f->datarq = 1;
    if( f->set_datarq ) f->set_datarq( f );
  }
}

void
wd_fdc_reset_datarq( wd_fdc *f )
{
  if( f->datarq == 1 ) {
    f->status_register &= ~WD_FDC_SR_IDX_DRQ;
    f->datarq = 0;
    if( f->reset_datarq ) f->reset_datarq( f );
  }
}

void
wd_fdc_set_hlt( wd_fdc *f, int hlt )
{
  f->hlt = hlt > 0 ? 1 : 0;
}

static void
crc_preset( wd_fdc *f )
{
  f->crc = 0xffff;
}

static void
crc_add( wd_fdc *f, fdd_t *d )
{
  f->crc = crc_fdc( f->crc, d->data & 0xff );
}

static int
disk_ready( wd_fdc *f )
{
  if( f->flags & WD_FLAG_BETA128 )	/* Beta 128, READY = HLD */
    return f->head_load;

  if( f->flags & WD_FLAG_RDY )
    return f->extra_signal;		/* MB-02+ set ready, if any of fdd selected */

  return f->current_drive->ready;
}

/* return 0 if found an ID
   return 1 if not found ID
   return 2 if found but with CRC error (READ ADDRESS command)

   what we can do, if disk not rotating, or head not loaded?
*/
static int
read_id( wd_fdc *f )
{
  int i = f->rev;
  fdd_t *d = f->current_drive;

  f->id_mark = WD_FDC_AM_NONE;

  if( f->rev <= 0 )
    return 1;

  while( i == f->rev ) { /* **FIXME d->motoron? */
    crc_preset( f );
    if( f->dden ) {	/* double density (MFM) */
      fdd_read_data( d );
      if( d->index ) f->rev--;
      crc_add(f, d);
      if( d->data == 0xffa1 ) {
        fdd_read_data( d ); crc_add(f, d);
        if( d->index ) f->rev--;
        if( d->data != 0xffa1 )
          continue;
        fdd_read_data( d ); crc_add(f, d);
        if( d->index ) f->rev--;
        if( d->data != 0xffa1 )
          continue;
      } else {		/* no 0xa1 with missing clock... */
        continue;
      }
    }
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    if( f->dden ) {	/* double density (MFM) */
      if( d->data != 0x00fe )
        continue;
    } else {		/* single density (FM) */
      if( d->data != 0xfffe )
        continue;
    }
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    f->id_track = d->data;
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    f->id_head = d->data;
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    f->id_sector = d->data;
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    f->id_length = d->data;
    if( f->non_ibm_len_code ) {		/* 00->256 01->512 10->1024 11->128 */
      f->sector_length = 0x80 << ( ( d->data + 1 ) & 0x03 );
    } else {				/* 00->128 01->256 10->512 11->1024 */
      f->sector_length = 0x80 << ( d->data & 0x03 );
    }
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    fdd_read_data( d ); crc_add(f, d);
    if( d->index ) f->rev--;
    if( f->crc != 0x0000 ) {
      f->status_register |= WD_FDC_SR_CRCERR;
      f->id_mark = WD_FDC_AM_ID;
      return 2;
    } else {
      f->status_register &= ~WD_FDC_SR_CRCERR;
      f->id_mark = WD_FDC_AM_ID;
      return 0;
    }
  }
  return 1;
}

static int
read_datamark( wd_fdc *f )
{
  fdd_t *d = f->current_drive;
  int i;

  f->id_mark = WD_FDC_AM_NONE;

  if( f->dden ) {	/* double density (MFM) */
    for( i = 40; i > 0; i-- ) {
      fdd_read_data( d );
      if( d->data == 0x4e )		/* read next */
	continue;

      if( d->data == 0x00 )		/* go to PLL sync */
	break;

      return 1;				/* something wrong... */
    }
    for( ; i > 0; i-- ) {
      crc_preset( f );
      fdd_read_data( d ); crc_add(f, d);
      if( d->data == 0x00 )
	continue;

      if( d->data == 0xffa1 )	/* got to a1 mark */
	break;

      return 1;
    }
    for( i = d->data == 0xffa1 ? 2 : 3; i > 0; i-- ) {
      fdd_read_data( d ); crc_add(f, d);
      if( d->data != 0xffa1 )
	return 1;
    }
    fdd_read_data( d ); crc_add(f, d);
    if( d->data < 0x00f8 || d->data > 0x00fb )	/* !fb deleted mark */
      return 1;
    if( d->data != 0x00fb )
      f->ddam = 1;
    else
      f->ddam = 0;
    f->id_mark = WD_FDC_AM_DATA;
    return 0;
  } else {		/* SD -> FM */
    for( i = 30; i > 0; i-- ) {
      fdd_read_data( d );
      if( d->data == 0xff )		/* read next */
	continue;

      if( d->data == 0x00 )		/* go to PLL sync */
	break;

      return 1;				/* something wrong... */
    }
    for( ; i > 0; i-- ) {
      crc_preset( f );
      fdd_read_data( d ); crc_add(f, d);
      if( d->data == 0x00 )
	continue;

      if( d->data >= 0xfff8 && d->data <= 0xfffb )	/* !fb deleted mark */
	break;

      return 1;
    }
    if( i == 0 ) {
      fdd_read_data( d ); crc_add(f, d);
      if( d->data < 0xfff8 || d->data > 0xfffb )	/* !fb deleted mark */
	return 1;
    }
    if( d->data != 0x00fb )
      f->ddam = 1;
    else
      f->ddam = 0;
    f->id_mark = WD_FDC_AM_DATA;
    return 0;
  }
  return 1;
}

libspectrum_byte
wd_fdc_sr_read( wd_fdc *f )
{
  fdd_t *d = f->current_drive;

  wd_fdc_reset_intrq( f );

  if( f->status_type == WD_FDC_STATUS_TYPE1 ) {
    f->status_register &= ~WD_FDC_SR_IDX_DRQ;
    if( !d->loaded || d->index_pulse )
      f->status_register |= WD_FDC_SR_IDX_DRQ;
  }
  if( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) {

/*    if( f->status_register & WD_FDC_SR_BUSY )
      f->status_register |= WD_FDC_SR_MOTORON;
    else
      f->status_register &= ~WD_FDC_SR_MOTORON; */

    if( disk_ready( f ) )
      f->status_register &= ~WD_FDC_SR_MOTORON;
    else
      f->status_register |= WD_FDC_SR_MOTORON;
  }

  return f->status_register;
}

static void
wd_fdc_seek_verify_read_id( wd_fdc *f )
{
  fdd_t *d = f->current_drive;
  int i;
  f->read_id = 1;

  event_remove_type( fdc_event );
  if( f->id_mark == WD_FDC_AM_NONE ) {
    while( f->rev ) {
      i = d->disk.i >= d->disk.bpt ? 0 : d->disk.i;	/* start position */
      if( !read_id( f ) ) {
	if( f->id_track != f->track_register ) {
	  f->status_register |= WD_FDC_SR_RNF;
	}
      } else
        f->id_mark = WD_FDC_AM_NONE;
      i = d->disk.bpt ? ( d->disk.i - i ) * 200 / d->disk.bpt : 200;
      if( i > 0 ) {
        event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			   machine_current->timings.processor_speed / 1000,
			   fdc_event, f );
        return;
      } else if( f->id_mark != WD_FDC_AM_NONE )
        break;
    }
    if( f->id_mark == WD_FDC_AM_NONE )
      f->status_register |= WD_FDC_SR_RNF;
  }
  f->state = WD_FDC_STATE_NONE;
  f->status_register &= ~WD_FDC_SR_BUSY;
  wd_fdc_set_intrq( f );
  f->read_id = 0;
}

static void
wd_fdc_seek_verify( wd_fdc *f )
{
  fdd_t *d = f->current_drive;

  event_remove_type( fdc_event );
  if( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) {
    if( !f->hlt ) {
      event_add_with_data( tstates + 5 * 			/* sample every 5 ms */
		    machine_current->timings.processor_speed / 1000,
			fdc_event, f );
      return;
    }
    if( f->head_load )
      f->status_register |= WD_FDC_SR_SPINUP;
			/* when set, it indicates head is loaded and enganged.
			   This bit is logical "and" of HLD and "HLT" signals.  */
  }

  if( d->tr00 )
    f->status_register |= WD_FDC_SR_LOST;
  else
    f->status_register &= ~WD_FDC_SR_LOST;

  f->rev = 5;
  f->id_mark = WD_FDC_AM_NONE;
  wd_fdc_seek_verify_read_id( f );
}

static void
wd_fdc_type_i( wd_fdc *f )
{
  libspectrum_byte b = f->command_register;
  fdd_t *d = f->current_drive;

  if( f->state == WD_FDC_STATE_SEEK_DELAY ) {	/* after delay */
    if( ( b & 0x60 ) != 0x00 )			/* STEP/STEP-IN/STEP-OUT */
      goto type_i_verify;
    goto type_i_loop;
  } else {					/* WD_FDC_STATE_SEEK */
    f->status_register |= WD_FDC_SR_SPINUP;
  }

  if( ( b & 0x60 ) != 0x00 ) {			/* STEP/STEP-IN/STEP-OUT */
    if( b & 0x40 )
      f->direction = b & 0x20 ? FDD_STEP_OUT : FDD_STEP_IN;
    if( b & 0x10 )				/* update? */
      goto type_i_update;
    goto type_i_noupdate;
  }
						/* SEEK or RESTORE */
  if ( !( b & 0x10 ) ) {				/* RESTORE */
    f->track_register = 0xff;
    f->data_register = 0;
  }

type_i_loop:
  if( f->track_register != f->data_register ) {
    f->direction = f->track_register < f->data_register ?
			FDD_STEP_IN : FDD_STEP_OUT;

type_i_update:
    f->track_register += f->direction == FDD_STEP_IN ? 1 : -1;

type_i_noupdate:
    if( d->tr00 && f->direction == FDD_STEP_OUT ) {
      f->track_register = 0;
    } else {
      fdd_step( d, f->direction );
      f->state = WD_FDC_STATE_SEEK_DELAY;
      event_remove_type( fdc_event );
      event_add_with_data( tstates + f->rates[ b & 0x03 ] *
			   machine_current->timings.processor_speed / 1000,
			   fdc_event, f );
      return;
    }
  }

type_i_verify:
  if( b & 0x04 ) {

    if( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) {
      f->head_load = 1;
      event_remove_type( motor_off_event );
      if( f->flags & WD_FLAG_BETA128 )
        fdd_motoron( d, 1 );
      else
        fdd_head_load( d, 1 );
      event_remove_type( fdc_event );
      event_add_with_data( tstates + 15 * 				/* 15ms */
		    machine_current->timings.processor_speed / 1000,
			fdc_event, f );
    }

    f->state = WD_FDC_STATE_VERIFY;

    if( ( f->type == WD1770 || f->type == WD1772 ) &&
	!( f->status_register & WD_FDC_SR_MOTORON ) ) {
      f->status_register |= WD_FDC_SR_MOTORON;
      fdd_motoron( f->current_drive, 1 );
      event_remove_type( fdc_event );
      event_add_with_data( tstates + 12 * 		/* 6 revolution 6 * 200 / 1000 */
		    machine_current->timings.processor_speed / 10,
			fdc_event, f );
      return;
    }

    wd_fdc_seek_verify( f );
    return;
  }

  if( d->tr00 )
    f->status_register |= WD_FDC_SR_LOST;
  else
    f->status_register &= ~WD_FDC_SR_LOST;

  f->state = WD_FDC_STATE_NONE;
  f->status_register &= ~WD_FDC_SR_BUSY;
  wd_fdc_set_intrq( f );
}

static void
wd_fdc_type_ii_seek( wd_fdc *f )
{
  libspectrum_byte b = f->command_register;
  fdd_t *d = f->current_drive;
  int i;

  event_remove_type( fdc_event );
  if( f->id_mark == WD_FDC_AM_NONE ) {
    f->read_id = 1;
    while( f->rev ) {
      i = d->disk.i >= d->disk.bpt ? 0 : d->disk.i;	/* start position */
      if( !read_id( f ) ) {
        if( ( f->data_check_head != -1 && f->data_check_head != !!( f->id_head ) ) ||
	    ( f->id_track != f->track_register || f->id_sector != f->sector_register ) ) {
          f->id_mark = WD_FDC_AM_NONE;
	}
      } else {
        f->id_mark = WD_FDC_AM_NONE;
      }
      i = d->disk.bpt ?
	( d->disk.i - i ) * 200 / d->disk.bpt : 200;
      if( i > 0 ) {
        event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			     machine_current->timings.processor_speed / 1000,
			     fdc_event, f );
        return;
      } else if( f->id_mark != WD_FDC_AM_NONE ) {
	break;
      }
    }
  }

  f->read_id = 0;

  if( f->id_mark == WD_FDC_AM_NONE ) {
    f->status_register |= WD_FDC_SR_RNF;
    f->status_register &= ~WD_FDC_SR_BUSY;
    f->state = WD_FDC_STATE_NONE;
    wd_fdc_set_intrq( f );
    return;
  }

  if( f->state == WD_FDC_STATE_READ ) {
    if( f->id_mark == WD_FDC_AM_ID )
      read_datamark( f );

    if( f->id_mark == WD_FDC_AM_NONE ) {		/* not found */
      f->status_register |= WD_FDC_SR_RNF;
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      return;
    }
    if( f->ddam )
      f->status_register |= WD_FDC_SR_SPINUP;	/* set deleted data mark */
    f->data_offset = 0;
    wd_fdc_set_datarq( f );

  } else {
    f->ddam = b & 0x01;
    for( i = 11; i > 0; i-- )	/* "delay" 11 GAP byte */
      fdd_read_data( d );
    wd_fdc_set_datarq( f );
    f->data_offset = 0;
    if( f->dden )
      for( i = 11; i > 0; i-- )	/* "delay" another 11 GAP byte */
	fdd_read_data( d );

    d->data = 0x00;
    for( i = f->dden ? 12 : 6; i > 0; i-- )	/* write 6/12 zero */
      fdd_write_data( d );
    crc_preset( f );
    if( f->dden ) {				/* MFM */
      d->data = 0xffa1;
      for( i = 3; i > 0; i-- ) {		/* write 3 0xa1 with clock mark */
	fdd_write_data( d ); crc_add(f, d);
      }
    }
    d->data = ( f->ddam ? 0x00f8 : 0x00fb ) |
    			( f->dden ? 0x0000 : 0xff00 );	/* write data mark */
    fdd_write_data( d ); crc_add(f, d);
  }
  event_remove_type( timeout_event );
  event_add_with_data( tstates + 			/* 5 revolutions: 5 * 200 / 1000 */
		       machine_current->timings.processor_speed,
		       timeout_event, f );
}

static void
wd_fdc_type_ii( wd_fdc *f )
{
  libspectrum_byte b = f->command_register;
  fdd_t *d = f->current_drive;

  event_remove_type( fdc_event );
  if( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) {
    if( !disk_ready( f ) ) {
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      return;
    }
    if( !f->hlt ) {
      event_add_with_data( tstates + 5 *
    		    machine_current->timings.processor_speed / 1000,
			fdc_event, f );
      return;
    }
  }

  if( f->state == WD_FDC_STATE_WRITE ) {
    if( d->wrprot ) {
      f->status_register |= WD_FDC_SR_WRPROT;
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      return;
    }
    f->status_register &= ~WD_FDC_SR_WRPROT;
  }

  f->data_multisector = b & 0x10 ? 1 : 0;
  f->rev = 5;
  f->id_mark = WD_FDC_AM_NONE;
  wd_fdc_type_ii_seek( f );
}

static void
wd_fdc_type_iii( wd_fdc *f )
{
  int i;
  fdd_t *d = f->current_drive;

  event_remove_type( fdc_event );
  if( !f->read_id && ( f->type == WD1773 || f->type == FD1793 || f->type == WD2797) ) {
    if( !disk_ready( f ) ) {
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      return;
    }
    if( !f->hlt ) {
      event_add_with_data( tstates + 5 *
    		    machine_current->timings.processor_speed / 1000,
			fdc_event, f );
      return;
    }
  }
  if( f->state == WD_FDC_STATE_WRITETRACK ) {		/* ----WRITE TRACK---- */
    if( d->wrprot ) {
      f->status_register |= WD_FDC_SR_WRPROT;
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      return;
    }
    f->status_register &= ~WD_FDC_SR_WRPROT;

    f->data_offset = 0;
    fdd_wait_index_hole( d );
    wd_fdc_set_datarq( f );
  } else if( f->state == WD_FDC_STATE_READTRACK ) {	/* ----READ TRACK---- */
    fdd_wait_index_hole( d );
    wd_fdc_set_datarq( f );
  } else {						/* ----READID---- */
    if( !f->read_id ) {
      f->read_id = 1;
      f->rev = 5;
      f->id_mark = WD_FDC_AM_NONE;
    }
    if( f->id_mark == WD_FDC_AM_NONE ) {
      while( f->rev ) {
        i = d->disk.i >= d->disk.bpt ? 0 : d->disk.i;	/* start position */
        read_id( f );
        i = d->disk.bpt ?
	    ( d->disk.i - i ) * 200 / d->disk.bpt : 200;
	if( i > 0 ) {
          event_add_with_data( tstates + i *		/* i * 1/20 revolution */
			       machine_current->timings.processor_speed / 1000,
			       fdc_event, f );
          return;
	} else if( f->id_mark != WD_FDC_AM_NONE )
	  break;
      }
      if( f->id_mark == WD_FDC_AM_NONE ) {
        f->state = WD_FDC_STATE_NONE;
        f->status_register |= WD_FDC_SR_RNF;
        f->status_register &= ~WD_FDC_SR_BUSY;
        wd_fdc_set_intrq( f );
        f->read_id = 0;
        return;
      }
    }
    f->read_id = 0;
    f->data_offset = 0;
    wd_fdc_set_datarq( f );
  }
  event_remove_type( timeout_event );
  event_add_with_data( tstates + 2 * 20 *	/* 2 revolutions: 2 * 200 / 1000  */
		       machine_current->timings.processor_speed / 100,
		       timeout_event, f );
}

static void
wd_fdc_event( libspectrum_dword last_tstates GCC_UNUSED, int event,
	      void *user_data )
{
  wd_fdc *f = user_data;
  fdd_t *d = f->current_drive;

  if( event == timeout_event ) {
    if( f->state == WD_FDC_STATE_READ       ||
	f->state == WD_FDC_STATE_WRITE      ||
	f->state == WD_FDC_STATE_READTRACK  ||
	f->state == WD_FDC_STATE_WRITETRACK ||
	f->state == WD_FDC_STATE_READID        ) {
      f->state = WD_FDC_STATE_NONE;
      f->status_register |= WD_FDC_SR_LOST;
      f->status_register &= ~WD_FDC_SR_BUSY;
      wd_fdc_reset_datarq( f );
      wd_fdc_set_intrq( f );
    }
    return;
  }

  if( event == motor_off_event ) {
    if( f->type == WD1770 || f->type == WD1772 ) {
      f->status_register &= ~WD_FDC_SR_MOTORON;
      fdd_motoron( d, 0 );
    } else {						/* WD1773/FD1973 */
      f->head_load = 0;
      if( f->flags & WD_FLAG_BETA128 )
        fdd_motoron( d, 0 );
      else
        fdd_head_load( d, 0 );
    }
    return;
  }

  if( ( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) &&
        f->hlt_time > 0 && f->head_load && !f->hlt )
    f->hlt = 1;

  if( ( ( f->type == WD1770 || f->type == WD1772 ) &&
	( f->status_register & WD_FDC_SR_MOTORON ) &&
	f->status_type == WD_FDC_STATUS_TYPE1 ) ||
      ( ( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) &&
	( f->state == WD_FDC_STATE_SEEK ||
	  f->state == WD_FDC_STATE_SEEK_DELAY ) &&
	f->head_load ) ) {
    f->status_register |= WD_FDC_SR_SPINUP;
  }

  if( f->read_id ) {
    if( f->state == WD_FDC_STATE_VERIFY )
      wd_fdc_seek_verify_read_id( f );
    else if( ( f->state == WD_FDC_STATE_READ || f->state == WD_FDC_STATE_WRITE ) &&
	     f->datarq )
      f->datarq = 0, wd_fdc_set_datarq( f );
    else if( f->state == WD_FDC_STATE_READ || f->state == WD_FDC_STATE_WRITE )
      wd_fdc_type_ii_seek( f );
    else if( f->state == WD_FDC_STATE_READID )
      wd_fdc_type_iii( f );
  } else if( f->state == WD_FDC_STATE_SEEK || f->state == WD_FDC_STATE_SEEK_DELAY )
    wd_fdc_type_i( f );
  else if( f->state == WD_FDC_STATE_VERIFY )
    wd_fdc_seek_verify( f );
  else if( ( f->state == WD_FDC_STATE_READ || f->state == WD_FDC_STATE_WRITE ) &&
	   f->datarq )
    f->datarq = 0, wd_fdc_set_datarq( f );
  else if( f->state == WD_FDC_STATE_READ || f->state == WD_FDC_STATE_WRITE )
    wd_fdc_type_ii( f );
  else if( ( f->state == WD_FDC_STATE_READTRACK  ||
	     f->state == WD_FDC_STATE_READID     ||
	     f->state == WD_FDC_STATE_WRITETRACK   ) &&
	   f->datarq )
      f->datarq = 0, wd_fdc_set_datarq( f );
  else if( f->state == WD_FDC_STATE_READTRACK  ||
	   f->state == WD_FDC_STATE_READID     ||
	   f->state == WD_FDC_STATE_WRITETRACK    )
    wd_fdc_type_iii( f );
}

/* this chart looks like most close of the reality...
           !
        +--+--+
       /  is   \  no
      ! MO =  0 !----->----+
       \   ?   /           !
        +--+--+            !
           !               !
+--------------+--------+  !
!        set MO         !  !
+--------------+--------+  v
           !               !
        +--+--+            !
       /  is   \  no       !
      ! h  =  0 !----->----+
       \   ?   /           !
        +--+--+            !
           !yes            !
+--------------+--------+  v
! delay 6 index pulses  !  !
+--------------+--------+  !
           !               !
           !-------<-------+
           !
*/

static int
wd_fdc_spinup( wd_fdc *f, libspectrum_byte b )
{
  libspectrum_dword delay = 0;
  fdd_t *d = f->current_drive;

  if( f->state != WD_FDC_STATE_SEEK && ( b & 0x04 ) )
    delay = 30;

  if( f->type == WD1770 || f->type == WD1772 ) {

    if( !( f->status_register & WD_FDC_SR_MOTORON ) ) {
      f->status_register |= WD_FDC_SR_MOTORON;
      fdd_motoron( d, 1 );
      if( !( b & 0x08 ) )
        delay += 6 * 200;
    }
  } else {			/* WD1773/FD1793/WD2797 */
    event_remove_type( motor_off_event );
    if( f->state == WD_FDC_STATE_SEEK ) {
      if( b & 0x08 ) {
	f->head_load = 1;
        if( f->flags & WD_FLAG_BETA128 )
          fdd_motoron( d, 1 );
        else
	  fdd_head_load( d, 1 );
      } else if( !( b & 0x04 ) ) {	/* HLD reset only if V flag == 0 too */
	f->head_load = 0;
        if( !( f->flags & WD_FLAG_NOHLT ) && f->hlt_time > 0 ) f->hlt = 0;		/* reset the trigger */
        if( f->flags & WD_FLAG_BETA128 )
          fdd_motoron( d, 0 );
        else
	  fdd_head_load( d, 0 );
      }
      return 0;
    } else {
      f->head_load = 1;
      if( f->flags & WD_FLAG_BETA128 )
        fdd_motoron( d, 1 );
      else
        fdd_head_load( d, 1 );
      if( f->hlt_time > 0 )
        delay += f->hlt_time;
    }
  }

  /* For Type III commands on WD2797 */
  if( f->type == WD2797 && ( b & 0xc0 ) == 0xc0 && ( b & 0x30 ) != 0x10 )
    fdd_set_head( d, b & 0x02 ? 1 : 0 );

  if( delay ) {
    event_remove_type( fdc_event );
    event_add_with_data( tstates + delay *
    		    machine_current->timings.processor_speed / 1000,
			fdc_event, f );
    return 1;
  }
  return 0;
}

void
wd_fdc_cr_write( wd_fdc *f, libspectrum_byte b )
{
  fdd_t *d = f->current_drive;

  wd_fdc_reset_intrq( f );

  if( ( b & 0xf0 ) == 0xd0 ) {			/* Type IV - Force Interrupt */
    event_remove_type( fdc_event );
    f->status_register &= ~( WD_FDC_SR_BUSY   | WD_FDC_SR_WRPROT |
			     WD_FDC_SR_CRCERR | WD_FDC_SR_IDX_DRQ );
    f->state = WD_FDC_STATE_NONE;
    f->status_type = WD_FDC_STATUS_TYPE1;
    wd_fdc_reset_datarq( f );

    if( b & 0x08 )
      wd_fdc_set_intrq( f );
    else if( b & 0x04 ) {
      d->fdc_index = wd_fdc_wait_index;
      d->fdc = f;
    }

    if( d->tr00 )
      f->status_register |= WD_FDC_SR_LOST;
    else
      f->status_register &= ~WD_FDC_SR_LOST;

    wd_fdc_spinup( f, b & 0xf7 ); /* spinup motor, but we do not have a 'h' bit!  */

    return;
  }

  if( f->status_register & WD_FDC_SR_BUSY )
    return;

  f->command_register = b;
  f->status_register |= WD_FDC_SR_BUSY;

  /* keep spindle motor on: */
  event_remove_type( motor_off_event );

  if( !( b & 0x80 ) ) {                         /* Type I */
    f->state = WD_FDC_STATE_SEEK;
    f->status_type = WD_FDC_STATUS_TYPE1;
    f->status_register &= ~( WD_FDC_SR_CRCERR | WD_FDC_SR_RNF |
			     WD_FDC_SR_IDX_DRQ );
    wd_fdc_reset_datarq( f );

    f->rev = 5;
    if( wd_fdc_spinup( f, b ) )
      return;
    wd_fdc_type_i( f );
  } else if( !( b & 0x40 ) ) {                  /* Type II */
    if( f->type == WD1773 || f->type == FD1793 ) {
      if( !disk_ready( f ) ) {
        f->status_register &= ~WD_FDC_SR_BUSY;
        f->state = WD_FDC_STATE_NONE;
        wd_fdc_set_intrq( f );
        return;
      }
    }
    if( f->type == WD1773 && b & 0x02 )
      f->data_check_head = b & 0x08 ? 1 : 0;
    else if( f->type == WD2797 )
      f->data_check_head = b & 0x02 ? 1 : 0;
    else
      f->data_check_head = -1;

    /* WD2797 (and FD1797) can read sectors with non-IBM-compatible
       sector length codes */
    f->non_ibm_len_code = ( f->type == WD2797 && !( b & 0x08 ) ) ? 1 : 0;

    f->state = b & 0x20 ? WD_FDC_STATE_WRITE : WD_FDC_STATE_READ;
    f->status_type = WD_FDC_STATUS_TYPE2;
    f->status_register &= ~( WD_FDC_SR_WRPROT | WD_FDC_SR_RNF |
			     WD_FDC_SR_IDX_DRQ| WD_FDC_SR_LOST|
			     WD_FDC_SR_SPINUP );

    if( f->type == WD2797 ) fdd_set_head( d, b & 0x02 ? 1 : 0 );

    f->rev = 5;
    if( wd_fdc_spinup( f, b ) )
      return;
    wd_fdc_type_ii( f );
  } else if( ( b & 0x30 ) != 0x10 ) {           /* Type III */
    if( f->type == WD1773 || f->type == FD1793 || f->type == WD2797 ) {
      if( !disk_ready( f ) ) {
        f->status_register &= ~WD_FDC_SR_BUSY;
        f->state = WD_FDC_STATE_NONE;
        wd_fdc_set_intrq( f );
        return;
      }
    }

    f->state = b & 0x20 ? ( b & 0x10 ?
			    WD_FDC_STATE_WRITETRACK : WD_FDC_STATE_READTRACK ) :
		WD_FDC_STATE_READID;
    f->status_type = WD_FDC_STATUS_TYPE2;
    f->status_register &= ~( WD_FDC_SR_SPINUP | WD_FDC_SR_RNF |
			     WD_FDC_SR_IDX_DRQ| WD_FDC_SR_LOST );

    f->rev = 5;
    if( wd_fdc_spinup( f, b ) )
      return;
    wd_fdc_type_iii( f );
  }
}

libspectrum_byte
wd_fdc_tr_read( wd_fdc *f )
{
  return f->track_register;
}

void
wd_fdc_tr_write( wd_fdc *f, libspectrum_byte b )
{
  f->track_register = b;
}

libspectrum_byte
wd_fdc_sec_read( wd_fdc *f )
{
  return f->sector_register;
}

void
wd_fdc_sec_write( wd_fdc *f, libspectrum_byte b )
{
  f->sector_register = b;
}

libspectrum_byte
wd_fdc_dr_read( wd_fdc *f )
{
  fdd_t *d = f->current_drive;

  if( f->flags & WD_FLAG_DRQ &&
	f->status_register & WD_FDC_SR_BUSY )
    event_remove_type( fdc_event );

  if( f->state == WD_FDC_STATE_READ ) {
    f->data_offset++;				/* count read bytes */
    fdd_read_data( d ); crc_add(f, d);		/* read a byte */
    if( d->data > 0xff ) {			/* no data */
      f->status_register |= WD_FDC_SR_RNF;
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->status_type = WD_FDC_STATUS_TYPE2;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      wd_fdc_reset_datarq( f );
    } else {
      f->data_register = d->data;
      if( f->data_offset == f->sector_length ) {	/* read the CRC */
	fdd_read_data( d ); crc_add(f, d);
	fdd_read_data( d ); crc_add(f, d);

	/* FIXME: make this per-FDC */
	event_remove_type( timeout_event );	/* clear the timeout */

	if( f->crc == 0x0000 && f->data_multisector ) {
	  f->sector_register++;
	  f->rev = 5;
	  wd_fdc_reset_datarq( f );
	  event_add_with_data( tstates +	 	/* 5 revolutions: 5 * 200 / 1000 */
			       machine_current->timings.processor_speed,
			       timeout_event, f );
	  event_add_with_data( tstates + 2 * 		/* 20 ms delay */
			       machine_current->timings.processor_speed / 100,
			       fdc_event, f );
	} else {
	  f->status_register &= ~WD_FDC_SR_BUSY;
	  if( f->crc == 0x0000 )
	    f->status_register &= ~WD_FDC_SR_CRCERR;
	  else
	    f->status_register |= WD_FDC_SR_CRCERR;
	  f->status_type = WD_FDC_STATUS_TYPE2;
	  f->state = WD_FDC_STATE_NONE;
	  wd_fdc_set_intrq( f );
	  wd_fdc_reset_datarq( f );
	}
      }
    }
  } else if( f->state == WD_FDC_STATE_READID ) {
    switch( f->data_offset ) {
    case 0:		/* track */
      f->data_register = f->id_track;
      break;
    case 1:		/* head */
      f->data_register = f->id_head;
      break;
    case 2:		/* sector */
      f->data_register = f->id_sector;
      break;
    case 3:		/* length */
      f->data_register = f->id_length;
      break;
    case 4:		/* crc1 */
      f->data_register = f->crc >> 8;
      break;
    case 5:		/* crc2 */
      f->sector_register = f->id_track;
      f->data_register = f->crc & 0xff;
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->status_type = WD_FDC_STATUS_TYPE2;
      f->state = WD_FDC_STATE_NONE;
      event_remove_type( timeout_event );	/* clear the timeout */
      wd_fdc_set_intrq( f );
      wd_fdc_reset_datarq( f );
      break;
    default:
      break;
    }
    f->data_offset++;
  } else if( f->state == WD_FDC_STATE_READTRACK ) {
						/* unformatted/out of track looks like 1x 0x00 */
    fdd_read_data( d );			/* read a byte and give to host */
    f->data_register = d->data & 0x00ff;	/* drop clock marks */
    if( d->index ) {
      event_remove_type( timeout_event );	/* clear the timeout */
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->status_type = WD_FDC_STATUS_TYPE2;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      wd_fdc_reset_datarq( f );
    }
  }
  if( ( f->flags & WD_FLAG_DRQ ) &&
	( f->status_register & WD_FDC_SR_BUSY ) ) {	/* we need a next datarq */
    event_add_with_data( tstates + 30 * 		/* 30 us delay */
			       machine_current->timings.processor_speed / 1000000,
			       fdc_event, f );
  }
  return f->data_register;
}

void
wd_fdc_dr_write( wd_fdc *f, libspectrum_byte b )
{
  fdd_t *d = f->current_drive;

  f->data_register = b;
  if( f->state == WD_FDC_STATE_WRITE ) {
    d->data = b;
    f->data_offset++;				/* count bytes read */
    fdd_write_data( d ); crc_add(f, d);
    if( f->data_offset == f->sector_length ) {	/* write the CRC */
      d->data = f->crc >> 8;
      fdd_write_data( d );			/* write crc1 */
      d->data = f->crc & 0xff;
      fdd_write_data( d );			/* write crc2 */
      d->data = 0xff;
      fdd_write_data( d );			/* write 1 byte of ff? */

      event_remove_type( timeout_event );	/* clear the timeout */

      if( f->data_multisector ) {
	f->sector_register++;
	f->rev = 5;
	wd_fdc_reset_datarq( f );
	event_add_with_data( tstates +		/* 5 revolutions: 5 * 200 / 1000 */
			     machine_current->timings.processor_speed,
			     timeout_event, f );
	event_add_with_data( tstates + 2 * 		/* 20ms delay */
			     machine_current->timings.processor_speed / 100,
			     fdc_event, f );
      } else {
	f->status_register &= ~WD_FDC_SR_BUSY;
	f->status_type = WD_FDC_STATUS_TYPE2;
	f->state = WD_FDC_STATE_NONE;
	wd_fdc_set_intrq( f );
	wd_fdc_reset_datarq( f );
      }
    }
  } else if( f->state == WD_FDC_STATE_WRITETRACK ) {
    d->data = b;
    if( f->dden ) {			/* MFM */
      if( b == 0xf7 ) {			/* CRC */
	d->data = f->crc >> 8;
	fdd_write_data( d );			/* write crc1 */
	d->data = f->crc & 0xff;
      } else if ( b == 0xf5 ) {
	d->data = 0xffa1;		/* and preset CRC */
	f->crc = 0xcdb4;		/* 3x crc = crc_fdc( crc, 0xa1 )???? */
      } else if ( b == 0xf6 ) {
	d->data = 0xffc2;
      } else {
	crc_add(f, d);
      }
    } else {				/* FM */
      if( b == 0xf7 ) {			/* CRC */
	d->data = f->crc >> 8;
	fdd_write_data( d );			/* write crc1 */
	d->data = f->crc & 0xff;
      } else if ( b == 0xfe || ( b >= 0xf8 && b <= 0xfb ) ) {
	crc_preset( f );		/* preset CRC */
	crc_add(f, d);
	d->data |= 0xff00;
      } else if ( b == 0xfc ) {
	d->data |= 0xff00;
      } else {
	crc_add(f, d);
      }
    }
    fdd_write_data( d );			/* write a byte */

    if( d->index ) {
      event_remove_type( timeout_event );	/* clear the timeout */
      f->status_register &= ~WD_FDC_SR_BUSY;
      f->state = WD_FDC_STATE_NONE;
      wd_fdc_set_intrq( f );
      wd_fdc_reset_datarq( f );
    }
  }
  if( ( f->flags & WD_FLAG_DRQ ) &&
	f->status_register & WD_FDC_SR_BUSY ) {	/* we need a next datarq */
    /* wd_fdc_reset_datarq( f ); */
    event_add_with_data( tstates + 30 * 		/* 30 us delay */
			       machine_current->timings.processor_speed / 1000000,
			       fdc_event, f );
  }
}

static void
wd_fdc_wait_index ( void *fdc )
{
  wd_fdc *f = fdc;
  wd_fdc_set_intrq( f ); /* generate an interrupt */
}
