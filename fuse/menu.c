/* menu.c: general menu callbacks
   Copyright (c) 2004-2017 Philip Kendall, Alex Badea, Sergio Baldoví,
     Stuart Brady, Gergely Szasz, Stefano Bodrato, Fredrick Meunier

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

   Philip Kendall <philip-fuse@shadowmagic.org.uk>

*/

#include <config.h>

#include <libspectrum.h>

#include "event.h"
#include "fuse.h"
#include "menu.h"
#include "movie.h"
#include "machines/specplus3.h"
#include "peripherals/dck.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/disciple.h"
#include "peripherals/disk/opus.h"
#include "peripherals/disk/plusd.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/ide/simpleide.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/ide/zxmmc.h"
#include "peripherals/if1.h"
#include "peripherals/if2.h"
#include "peripherals/joystick.h"
#include "peripherals/multiface.h"
#include "peripherals/scld.h"
#include "profile.h"
#include "psg.h"
#include "rzx.h"
#include "screenshot.h"
#include "settings.h"
#include "snapshot.h"
#include "svg.h"
#include "tape.h"
#include "ui/scaler/scaler.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "utils.h"
#include "z80/z80.h"

static int menu_select_machine_roms( libspectrum_machine machine, size_t start,
				     size_t n );

static int menu_select_peripheral_roms( const char *peripheral_name,
					size_t start, size_t n );

MENU_CALLBACK( menu_file_open )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Open Spectrum File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  utils_open_file( filename, tape_can_autoload(), NULL );

  libspectrum_free( filename );

  display_refresh_all();

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_insertsnapshot )
{
  libspectrum_snap *snap;
  libspectrum_error error;

  if( !rzx_recording ) return;

  ui_widget_finish();

  libspectrum_rzx_stop_input( rzx );

  snap = libspectrum_snap_alloc();

  error = snapshot_copy_to( snap );
  if( error ) { libspectrum_snap_free( snap ); return; }

  libspectrum_rzx_add_snap( rzx, snap, 0 );

  libspectrum_rzx_start_input( rzx, tstates );
}

MENU_CALLBACK( menu_file_recording_rollback )
{
  libspectrum_error error;
  
  if( !rzx_recording ) return;

  ui_widget_finish();

  fuse_emulation_pause();

  error = rzx_rollback();
  if( error ) { fuse_emulation_unpause(); return; }

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_rollbackto )
{
  libspectrum_error error;

  if( !rzx_recording ) return;

  ui_widget_finish();

  fuse_emulation_pause();

  error = rzx_rollback_to();
  if( error ) { fuse_emulation_unpause(); return; }

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_play )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = ui_get_open_filename( "Fuse - Start Replay" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_playback( recording, 1 );

  libspectrum_free( recording );

  display_refresh_all();

  if( rzx_playback ) ui_menu_activate( UI_MENU_ITEM_RECORDING, 1 );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_stop )
{
  if( !( rzx_recording || rzx_playback ) ) return;

  ui_widget_finish();

  if( rzx_recording ) rzx_stop_recording();
  if( rzx_playback  ) rzx_stop_playback( 1 );
}  

MENU_CALLBACK( menu_file_recording_finalise )
{
  char *rzx_filename;
  int error;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  rzx_filename = ui_get_open_filename( "Fuse - Finalise Recording" );
  if( !rzx_filename ) { fuse_emulation_unpause(); return; }

  error = rzx_finalise_recording( rzx_filename );

  if( error == LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_INFO, "RZX file finalised" );
  } else {
    ui_error( UI_ERROR_WARNING, "RZX file cannot be finalised" );
  }

  libspectrum_free( rzx_filename );

  fuse_emulation_unpause();
}  

MENU_CALLBACK( menu_file_aylogging_stop )
{
  if ( !psg_recording ) return;

  ui_widget_finish();

  psg_stop_recording();
  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 0 );
}

MENU_CALLBACK( menu_file_screenshot_openscrscreenshot )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Open SCR Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_read( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_screenshot_openmltscreenshot )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Open MLT Screenshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_mlt_read( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_movie_stop )
{
  ui_widget_finish();

  movie_stop();
}

MENU_CALLBACK( menu_file_movie_pause )
{
  ui_widget_finish();

  movie_pause();
}

MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_machine_select )
{
  switch( action ) {

  case  1: menu_select_machine_roms( LIBSPECTRUM_MACHINE_16,        0, 1 ); return;
  case  2: menu_select_machine_roms( LIBSPECTRUM_MACHINE_48,        1, 1 ); return;
  case  3: menu_select_machine_roms( LIBSPECTRUM_MACHINE_128,       2, 2 ); return;
  case  4: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PLUS2,     4, 2 ); return;
  case  5: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PLUS2A,    6, 4 ); return;
  case  6: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PLUS3,    10, 4 ); return;
  case  7: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PLUS3E,   14, 4 ); return;
  case  8: menu_select_machine_roms( LIBSPECTRUM_MACHINE_TC2048,   18, 1 ); return;
  case  9: menu_select_machine_roms( LIBSPECTRUM_MACHINE_TC2068,   19, 2 ); return;
  case 10: menu_select_machine_roms( LIBSPECTRUM_MACHINE_TS2068,   21, 2 ); return;
  case 11: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PENT,     23, 3 ); return;
  case 12: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PENT512,  26, 4 ); return;
  case 13: menu_select_machine_roms( LIBSPECTRUM_MACHINE_PENT1024, 30, 4 ); return;
  case 14: menu_select_machine_roms( LIBSPECTRUM_MACHINE_SCORP,    34, 4 ); return;
  case 15: menu_select_machine_roms( LIBSPECTRUM_MACHINE_SE,       38, 2 ); return;

  }

  ui_error( UI_ERROR_ERROR,
	    "menu_options_selectroms_machine_select: unknown action %d", action );
  fuse_abort();
}

MENU_CALLBACK_WITH_ACTION( menu_options_selectroms_peripheral_select )
{
  switch( action ) {

  case  1: menu_select_peripheral_roms( "Interface 1",     0, 1 ); return;
  case  2: menu_select_peripheral_roms( "Beta 128",        1, 1 ); return;
  case  3: menu_select_peripheral_roms( "+D",              2, 1 ); return;
  case  4: menu_select_peripheral_roms( "Didaktik 80",     3, 1 ); return;
  case  5: menu_select_peripheral_roms( "DISCiPLE",        4, 1 ); return;
  case  6: menu_select_peripheral_roms( "Multiface One",   5, 1 ); return;
  case  7: menu_select_peripheral_roms( "Multiface 128",   6, 1 ); return;
  case  8: menu_select_peripheral_roms( "Multiface 3",     7, 1 ); return;
  case  9: menu_select_peripheral_roms( "Opus Discovery",  8, 1 ); return;
  case 10: menu_select_peripheral_roms( "SpeccyBoot",      9, 1 ); return;
  case 11: menu_select_peripheral_roms( "uSource",        10, 1 ); return;

  }

  ui_error( UI_ERROR_ERROR,
	    "menu_options_selectroms_peripheral_select: unknown action %d", action );
  fuse_abort();
}

MENU_CALLBACK( menu_options_filter )
{
  scaler_type scaler;

  /* Stop emulation */
  fuse_emulation_pause();

  scaler = menu_get_scaler( scaler_is_supported );
  if( scaler != SCALER_NUM && scaler != current_scaler )
    scaler_select_scaler( scaler );

  /* Carry on with emulation again */
  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_options_fullscreen )
{
  ui_widget_finish();
  settings_current.full_screen = !settings_current.full_screen;
}

MENU_CALLBACK( menu_options_save )
{
  ui_widget_finish();
  settings_write_config( &settings_current );
}

MENU_CALLBACK( menu_machine_profiler_start )
{
  ui_widget_finish();
  profile_start();
}

MENU_CALLBACK( menu_machine_profiler_stop )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Save Profile Data" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  profile_finish( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_machine_nmi )
{
  ui_widget_finish();
  event_add( 0, z80_nmi_event );
}

MENU_CALLBACK( menu_machine_multifaceredbutton )
{
  ui_widget_finish();
  multiface_red_button();
}

MENU_CALLBACK( menu_media_tape_open )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Open Tape" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  tape_open( filename, 0 );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_tape_play )
{
  ui_widget_finish();
  tape_toggle_play( 0 );
}

MENU_CALLBACK( menu_media_tape_rewind )
{
  ui_widget_finish();
  tape_rewind();
}

MENU_CALLBACK( menu_media_tape_clear )
{
  ui_widget_finish();
  tape_close();
}

