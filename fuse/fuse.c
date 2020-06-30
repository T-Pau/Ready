/* fuse.c: The Free Unix Spectrum Emulator
   Copyright (c) 1999-2017 Philip Kendall and others

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

#include <config.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif				/* #ifdef WIN32 */

#include <unistd.h>

/* We need to include SDL.h on Mac O X and Windows to do some magic
   bootstrapping by redefining main. As we now allow SDL joystick code to be
   used in the GTK+ and Xlib UIs we need to also do the magic when that code is
   in use, feel free to look away for the next line */
#if defined UI_SDL || (defined USE_JOYSTICK && !defined HAVE_JSW_H && (defined UI_X || defined UI_GTK) )
#include <SDL.h>		/* Needed on MacOS X and Windows */
#endif /* #if defined UI_SDL || (defined USE_JOYSTICK && !defined HAVE_JSW_H && (defined UI_X || defined UI_GTK) ) */

#ifdef GEKKO
#include <fat.h>
#endif				/* #ifdef GEKKO */

#ifdef HAVE_LIB_XML2
#include <libxml/encoding.h>
#endif

#include "debugger/debugger.h"
#include "display.h"
#include "event.h"
#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "keyboard.h"
#include "machine.h"
#include "machines/machines_periph.h"
#include "memory_pages.h"
#include "module.h"
#include "movie.h"
#include "mempool.h"
#include "peripherals/ay.h"
#include "peripherals/dck.h"
#include "peripherals/disk/beta.h"
#include "peripherals/disk/didaktik.h"
#include "peripherals/disk/fdd.h"
#include "peripherals/fuller.h"
#include "peripherals/ide/divide.h"
#include "peripherals/ide/divmmc.h"
#include "peripherals/ide/simpleide.h"
#include "peripherals/ide/zxatasp.h"
#include "peripherals/ide/zxcf.h"
#include "peripherals/ide/zxmmc.h"
#include "peripherals/joystick.h"
#include "peripherals/if1.h"
#include "peripherals/if2.h"
#include "peripherals/kempmouse.h"
#include "peripherals/melodik.h"
#include "peripherals/multiface.h"
#include "peripherals/printer.h"
#include "peripherals/scld.h"
#include "peripherals/speccyboot.h"
#include "peripherals/spectranet.h"
#include "peripherals/ula.h"
#include "peripherals/usource.h"
#include "phantom_typist.h"
#include "pokefinder/pokemem.h"
#include "profile.h"
#include "psg.h"
#include "rzx.h"
#include "settings.h"
#include "slt.h"
#include "snapshot.h"
#include "sound.h"
#include "spectrum.h"
#include "tape.h"
#include "timer/timer.h"
#include "ui/scaler/scaler.h"
#include "ui/ui.h"
#include "ui/uimedia.h"
#include "unittests/unittests.h"
#include "utils.h"

#include "z80/z80.h"

/* What name were we called under? */
const char *fuse_progname;

/* A flag to say when we want to exit the emulator */
int fuse_exiting;

/* Is Spectrum emulation currently paused, and if so, how many times? */
int fuse_emulation_paused;

/* The creator information we'll store in file formats that support this */
libspectrum_creator *fuse_creator;

/* The earliest version of libspectrum we need */
static const char * const LIBSPECTRUM_MIN_VERSION = "0.5.0";

/* The various types of file we may want to run on startup */
typedef struct start_files_t {

  const char *disk_plus3;
  const char *disk_opus;
  const char *disk_plusd;
  const char *disk_beta;
  const char *disk_didaktik80;
  const char *disk_disciple;
  const char *dock;
  const char *if2;
  const char *playback;
  const char *recording;
  const char *snapshot;
  const char *tape;

  const char *simpleide_master, *simpleide_slave;
  const char *zxatasp_master, *zxatasp_slave;
  const char *zxcf;
  const char *divide_master, *divide_slave;
  const char *divmmc;
  const char *zxmmc;
  const char *mdr[8];

} start_files_t;

