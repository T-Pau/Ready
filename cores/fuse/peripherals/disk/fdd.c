/* fdd.c: Routines for emulating floppy disk drives
   Copyright (c) 2007-2016 Gergely Szasz, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2016 BogDan Vatra

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

#include <config.h>

#include <libspectrum.h>

#include "bitmap.h"
#include "compat.h"
#include "event.h"
#include "fdd.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "spectrum.h"
#include "settings.h"
#include "ui/ui.h"
#include "upd_fdc.h"
#include "wd_fdc.h"

#define FDD_LOAD_FACT 2
#define FDD_HEAD_FACT 16		/* load head */
#define FDD_STEP_FACT 34
#define FDD_MAX_TRACK 99		/* absolute maximum number of track*/
#define FDD_TRACK_TRESHOLD 10		/* unreadable disk*/

typedef enum fdd_write_t {
  FDD_READ = 0,
  FDD_WRITE,
} fdd_write_t;

static const char * const fdd_error[] = {
  "OK",
  "invalid disk geometry",
  "read only disk",
  "disk not exist (disabled)",

  "unknown error code"			/* will be the last */
};

const fdd_params_t fdd_params[] = {
  { 0, 0, 0 },		/* Disabled */
  { 1, 1, 40 },		/* Single-sided 40 track */
  { 1, 2, 40 },		/* Double-sided 80 track */
  { 1, 1, 80 },		/* Single-sided 40 track */
  { 1, 2, 80 }		/* Double-sided 80 track */
};

static void
fdd_event( libspectrum_dword last_tstates, int event, void *user_data );

static int motor_event;
static int index_event;

static int fdd_motor = 0; /* to manage 'disk' icon */

static int
fdd_init_events( void *context )
{
  motor_event = event_register( fdd_event, "FDD motor on" );
  index_event = event_register( fdd_event, "FDD index" );

  upd_fdc_init_events();
  wd_fdc_init_events();

  return 0;
}

void
fdd_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_EVENT,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_FDD, dependencies,
                            ARRAY_SIZE( dependencies ), fdd_init_events, NULL,
                            NULL );
}

const char *
fdd_strerror( int error )
{
  if( error >= FDD_LAST_ERROR )
    error = FDD_LAST_ERROR - 1;
  return fdd_error[ error ];
}

/*
 * disk.sides   1  2  1  2  1  2  1  2
 * d->c_head    0  0  1  1  0  0  1  1
 * d->upside    0  0  0  0  1  1  1  1
 *
 * UNREADABLE   0  0  1  0  1  0  0  0
 */

static void
fdd_set_data( fdd_t *d, int fact )
{
  int head = d->upsidedown ? 1 - d->c_head : d->c_head;

  if( !d->loaded )
    return;

  if( d->unreadable || ( d->disk.sides == 1 && head == 1 ) ||
      d->c_cylinder >= d->disk.cylinders ) {
    d->disk.track = NULL;
    d->disk.clocks = NULL;
    d->disk.fm = NULL;
    d->disk.weak = NULL;
    return;
  }

  DISK_SET_TRACK( &d->disk, head, d->c_cylinder );
  d->c_bpt = d->disk.track[-3] + 256 * d->disk.track[-2];
  if( fact > 0 ) {
    /* this generate a bpt/fact +-10% triangular distribution skip in bytes 
       i know, we should use the higher bits of rand(), but we not
       keen on _real_ (pseudo)random numbers... ;)
    */
    d->disk.i += d->c_bpt / fact + d->c_bpt *
		  ( rand() % 10 + rand() % 10 - 9 ) / fact / 100;
    while( d->disk.i >= d->c_bpt )
      d->disk.i -= d->c_bpt;
  }
  d->index = d->disk.i ? 0 : 1;
}

/* initialise fdd */
int
fdd_init( fdd_t *d, fdd_type_t type, const fdd_params_t *dt, int reinit )
{
  int upsidedown = d->upsidedown;
  int loaded = d->loaded;
  int selected = d->selected;
  int do_read_weak = d->do_read_weak;
  if( dt == NULL ) dt = &fdd_params[0];

  d->fdd_heads = d->fdd_cylinders = d->c_head = d->c_cylinder = 0;
  d->upsidedown = d->unreadable = d->loaded = d->auto_geom = d->selected = 0;
  d->dskchg = d->hdout = 0;
  d->ready = 0;
  d->do_read_weak = 0;
  if( type == FDD_TYPE_NONE )
    d->index = d->tr00 = d->wrprot = 0;
  else
    d->index = d->tr00 = d->wrprot = 1;
  d->type = type;

  d->fdc_index = NULL;
  d->fdc = NULL;

  if( dt->heads < 0 || dt->heads > 2 || dt->cylinders < 0 || dt->cylinders > FDD_MAX_TRACK )
    return d->status = FDD_GEOM;

  if( dt->heads == 0 )
    d->auto_geom = 1;
  d->fdd_heads = dt->heads;
  d->fdd_cylinders = dt->cylinders == 80 ? settings_current.drive_80_max_track : settings_current.drive_40_max_track;
  if( reinit ) {
    d->selected = selected;
    d->do_read_weak = do_read_weak;
  } else {
    fdd_unload( d );
  }
  if( reinit && loaded ) {
    fdd_unload( d );
    fdd_load( d, upsidedown );
  }
   else
    d->disk.data = NULL;

  return d->status = FDD_OK;
}