MENU_CALLBACK( menu_media_tape_write )
{
  ui_tape_write();
}

MENU_CALLBACK( menu_media_tape_recordstart )
{
  ui_widget_finish();
  tape_record_start();
}

MENU_CALLBACK( menu_media_tape_recordstop )
{
  ui_widget_finish();
  tape_record_stop();
}

MENU_CALLBACK_WITH_ACTION( menu_media_if1_rs232 )
{
  char *filename;

  fuse_emulation_pause();

  if( action & 0xf0 ) {
    ui_widget_finish();
    if1_unplug( action & 0x0f );
  } else {
    filename = ui_get_open_filename( "Fuse - Select File for Communication" );
    if( !filename ) { fuse_emulation_unpause(); return; }

    if1_plug( filename, action );

    libspectrum_free( filename );
  }
  fuse_emulation_unpause();

}

MENU_CALLBACK_WITH_ACTION( menu_media_insert_new )
{
  int which, type;
  ui_media_drive_info_t *drive;
  
  ui_widget_finish();

  action--;
  which = action & 0x0f;
  type = ( action & 0xf0 ) >> 4;

  switch( type ) {
  case 3:
    if1_mdr_insert( which, NULL );
    break;
  default:
    drive = ui_media_drive_find( type, which );
    if( !drive )
      return;
    ui_media_drive_insert( drive, NULL, 0 );
    break;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_insert )
{
  char *filename;
  char title[80];
  int which, type;
  ui_media_drive_info_t *drive = NULL;
  
  action--;
  which = action & 0x0f;
  type = ( action & 0xf0 ) >> 4;

  fuse_emulation_pause();

  switch( type ) {
  case 3:
    snprintf( title, 80, "Fuse - Insert Microdrive Cartridge %i", which + 1 );
    break;
  default:
    drive = ui_media_drive_find( type, which );
    if( !drive )
      return;
    snprintf( title, sizeof(title), "Fuse - Insert %s", drive->name );
    break;
  }
  filename = ui_get_open_filename( title );
  if( !filename ) { fuse_emulation_unpause(); return; }

  switch( type ) {
  case 3:
    if1_mdr_insert( which, filename );
    break;
  default:
    ui_media_drive_insert( drive, filename, 0 );
    break;
  }

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_eject )
{
  int which, type;

  ui_widget_finish();

  action--;
  which = action & 0x00f;
  type = ( action & 0x0f0 ) >> 4;

  switch( type ) {
  case 3:
    if1_mdr_eject( which );
    break;
  default:
    ui_media_drive_eject( type, which );
    break;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_save )
{
  int which, saveas, type;

  ui_widget_finish();

  action--;
  which = action & 0x00f;
  type = ( action & 0x0f0 ) >> 4;
  saveas = ( action & 0xf00 ) >> 8;

  switch( type ) {
  case 3:
    if1_mdr_save( which, saveas );
    break;
  default:
    ui_media_drive_save( type, which, saveas );
    break;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_flip )
{
  int which, type, flip;
  
  ui_widget_finish();

  action--;
  which = action & 0x0f;
  type = ( action & 0xf0 ) >> 4;
  flip = !!( action & 0x100 );

  switch( type ) {
  /* No flip option for IF1 */
  default:
    ui_media_drive_flip( type, which, flip );
    break;
  }
}

MENU_CALLBACK_WITH_ACTION( menu_media_writeprotect )
{
  int which, wrprot, type;

  ui_widget_finish();

  action--;
  which = action & 0x00f;
  type = ( action & 0x0f0 ) >> 4;
  wrprot = !!( action & 0x100 );

  switch( type ) {
  case 3:
    if1_mdr_writeprotect( which, wrprot );
    break;
  default:
    ui_media_drive_writeprotect( type, which, wrprot );
    break;
  }

}

MENU_CALLBACK( menu_media_cartridge_timexdock_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Insert Timex Dock Cartridge" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  dck_insert( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_cartridge_timexdock_eject )
{
  ui_widget_finish();
  dck_eject();
}

MENU_CALLBACK( menu_media_cartridge_interface2_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Insert Interface 2 Cartridge" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  if2_insert( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_media_cartridge_interface2_eject )
{
  ui_widget_finish();
  if2_eject();
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_insert )
{
  char *filename;

  fuse_emulation_pause();

  filename = ui_get_open_filename( "Fuse - Insert Hard Disk File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  switch( action ) {
  case 1: simpleide_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_insert( filename ); break;
  case 6: divide_insert( filename, LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_insert( filename, LIBSPECTRUM_IDE_SLAVE  ); break;
  case 8: divmmc_insert( filename ); break;
  case 9: zxmmc_insert( filename ); break;
  }

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_commit )
{
  fuse_emulation_pause();

  switch( action ) {
  case 1: simpleide_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_commit(); break;
  case 6: divide_commit( LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_commit( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 8: divmmc_commit(); break;
  case 9: zxmmc_commit(); break;
  }

  fuse_emulation_unpause();

  ui_widget_finish();
}

MENU_CALLBACK_WITH_ACTION( menu_media_ide_eject )
{
  fuse_emulation_pause();

  switch( action ) {
  case 1: simpleide_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 2: simpleide_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 3: zxatasp_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 4: zxatasp_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 5: zxcf_eject(); break;
  case 6: divide_eject( LIBSPECTRUM_IDE_MASTER ); break;
  case 7: divide_eject( LIBSPECTRUM_IDE_SLAVE  ); break;
  case 8: divmmc_eject(); break;
  }

  fuse_emulation_unpause();

  ui_widget_finish();
}

MENU_CALLBACK( menu_file_savesnapshot )
{
  char *filename;

  ui_widget_finish();

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Save Snapshot" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  snapshot_write( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_screenshot_savescreenasscr )
{
  char *filename;

  ui_widget_finish();

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Save Screenshot as SCR" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_scr_write( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_screenshot_savescreenasmlt )
{
  char *filename;

  ui_widget_finish();

  fuse_emulation_pause();

  if( machine_current->timex && scld_last_dec.name.hires ) {
    ui_error( UI_ERROR_ERROR,
              "MLT format not supported for Timex hi-res screen mode" );
    fuse_emulation_unpause();
    return;
  }

  filename = ui_get_save_filename( "Fuse - Save Screenshot as MLT" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_mlt_write( filename );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

#ifdef USE_LIBPNG

MENU_CALLBACK( menu_file_screenshot_savescreenaspng )
{
  scaler_type scaler;
  char *filename;

  ui_widget_finish();

  fuse_emulation_pause();

  scaler = menu_get_scaler( screenshot_available_scalers );
  if( scaler == SCALER_NUM ) {
    fuse_emulation_unpause();
    return;
  }

  filename =
    ui_get_save_filename( "Fuse - Save Screenshot as PNG" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  screenshot_write( filename, scaler );

  libspectrum_free( filename );

  fuse_emulation_unpause();
}

#endif

#ifdef HAVE_LIB_XML2

MENU_CALLBACK( menu_file_scalablevectorgraphics_startcaptureinlinemode )
{
  char *filename;

  ui_widget_finish();
  if( !svg_capture_active ) {

    fuse_emulation_pause();

    filename = ui_get_save_filename( "Fuse - Capture to SVG File" );
    if( !filename ) { fuse_emulation_unpause(); return; }

    ui_menu_activate( UI_MENU_ITEM_FILE_SVG_CAPTURE, 1 );
    svg_startcapture( filename, SVG_CAPTURE_LINES );

    fuse_emulation_unpause();
  }
}

MENU_CALLBACK( menu_file_scalablevectorgraphics_startcaptureindotmode )
{
  char *filename;

  ui_widget_finish();
  if( !svg_capture_active ) {

    fuse_emulation_pause();

    filename = ui_get_save_filename( "Fuse - Capture to SVG File" );
    if( !filename ) { fuse_emulation_unpause(); return; }
    ui_menu_activate( UI_MENU_ITEM_FILE_SVG_CAPTURE, 1 );

    svg_startcapture( filename, SVG_CAPTURE_DOTS );

    fuse_emulation_unpause();
  }
}

MENU_CALLBACK( menu_file_scalablevectorgraphics_stopcapture )
{
  ui_widget_finish();
  if( svg_capture_active ) {

    fuse_emulation_pause();

    svg_stopcapture();
    ui_menu_activate( UI_MENU_ITEM_FILE_SVG_CAPTURE, 0 );

    fuse_emulation_unpause();
  }
}

#endif  /* HAVE_LIB_XML2 */


MENU_CALLBACK( menu_file_movie_record )
{
  char *filename;

  ui_widget_finish();

  fuse_emulation_pause();

  filename = ui_get_save_filename( "Fuse - Record Movie File" );
  if( !filename ) { fuse_emulation_unpause(); return; }

  movie_start( filename );
  libspectrum_free( filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_movie_record_recordfromrzx )
{
  char *rzx_file, *fmf_file;

  ui_widget_finish();

  if( rzx_playback || rzx_recording || movie_recording ) return;

  fuse_emulation_pause();

  rzx_file = ui_get_open_filename( "Fuse - Load RZX" );
  if( !rzx_file ) { fuse_emulation_unpause(); return; }

  rzx_start_playback( rzx_file, 1 );
  libspectrum_free( rzx_file );
  display_refresh_all();

  if( rzx_playback ) {
    fmf_file = ui_get_save_filename( "Fuse - Record Movie File" );
    if( !fmf_file ) { 
      rzx_stop_playback( 1 );
      fuse_emulation_unpause();
      return;
    }

    movie_start( fmf_file );
    libspectrum_free( fmf_file );
    ui_menu_activate( UI_MENU_ITEM_RECORDING, 1 );
  }

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_record )
{
  char *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  recording = ui_get_save_filename( "Fuse - Start Recording" );
  if( !recording ) { fuse_emulation_unpause(); return; }

  rzx_start_recording( recording, 1 );

  libspectrum_free( recording );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_recordfromsnapshot )
{
  char *snap, *recording;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  snap = ui_get_open_filename( "Fuse - Load Snapshot " );
  if( !snap ) { fuse_emulation_unpause(); return; }

  recording = ui_get_save_filename( "Fuse - Start Recording" );
  if( !recording ) {
    libspectrum_free( snap );
    fuse_emulation_unpause();
    return;
  }

  if( snapshot_read( snap ) ) {
    libspectrum_free( snap );
    libspectrum_free( recording );
    fuse_emulation_unpause();
    return;
  }

  rzx_start_recording( recording, settings_current.embed_snapshot );

  libspectrum_free( recording );

  display_refresh_all();

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_recording_continuerecording )
{
  char *rzx_filename;
  int error;

  if( rzx_playback || rzx_recording ) return;

  fuse_emulation_pause();

  rzx_filename = ui_get_open_filename( "Fuse - Continue Recording" );
  if( !rzx_filename ) { fuse_emulation_unpause(); return; }

  error = rzx_continue_recording( rzx_filename );

  if( error != LIBSPECTRUM_ERROR_NONE ) {
    ui_error( UI_ERROR_WARNING, "RZX file cannot be continued" );
  }

  libspectrum_free( rzx_filename );

  fuse_emulation_unpause();
}

MENU_CALLBACK( menu_file_aylogging_record )
{
  char *psgfile;

  if( psg_recording ) return;

  fuse_emulation_pause();

  psgfile = ui_get_save_filename( "Fuse - Start AY Log" );
  if( !psgfile ) { fuse_emulation_unpause(); return; }

  psg_start_recording( psgfile );

  libspectrum_free( psgfile );

  display_refresh_all();

  ui_menu_activate( UI_MENU_ITEM_AY_LOGGING, 1 );

  fuse_emulation_unpause();
}

int
menu_check_media_changed( void )
{
  int confirm, i;

  confirm = tape_close(); if( confirm ) return 1;

  confirm = ui_media_drive_eject_all();
  if( confirm ) return 1;

  for( i = 0; i < 8; i++ ) {
    confirm = if1_mdr_eject( i );
    if( confirm ) return 1;
  }

  if( settings_current.simpleide_master_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.simpleide_slave_file ) {
    confirm = simpleide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_master_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.zxatasp_slave_file ) {
    confirm = zxatasp_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.zxcf_pri_file ) {
    confirm = zxcf_eject(); if( confirm ) return 1;
  }

  if( settings_current.divide_master_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_MASTER );
    if( confirm ) return 1;
  }

  if( settings_current.divide_slave_file ) {
    confirm = divide_eject( LIBSPECTRUM_IDE_SLAVE );
    if( confirm ) return 1;
  }

  if( settings_current.divmmc_file ) {
    confirm = divmmc_eject();
    if( confirm ) return 1;
  }

  if( settings_current.zxmmc_file ) {
    confirm = zxmmc_eject();
    if( confirm ) return 1;
  }

  return 0;
}

static int
menu_select_machine_roms( libspectrum_machine machine, size_t start, size_t n )
{
  return menu_select_roms_with_title( libspectrum_machine_name( machine ),
				      start, n, 0 );
}

static int
menu_select_peripheral_roms( const char *peripheral_name, size_t start, size_t n )
{
  return menu_select_roms_with_title( peripheral_name,
				      start, n, 1 );
}

const char*
menu_machine_detail( void )
{
  return libspectrum_machine_name( machine_current->machine );
}

const char*
menu_filter_detail( void )
{
  return scaler_name(current_scaler);
}

const char*
menu_keyboard_joystick_detail( void )
{
  return joystick_name[ settings_current.joystick_keyboard_output ];
}

const char*
menu_joystick_1_detail( void )
{
  return joystick_name[ settings_current.joystick_1_output ];
}

const char*
menu_joystick_2_detail( void )
{
  return joystick_name[ settings_current.joystick_2_output ];
}

const char*
menu_tape_detail( void )
{
  if( !tape_present() ) return "Not inserted";

  if( tape_is_playing() ) return "Playing";
  else return "Stopped";
}

static const char * const disk_detail_str[] = {
  "Inserted",
  "Inserted WP",
  "Inserted UD",
  "Inserted WP,UD",

  "*Inserted",
  "*Inserted WP",
  "*Inserted UD",
  "*Inserted WP,UD",
  "Not inserted",
};

static const char*
menu_disk_detail( fdd_t *f )
{
  int i = 0;

  if( !f->loaded ) return disk_detail_str[8];
  if( f->wrprot ) i = 1;
  if( f->upsidedown ) i += 2;
  if( f->disk.dirty ) i += 4;
  return disk_detail_str[i];
}

const char*
menu_plus3a_detail( void )
{
  fdd_t *f = specplus3_get_fdd( SPECPLUS3_DRIVE_A );

  return menu_disk_detail( f );
}

const char*
menu_plus3b_detail( void )
{
  fdd_t *f = specplus3_get_fdd( SPECPLUS3_DRIVE_B );

  return menu_disk_detail( f );
}

const char*
menu_beta128a_detail( void )
{
  fdd_t *f = beta_get_fdd( BETA_DRIVE_A );

  return menu_disk_detail( f );
}

const char*
menu_beta128b_detail( void )
{
  fdd_t *f = beta_get_fdd( BETA_DRIVE_B );

  return menu_disk_detail( f );
}

const char*
menu_beta128c_detail( void )
{
  fdd_t *f = beta_get_fdd( BETA_DRIVE_C );

  return menu_disk_detail( f );
}

const char*
menu_beta128d_detail( void )
{
  fdd_t *f = beta_get_fdd( BETA_DRIVE_D );

  return menu_disk_detail( f );
}

const char*
menu_opus1_detail( void )
{
  fdd_t *f = opus_get_fdd( OPUS_DRIVE_1 );

  return menu_disk_detail( f );
}

const char*
menu_opus2_detail( void )
{
  fdd_t *f = opus_get_fdd( OPUS_DRIVE_2 );

  return menu_disk_detail( f );
}

const char*
menu_plusd1_detail( void )
{
  fdd_t *f = plusd_get_fdd( PLUSD_DRIVE_1 );

  return menu_disk_detail( f );
}

const char*
menu_plusd2_detail( void )
{
  fdd_t *f = plusd_get_fdd( PLUSD_DRIVE_2 );

  return menu_disk_detail( f );
}

const char*
menu_didaktik_a_detail( void )
{
  fdd_t *f = didaktik80_get_fdd( DIDAKTIK80_DRIVE_A );

  return menu_disk_detail( f );
}

const char*
menu_didaktik_b_detail( void )
{
  fdd_t *f = didaktik80_get_fdd( DIDAKTIK80_DRIVE_B );

  return menu_disk_detail( f );
}

const char*
menu_disciple1_detail( void )
{
  fdd_t *f = disciple_get_fdd( DISCIPLE_DRIVE_1 );

  return menu_disk_detail( f );
}

const char*
menu_disciple2_detail( void )
{
  fdd_t *f = disciple_get_fdd( DISCIPLE_DRIVE_2 );

  return menu_disk_detail( f );
}

MENU_CALLBACK( menu_machine_didaktiksnap )
{
  ui_widget_finish();
  didaktik80_snap = 1;
  event_add( 0, z80_nmi_event );
}