/* Context for the display startup routine */
static display_startup_context display_context;

static int fuse_init(int argc, char **argv);

static void creator_register_startup( void );

static void fuse_show_copyright(void);
static void fuse_show_version( void );
static void fuse_show_help( void );

static int setup_start_files( start_files_t *start_files );
static int parse_nonoption_args( int argc, char **argv, int first_arg,
				 start_files_t *start_files );
static int do_start_files( start_files_t *start_files );

static int fuse_end(void);

#ifdef UI_WIN32
int fuse_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  int r;

#ifdef WIN32
  SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
#endif

#ifdef GEKKO
  fatInitDefault();
#endif				/* #ifdef GEKKO */
  
  if(fuse_init(argc,argv)) {
    fprintf(stderr,"%s: error initialising -- giving up!\n", fuse_progname);
    return 1;
  }

  if( settings_current.show_help ||
      settings_current.show_version ) return 0;

  if( settings_current.unittests ) {
    r = unittests_run();
  } else {
    while( !fuse_exiting ) {
      z80_do_opcodes();
      event_do_events();
    }
    r = debugger_get_exit_code();
  }

  fuse_end();
  
  return r;
}

static int
fuse_libspectrum_init( void *context )
{
  if( libspectrum_check_version( LIBSPECTRUM_MIN_VERSION ) ) {
    if( libspectrum_init() ) return 1;
  } else {
    ui_error( UI_ERROR_ERROR,
              "libspectrum version %s found, but %s required",
	      libspectrum_version(), LIBSPECTRUM_MIN_VERSION );
    return 1;
  }

  return 0;
}

static void
libspectrum_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DISPLAY
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_LIBSPECTRUM, dependencies,
                            ARRAY_SIZE( dependencies ), fuse_libspectrum_init,
                            NULL, NULL );
}

static int
libxml2_init( void *context )
{
#ifdef HAVE_LIB_XML2
  LIBXML_TEST_VERSION
#endif

  return 0;
}

static void
libxml2_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_LIBXML2, dependencies,
                            ARRAY_SIZE( dependencies ), libxml2_init, NULL,
                            NULL );
}

static int
setuid_init( void *context )
{
#ifdef HAVE_GETEUID
  int error;

  /* Drop root privs if we have them */
  if( !geteuid() ) {
    error = setuid( getuid() );
    if( error ) {
      ui_error( UI_ERROR_ERROR, "Could not drop root privileges" );
      return 1;
    }
  }
#endif				/* #ifdef HAVE_GETEUID */

  return 0;
}

static void
setuid_register_startup()
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_DISPLAY,
    STARTUP_MANAGER_MODULE_LIBSPECTRUM,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_SETUID, dependencies,
                            ARRAY_SIZE( dependencies ), setuid_init, NULL,
                            NULL );
}

static int
run_startup_manager( int *argc, char ***argv )
{
  startup_manager_init();

  display_context.argc = argc;
  display_context.argv = argv;

  /* Get every module to register its init function */
  ay_register_startup();
  beta_register_startup();
  creator_register_startup();
  covox_register_startup();
  debugger_register_startup();
  didaktik80_register_startup();
  disciple_register_startup();
  display_register_startup( &display_context );
  divide_register_startup();
  divmmc_register_startup();
  event_register_startup();
  fdd_register_startup();
  fuller_register_startup();
  if1_register_startup();
  if2_register_startup();
  joystick_register_startup();
  kempmouse_register_startup();
  keyboard_register_startup();
  libspectrum_register_startup();
  libxml2_register_startup();
  machine_register_startup();
  machines_periph_register_startup();
  melodik_register_startup();
  memory_register_startup();
  mempool_register_startup();
  multiface_register_startup();
  opus_register_startup();
  phantom_typist_register_startup();
  plusd_register_startup();
  printer_register_startup();
  profile_register_startup();
  psg_register_startup();
  rzx_register_startup();
  scld_register_startup();
  settings_register_startup();
  setuid_register_startup();
  simpleide_register_startup();
  slt_register_startup();
  sound_register_startup();
  speccyboot_register_startup();
  specdrum_register_startup();
  spectranet_register_startup();
  spectrum_register_startup();
  tape_register_startup();
  timer_register_startup();
  ula_register_startup();
  usource_register_startup();
  z80_register_startup();
  zxatasp_register_startup();
  zxcf_register_startup();
  zxmmc_register_startup();

  return startup_manager_run();
}