void
fdd_motoron( fdd_t *d, int on )
{
  if( !d->loaded )
    return;
  on = on > 0 ? 1 : 0;
  if( d->motoron == on )
    return;
  d->motoron = on;
  fdd_motor += on ? 1 : -1;
  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK,
                       fdd_motor > 0 ? UI_STATUSBAR_STATE_ACTIVE :
                       UI_STATUSBAR_STATE_INACTIVE );
  /*
  TEAC FD55 Spec:
  (13) READY output signal
    i)	The FDD is powered on.
    ii)	Disk is installed.
    iii)The disk rotates at more than 50% of the rated speed.
    iv) Two index pulses have been counted after item iii) is
	satisfied

    Note: Pre-ready is the state that at least one INDEX
	pulse has been detected after item iii) is satisfied
  */
  event_remove_type_user_data( motor_event, d );		/* remove pending motor-on event for *this* drive */
  if( on ) {
    event_add_with_data( tstates + 4 *			/* 2 revolution: 2 * 200 / 1000 */
			 machine_current->timings.processor_speed / 10,
			 motor_event, d );
    if( d->loaded ) /* index rotating */
      event_add_with_data( tstates + ( d->index_pulse ? 10 : 190 ) *
			 machine_current->timings.processor_speed / 1000,
			 index_event, d );
  } else {
    event_add_with_data( tstates + 3 *			/* 1.5 revolution */
			 machine_current->timings.processor_speed / 10,
			 motor_event, d );
  }
}

void
fdd_head_load( fdd_t *d, int load )
{
  if( !d->loaded )
    return;
  load = load > 0 ? 1 : 0;
  if( d->loadhead == load )
    return;
  d->loadhead = load;
  fdd_set_data( d, FDD_HEAD_FACT );
}

void
fdd_select( fdd_t *d, int select )
{
  d->selected = select > 0 ? 1 : 0;
  /*
      ... Drive Select when activated to a logical
      zero level, will load the R/W head against the
      diskette enabling contact of the R/W head against
      the media. ...
  */
  if( d->type == FDD_SHUGART )
    fdd_head_load( d, d->selected );
}


/* load a disk into fdd */
int
fdd_load( fdd_t *d, int upsidedown )
{
  if( d->type == FDD_TYPE_NONE )
    return d->status = FDD_NONE;

  if( d->disk.sides < 0 || d->disk.sides > 2 ||
      d->disk.cylinders < 0 || d->disk.cylinders > FDD_MAX_TRACK )
    return d->status = FDD_GEOM;

  if( d->auto_geom )
    d->fdd_heads = d->disk.sides;		/* 1 or 2 */
  if( d->auto_geom )
    d->fdd_cylinders = d->disk.cylinders > settings_current.drive_40_max_track ?
				settings_current.drive_80_max_track :
                                settings_current.drive_40_max_track;

  if( d->disk.cylinders > d->fdd_cylinders + FDD_TRACK_TRESHOLD ) {
    d->unreadable = 1;
    ui_error( UI_ERROR_WARNING,
              "This %d track disk image is incompatible with the configured "
              "%d track drive. Use disk options to select a compatible drive.",
              d->disk.cylinders, d->fdd_cylinders );
  }

  d->upsidedown = upsidedown > 0 ? 1 : 0;
  d->wrprot = d->disk.wrprot;		/* write protect */
  d->loaded = 1;
  if( d->type == FDD_SHUGART && d->selected )
    fdd_head_load( d, 1 );

  d->do_read_weak = d->disk.have_weak;
  fdd_set_data( d, FDD_LOAD_FACT );
  d->ready = ( d->motoron && d->loaded );
  if( d->disk.density == DISK_HD ) d->hdout = 1;

  return d->status = FDD_OK;
}

void
fdd_unload( fdd_t *d )
{
  d->ready = d->loaded = d->dskchg = d->hdout = 0;
  d->index = d->wrprot = 1;
  fdd_motoron( d, 0 );
  if( d->type == FDD_SHUGART && d->selected )
    fdd_head_load( d, 0 );
}