static int fuse_init(int argc, char **argv)
{
  int error, first_arg;
  char *start_scaler;
  start_files_t start_files;

  /* Seed the bad but widely-available random number
     generator with the current time */
  srand( (unsigned)time( NULL ) );

  /* Some platforms (e.g. Wii) do not have argc/argv */
  if(argc > 0)
    fuse_progname = argv[0];
  else
    fuse_progname = "fuse";
  
  libspectrum_error_function = ui_libspectrum_error;

#ifdef GEKKO
  /* On the Wii, init the display first so we have a way of outputting
     messages */
  if( display_init(&argc,&argv) ) return 1;
#endif

  if( settings_init( &first_arg, argc, argv ) ) return 1;

  if( settings_current.show_version ) {
    fuse_show_version();
    return 0;
  } else if( settings_current.show_help ) {
    fuse_show_help();
    return 0;
  }

  start_scaler = utils_safe_strdup( settings_current.start_scaler_mode );

  fuse_show_copyright();

  if( run_startup_manager( &argc, &argv ) ) return 1;

  error = machine_select_id( settings_current.start_machine );
  if( error ) return error;

  error = scaler_select_id( start_scaler ); libspectrum_free( start_scaler );
  if( error ) return error;

  if( setup_start_files( &start_files ) ) return 1;
  if( parse_nonoption_args( argc, argv, first_arg, &start_files ) ) return 1;
  if( do_start_files( &start_files ) ) return 1;

  /* Must do this after all subsytems are initialised */
  debugger_command_evaluate( settings_current.debugger_command );

  if( ui_mouse_present ) ui_mouse_grabbed = ui_mouse_grab( 1 );

  fuse_emulation_paused = 0;
  movie_init();

  return 0;
}

static int
creator_init( void *context )
{
  size_t i;
  unsigned int version[4] = { 0, 0, 0, 0 };
  char *custom, osname[ 192 ];
  static const size_t CUSTOM_SIZE = 256;
  
  libspectrum_error error; int sys_error;

  const char *gcrypt_version;

  sscanf( VERSION, "%u.%u.%u.%u",
	  &version[0], &version[1], &version[2], &version[3] );

  for( i=0; i<4; i++ ) if( version[i] > 0xff ) version[i] = 0xff;

  sys_error = compat_osname( osname, sizeof( osname ) );
  if( sys_error ) return 1;

  fuse_creator = libspectrum_creator_alloc();

  error = libspectrum_creator_set_program( fuse_creator, "Fuse" );
  if( error ) { libspectrum_creator_free( fuse_creator ); return error; }

  error = libspectrum_creator_set_major( fuse_creator,
					 version[0] * 0x100 + version[1] );
  if( error ) { libspectrum_creator_free( fuse_creator ); return error; }

  error = libspectrum_creator_set_minor( fuse_creator,
					 version[2] * 0x100 + version[3] );
  if( error ) { libspectrum_creator_free( fuse_creator ); return error; }

  custom = libspectrum_new( char, CUSTOM_SIZE );

  gcrypt_version = libspectrum_gcrypt_version();
  if( !gcrypt_version ) gcrypt_version = "not available";

  snprintf( custom, CUSTOM_SIZE,
	    "gcrypt: %s\nlibspectrum: %s\nuname: %s", gcrypt_version,
	    libspectrum_version(), osname );

  error = libspectrum_creator_set_custom(
    fuse_creator, (libspectrum_byte*)custom, strlen( custom )
  );
  if( error ) {
    libspectrum_free( custom ); libspectrum_creator_free( fuse_creator );
    return error;
  }

  return 0;
}

static void
creator_end( void )
{
  libspectrum_creator_free( fuse_creator );
}

static void
creator_register_startup( void )
{
  startup_manager_module dependencies[] = { STARTUP_MANAGER_MODULE_SETUID };
  startup_manager_register( STARTUP_MANAGER_MODULE_CREATOR, dependencies,
                            ARRAY_SIZE( dependencies ), creator_init, NULL,
                            creator_end );
}

static void fuse_show_copyright(void)
{
  printf( "\n" );
  fuse_show_version();
  printf(
   FUSE_COPYRIGHT "; see the file\n"
   "'AUTHORS' for more details.\n"
   "\n"
   "For help, please mail <fuse-emulator-devel@lists.sf.net> or use\n"
   "the forums at <http://sourceforge.net/p/fuse-emulator/discussion/>.\n"
   "\n"
   "This program is distributed in the hope that it will be useful,\n"
   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
   "GNU General Public License for more details.\n\n");
}

static void fuse_show_version( void )
{
  printf( "The Free Unix Spectrum Emulator (Fuse) version " VERSION ".\n" );
}

static void fuse_show_help( void )
{
  printf( "\n" );
  fuse_show_version();
  printf(
   "\nAvailable command-line options:\n\n"
   "Boolean options (use `--no-<option>' to turn off):\n\n"
   "--auto-load            Automatically load tape files when opened.\n"
   "--compress-rzx         Write RZX files out compressed.\n"
   "--issue2               Emulate an Issue 2 Spectrum.\n"
   "--kempston             Emulate the Kempston joystick on QAOP<space>.\n"
   "--loading-sound        Emulate the sound of tapes loading.\n"
   "--sound                Produce sound.\n"
   "--sound-force-8bit     Generate 8-bit sound even if 16-bit is available.\n"
   "--slt                  Turn SLT traps on.\n"
   "--traps                Turn tape traps on.\n\n"
   "Other options:\n\n"
   "--help                 This information.\n"
   "--machine <type>       Which machine should be emulated?\n"
   "--playback <filename>  Play back RZX file <filename>.\n"
   "--record <filename>    Record to RZX file <filename>.\n"
   "--separation <type>    Use ACB/ABC stereo for the AY-3-8912 sound chip.\n"
   "--snapshot <filename>  Load snapshot <filename>.\n"
   "--speed <percentage>   How fast should emulation run?\n"
   "--fb-mode <mode>       Which mode should be used for FB?\n"
   "--tape <filename>      Open tape file <filename>.\n"
   "--version              Print version number and exit.\n"
   "\n"
   "For help, please mail <fuse-emulator-devel@lists.sf.net> or use\n"
   "the forums at <http://sourceforge.net/p/fuse-emulator/discussion/>.\n"
   "For complete documentation, see the manual page of Fuse.\n\n" );
}

/* Stop all activities associated with actual Spectrum emulation */
int fuse_emulation_pause(void)
{
  int error;

  /* If we were already paused, just return. In any case, increment
     the pause count */
  if( fuse_emulation_paused++ ) return 0;

  /* Stop recording any competition mode RZX file */
  if( rzx_recording && rzx_competition_mode ) {
    ui_error( UI_ERROR_INFO, "Stopping competition mode RZX recording" );
    error = rzx_stop_recording(); if( error ) return error;
  }
      
  /* If we had sound enabled (and hence doing the speed regulation),
     turn it off */
  sound_pause();

  return 0;
}

/* Restart emulation activities */
int fuse_emulation_unpause(void)
{
  int error;

  /* If this doesn't start us running again, just return. In any case,
     decrement the pause count */
  if( --fuse_emulation_paused ) return 0;

  /* If we now want sound, enable it */
  sound_unpause();

  /* Restart speed estimation with no information */
  error = timer_estimate_reset(); if( error ) return error;

  return 0;
}