/* change current head */
void
fdd_set_head( fdd_t *d, int head )
{
  if( d->fdd_heads == 1 )
    return;

  head = head > 0 ? 1 : 0;
  if( d->c_head == head )
    return;

  d->c_head = head;
  fdd_set_data( d, 0 );
}

/* change current track dir = 1 / -1 */
void
fdd_step( fdd_t *d, fdd_dir_t direction )
{
  if( direction == FDD_STEP_OUT ) {
    if( d->c_cylinder > 0 )
      d->c_cylinder--;
  } else { /* direction == FDD_STEP_IN */
    if( d->c_cylinder < d->fdd_cylinders - 1 )
      d->c_cylinder++;
  }
  if( d->c_cylinder == 0 )
    d->tr00 = 1;
  else
    d->tr00 = 0;

  fdd_set_data( d, FDD_STEP_FACT );
  if( d->loaded && d->selected ) d->dskchg = 1;
}

/* read/write next byte from/to sector */
static int
fdd_read_write_data( fdd_t *d, fdd_write_t write )
{
  if( !d->selected || !d->ready || !d->loadhead || d->disk.track == NULL ) {
    if( d->loaded && d->motoron ) {			/* spin the disk */
      if( d->disk.i >= d->c_bpt ) {		/* next data byte */
        d->disk.i = 0;
      }
      if( !write )
        d->data = 0x100;				/* no data */
      d->disk.i++;
      d->index = d->disk.i >= d->c_bpt ? 1 : 0;
    }
    return d->status = FDD_OK;
  }

  if( d->disk.i >= d->c_bpt ) {		/* next data byte */
    d->disk.i = 0;
  }
  if( write ) {
    if( d->disk.wrprot ) {
      d->disk.i++;
      d->index = d->disk.i >= d->c_bpt ? 1 : 0;
      return d->status = FDD_RDONLY;
    }
    d->disk.track[ d->disk.i ] = d->data & 0x00ff;
    if( d->data & 0xff00 )
      bitmap_set( d->disk.clocks, d->disk.i );
    else
      bitmap_reset( d->disk.clocks, d->disk.i );

    if( d->marks & 0x01 )
      bitmap_set( d->disk.fm, d->disk.i );
    else
      bitmap_reset( d->disk.fm, d->disk.i );
#if 0		/* hmm... we cannot write weak data with 'standard' hardware */
    if( d->marks & 0x02 )
      bitmap_set( d->disk.weak, d->disk.i );
    else
      bitmap_reset( d->disk.weak, d->disk.i );
#else
    bitmap_reset( d->disk.weak, d->disk.i );
#endif
    d->disk.dirty = 1;
  } else {	/* read */
    d->data = d->disk.track[ d->disk.i ];
    if( bitmap_test( d->disk.clocks, d->disk.i ) )
      d->data |= 0xff00;
    d->marks = 0;
    if( bitmap_test( d->disk.fm, d->disk.i ) )
      d->marks |= 0x01;
    if( bitmap_test( d->disk.weak, d->disk.i ) ) {
      d->marks |= 0x02;
      /* mess up data byte */
      d->data &= rand() % 0xff, d->data |= rand() % 0xff;
    }
  }
  d->disk.i++;
  d->index = d->disk.i >= d->c_bpt ? 1 : 0;

  return d->status = FDD_OK;
}

/* read next byte from sector */
int
fdd_read_data( fdd_t *d )
{
  return fdd_read_write_data( d, FDD_READ );
}

/* write next byte to sector */
int
fdd_write_data( fdd_t *d )
{
  return fdd_read_write_data( d, FDD_WRITE );
}

void fdd_flip( fdd_t *d, int upsidedown )
{
  if( !d->loaded )
    return;

  d->upsidedown = upsidedown > 0 ? 1 : 0;
  fdd_set_data( d, FDD_LOAD_FACT );
}

void
fdd_wrprot( fdd_t *d, int wrprot )
{
  if( !d->loaded )
    return;

  d->wrprot = d->disk.wrprot = wrprot;
}

void
fdd_wait_index_hole( fdd_t *d )
{
  if( !d->selected || !d->ready )
    return;

  d->disk.i = 0;
  d->index = 1;
}

static void
fdd_event( libspectrum_dword last_tstates, int event,
           void *user_data ) 
{
  fdd_t *d = user_data;

  if( event == motor_event ) {
    d->ready = ( d->motoron & d->loaded );	/* 0x01 & 0x01 */
    return;
  }

  d->index_pulse = !d->index_pulse;
  if( !d->index_pulse && d->fdc ) { /* if d->fdc != NULL fdc wait for index */
      d->fdc_index( d->fdc );
      d->fdc = NULL;
  }

  if( d->motoron & d->loaded ) /* keep rotating */
    event_add_with_data( last_tstates + ( d->index_pulse ? 10 : 190 ) *
			 machine_current->timings.processor_speed / 1000,
			 index_event, d );
}