static int
setup_start_files( start_files_t *start_files )
{
  start_files->disk_plus3 = settings_current.plus3disk_file;
  start_files->disk_opus = settings_current.opusdisk_file;
  start_files->disk_plusd = settings_current.plusddisk_file;
  start_files->disk_didaktik80 = settings_current.didaktik80disk_file;
  start_files->disk_disciple = settings_current.discipledisk_file;
  start_files->disk_beta = settings_current.betadisk_file;
  start_files->dock = settings_current.dck_file;
  start_files->if2 = settings_current.if2_file;
  start_files->playback = settings_current.playback_file;
  start_files->recording = settings_current.record_file;
  start_files->snapshot = settings_current.snapshot;
  start_files->tape = settings_current.tape_file;

  start_files->simpleide_master =
    utils_safe_strdup( settings_current.simpleide_master_file );
  start_files->simpleide_slave = 
    utils_safe_strdup( settings_current.simpleide_slave_file );

  start_files->zxatasp_master =
    utils_safe_strdup( settings_current.zxatasp_master_file );
  start_files->zxatasp_slave =
    utils_safe_strdup( settings_current.zxatasp_slave_file );

  start_files->zxcf = utils_safe_strdup( settings_current.zxcf_pri_file );

  start_files->divide_master =
    utils_safe_strdup( settings_current.divide_master_file );
  start_files->divide_slave =
    utils_safe_strdup( settings_current.divide_slave_file );

  start_files->divmmc =
    utils_safe_strdup( settings_current.divmmc_file );

  start_files->zxmmc = utils_safe_strdup( settings_current.zxmmc_file );

  start_files->mdr[0] = settings_current.mdr_file;
  start_files->mdr[1] = settings_current.mdr_file2;
  start_files->mdr[2] = settings_current.mdr_file3;
  start_files->mdr[3] = settings_current.mdr_file4;
  start_files->mdr[4] = settings_current.mdr_file5;
  start_files->mdr[5] = settings_current.mdr_file6;
  start_files->mdr[6] = settings_current.mdr_file7;
  start_files->mdr[7] = settings_current.mdr_file8;

  return 0;
}

/* Make 'best guesses' as to what to do with non-option arguments */
static int
parse_nonoption_args( int argc, char **argv, int first_arg,
		      start_files_t *start_files )
{
  size_t i, j;
  const char *filename;
  utils_file file;
  libspectrum_id_t type;
  libspectrum_class_t class;
  int error;

#ifdef GEKKO
  /* No argv on the Wii. Just return */
  return 0;
#endif

  for( i = first_arg; i < (size_t)argc; i++ ) {

    filename = argv[i];

    error = utils_read_file( filename, &file );
    if( error ) return error;

    error = libspectrum_identify_file_with_class( &type, &class, filename,
						  file.buffer, file.length );
    if( error ) {
      utils_close_file( &file );
      return error;
    }

    switch( class ) {

    case LIBSPECTRUM_CLASS_CARTRIDGE_TIMEX:
      start_files->dock = filename; break;

    case LIBSPECTRUM_CLASS_CARTRIDGE_IF2:
      start_files->if2 = filename; break;

    case LIBSPECTRUM_CLASS_HARDDISK:
      if( settings_current.zxcf_active ) {
	start_files->zxcf = filename;
      } else if( settings_current.zxatasp_active ) {
	start_files->zxatasp_master = filename;
      } else if( settings_current.simpleide_active ) {
	start_files->simpleide_master = filename;
      } else if( settings_current.divide_enabled ) {
	start_files->divide_master = filename;
      } else if( settings_current.divmmc_enabled ) {
	start_files->divmmc = filename;
      } else if( settings_current.zxmmc_enabled ) {
	start_files->zxmmc = filename;
      } else {
	/* No IDE interface active, so activate the ZXCF */
	settings_current.zxcf_active = 1;
	start_files->zxcf = filename;
      }
      break;

    case LIBSPECTRUM_CLASS_DISK_PLUS3:
      start_files->disk_plus3 = filename; break;

    case LIBSPECTRUM_CLASS_DISK_OPUS:
      start_files->disk_opus = filename; break;

    case LIBSPECTRUM_CLASS_DISK_DIDAKTIK:
      start_files->disk_didaktik80 = filename; break;

    case LIBSPECTRUM_CLASS_DISK_PLUSD:
      if( periph_is_active( PERIPH_TYPE_DISCIPLE ) )
        start_files->disk_disciple = filename;
      else
        start_files->disk_plusd = filename;
      break;

    case LIBSPECTRUM_CLASS_DISK_TRDOS:
      start_files->disk_beta = filename; break;

    case LIBSPECTRUM_CLASS_DISK_GENERIC:
      if( machine_current->capabilities &
                 LIBSPECTRUM_MACHINE_CAPABILITY_PLUS3_DISK )
        start_files->disk_plus3 = filename;
      else if( machine_current->capabilities &
                 LIBSPECTRUM_MACHINE_CAPABILITY_TRDOS_DISK )
        start_files->disk_beta = filename; 
      else {
        if( periph_is_active( PERIPH_TYPE_BETA128 ) )
          start_files->disk_beta = filename; 
        else if( periph_is_active( PERIPH_TYPE_PLUSD ) )
          start_files->disk_plusd = filename;
        else if( periph_is_active( PERIPH_TYPE_DIDAKTIK80 ) )
          start_files->disk_didaktik80 = filename;
        else if( periph_is_active( PERIPH_TYPE_DISCIPLE ) )
          start_files->disk_disciple = filename;
        else if( periph_is_active( PERIPH_TYPE_OPUS ) )
          start_files->disk_opus = filename;
      }
      break;

    case LIBSPECTRUM_CLASS_RECORDING:
      start_files->playback = filename; break;

    case LIBSPECTRUM_CLASS_SNAPSHOT:
      start_files->snapshot = filename; break;

    case LIBSPECTRUM_CLASS_MICRODRIVE:
      for( j = 0; j < 8; j++ ) {
        if( !start_files->mdr[j] ) {
	  start_files->mdr[j] = filename;
	  break;
	}
      }
      break;

    case LIBSPECTRUM_CLASS_TAPE:
      start_files->tape = filename; break;

    case LIBSPECTRUM_CLASS_AUXILIARY:
      if( type == LIBSPECTRUM_ID_AUX_POK ) {
        pokemem_set_pokfile( filename );
      }
      break;

    case LIBSPECTRUM_CLASS_UNKNOWN:
      ui_error( UI_ERROR_WARNING, "couldn't identify '%s'; ignoring it",
		filename );
      break;

    default:
      ui_error( UI_ERROR_ERROR, "parse_nonoption_args: unknown file class %d",
		class );
      break;

    }

    utils_close_file( &file );
  }

  return 0;
}

static int
do_start_files( start_files_t *start_files )
{
  int autoload, error, i, check_snapshot;

  /* Can't do both input recording and playback */
  if( start_files->playback && start_files->recording ) {
    ui_error(
      UI_ERROR_WARNING,
      "can't do both input playback and recording; recording disabled"
    );
    start_files->recording = NULL;
  }

  /* Can't use both +3 and TR-DOS disks simultaneously */
  if( start_files->disk_plus3 && start_files->disk_beta ) {
    ui_error(
      UI_ERROR_WARNING,
      "can't use +3 and TR-DOS disks simultaneously; +3 disk ignored"
    );
    start_files->disk_plus3 = NULL;
  }

  /* Can't use disks and the dock simultaneously */
  if( ( start_files->disk_plus3 || start_files->disk_beta ) &&
      start_files->dock                                         ) {
    ui_error(
      UI_ERROR_WARNING,
      "can't use disks and the dock simultaneously; dock cartridge ignored"
    );
    start_files->dock = NULL;
  }

  /* Can't use disks and the Interface 2 simultaneously */
  if( ( start_files->disk_plus3 || start_files->disk_beta ) &&
      start_files->if2                                          ) {
    ui_error(
      UI_ERROR_WARNING,
      "can't use disks and the Interface 2 simultaneously; cartridge ignored"
    );
    start_files->if2 = NULL;
  }

  /* If a snapshot has been specified, don't autoload tape, disks etc */
  autoload = start_files->snapshot ? 0 : tape_can_autoload();

  /* Load in each of the files. Input recording must be done after
     snapshot loading such that the right snapshot is embedded into
     the file; input playback being done after snapshot loading means
     any embedded snapshot in the input recording will override any
     specified snapshot */

  if( start_files->disk_plus3 ) {
    error = utils_open_file( start_files->disk_plus3, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->disk_plusd ) {
    error = utils_open_file( start_files->disk_plusd, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->disk_didaktik80 ) {
    error = utils_open_file( start_files->disk_didaktik80, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->disk_disciple ) {
    error = utils_open_file( start_files->disk_disciple, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->disk_opus ) {
    error = utils_open_file( start_files->disk_opus, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->disk_beta ) {
    error = utils_open_file( start_files->disk_beta, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->dock ) {
    error = utils_open_file( start_files->dock, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->if2 ) {
    error = utils_open_file( start_files->if2, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->snapshot ) {
    error = utils_open_file( start_files->snapshot, autoload, NULL );
    if( error ) return error;
  }

  if( start_files->tape ) {
    error = utils_open_file( start_files->tape, autoload, NULL );
    if( error ) return error;
  }

  /* Microdrive cartridges */

  for( i = 0; i < 8; i++ ) {
    if( start_files->mdr[i] ) {
      error = utils_open_file( start_files->mdr[i], autoload, NULL );
      if( error ) return error;
    }
  }

  /* IDE hard disk images */

  if( start_files->simpleide_master ) {
    error = simpleide_insert( start_files->simpleide_master,
			      LIBSPECTRUM_IDE_MASTER );
    simpleide_reset( 0 );
    if( error ) return error;
  }

  if( start_files->simpleide_slave ) {
    error = simpleide_insert( start_files->simpleide_slave,
			      LIBSPECTRUM_IDE_SLAVE );
    simpleide_reset( 0 );
    if( error ) return error;
  }

  if( start_files->zxatasp_master ) {
    error = zxatasp_insert( start_files->zxatasp_master,
			    LIBSPECTRUM_IDE_MASTER );
    if( error ) return error;
  }

  if( start_files->zxatasp_slave ) {
    error = zxatasp_insert( start_files->zxatasp_slave,
			    LIBSPECTRUM_IDE_SLAVE );
    if( error ) return error;
  }

  if( start_files->zxcf ) {
    error = zxcf_insert( start_files->zxcf ); if( error ) return error;
  }

  if( start_files->divide_master ) {
    error = divide_insert( start_files->divide_master,
			    LIBSPECTRUM_IDE_MASTER );
    if( error ) return error;
  }

  if( start_files->divide_slave ) {
    error = divide_insert( start_files->divide_slave,
			    LIBSPECTRUM_IDE_SLAVE );
    if( error ) return error;
  }

  if( start_files->divmmc ) {
    error = divmmc_insert( start_files->divmmc );
    if( error ) return error;
  }

  if( start_files->zxmmc ) {
    error = zxmmc_insert( start_files->zxmmc );
    if( error ) return error;
  }

  /* Input recordings */

  if( start_files->playback ) {
    check_snapshot = start_files->snapshot ? 0 : 1;
    error = rzx_start_playback( start_files->playback, check_snapshot );
    if( error ) return error;
  }

  if( start_files->recording ) {
    error = rzx_start_recording( start_files->recording,
				 settings_current.embed_snapshot );
    if( error ) return error;
  }

  return 0;
}

/* Tidy-up function called at end of emulation */
static int fuse_end(void)
{
  movie_stop();		/* stop movie recording */

  startup_manager_run_end();

  periph_end();
  ui_end();
  ui_media_drive_end();
  module_end();
  pokemem_end();

  svg_capture_end();

  libspectrum_end();

  return 0;
}

/* Emergency shutdown */
void
fuse_abort( void )
{
  fuse_end();
  abort();
}
