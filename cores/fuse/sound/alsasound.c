/* alsasound.c: ALSA (Linux) sound I/O
   Copyright (c) 2006 Gergely Szasz

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

*/

#include <config.h>

/* This is necessary to prevent warnings from the calls to
   snd_pcm_[hs]w_params_alloca() */
#ifndef NDEBUG
#define NDEBUG
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <alsa/asoundlib.h>

#include "settings.h"
#include "sfifo.h"
#include "sound.h"
#include "spectrum.h"
#include "ui/ui.h"

/* Number of Spectrum frames audio latency to use */
#define NUM_FRAMES 3

static snd_pcm_t *pcm_handle;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
static int ch, framesize;
static const char *pcm_name = NULL;
static int verb = 0;
static snd_pcm_uframes_t exact_periodsize, exact_bsize;

static snd_output_t *output = NULL;

void
sound_lowlevel_end( void )
{
/* Stop PCM device and drop pending frames */
  snd_pcm_drop( pcm_handle );
  snd_pcm_close( pcm_handle );
}

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
  unsigned int exact_rate, periods;
  unsigned int val, n;
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_sw_params_t *sw_params;
  snd_pcm_uframes_t avail_min = 0, sound_periodsize, bsize = 0;
  static int first_init = 1;
  static int init_running = 0;
  const char *option;
  char tmp;
  int err, dir, nperiods = NUM_FRAMES;

  float hz;

  if( init_running )
    return 0;
  
  init_running = 1;
/* select a default device if we weren't explicitly given one */

  option = device;
  while( option && *option ) {
    tmp = '*';
    if( ( err = sscanf( option, " buffer=%i %n%c", &val, &n, &tmp ) > 0 ) &&
		( tmp == ',' || strlen( option ) == n ) ) {
      if( val < 1 ) {
	fprintf( stderr, "Bad value for ALSA buffer size %i, using default\n",
		    val );
      } else {
        bsize = val;
      }
    } else if( ( err = sscanf( option, " frames=%i %n%c", &val, &n, &tmp ) > 0 ) &&
		( tmp == ',' || strlen( option ) == n ) ) {
      if( val < 1 ) {
	fprintf( stderr, "Bad value for ALSA buffer size %i frames, using default (%d)\n",
		    val, NUM_FRAMES );
      } else {
        nperiods = val;
      }
    } else if( ( err = sscanf( option, " avail=%i %n%c", &val, &n, &tmp ) > 0 ) &&
		( tmp == ',' || strlen( option ) == n ) ) {
      if( val < 1 ) {
	fprintf( stderr, "Bad value for ALSA avail_min size %i frames, using default\n",
		    val );
      } else {
        avail_min = val;
      }
    } else if( ( err = sscanf( option, " verbose %n%c", &n, &tmp ) == 1 ) &&
		( tmp == ','  || strlen( option ) == n ) ) {
      verb = 1;
    } else {					/* try as device name */
	while( isspace(*option) )
          option++;
	if( *option == '\'' )		/* force device... */
	  option++;
	pcm_name = option;
	n = strlen( pcm_name );
    }
    option += n + ( tmp == ',' );
  }

/* Open the sound device
 */
  if( pcm_name == NULL || *pcm_name == '\0' )
    pcm_name = "default";
  if( snd_pcm_open( &pcm_handle, pcm_name , stream, 0 ) < 0 ) {
    if( strcmp( pcm_name, "default" ) == 0 ) {
    /* we try a last one: plughw:0,0 but what a weired ALSA conf.... */
      if( snd_pcm_open( &pcm_handle, "plughw:0,0", stream, 0 ) < 0 ) {
        settings_current.sound = 0;
        ui_error( UI_ERROR_ERROR,
                  "couldn't open sound device 'default' and 'plughw:0,0' check ALSA configuration."
                  );
	init_running = 0;
        return 1;
      } else {
        if( first_init )
          fprintf( stderr,
                    "Couldn't open sound device 'default', using 'plughw:0,0' check ALSA configuration.\n"
                    );
      }
    }
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't open sound device '%s'.", pcm_name );
    init_running = 0;
    return 1;
  }

/* Allocate the snd_pcm_hw_params_t structure on the stack. */
  snd_pcm_hw_params_alloca( &hw_params );

/* Init hw_params with full configuration space */
  if( snd_pcm_hw_params_any( pcm_handle, hw_params ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR,
              "couldn't get configuration space on sound device '%s'.",
              pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  if( snd_pcm_hw_params_set_access( pcm_handle, hw_params,
                                    SND_PCM_ACCESS_RW_INTERLEAVED ) < 0) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set access interleaved on '%s'.",
              pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }
  
    /* Set sample format */
  if( snd_pcm_hw_params_set_format( pcm_handle, hw_params, 
#if defined WORDS_BIGENDIAN
				    SND_PCM_FORMAT_S16_BE
#else
				    SND_PCM_FORMAT_S16_LE
#endif
						    ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set format on '%s'.", pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  ch = *stereoptr ? 2 : 1;

  if( snd_pcm_hw_params_set_channels( pcm_handle, hw_params, ch )
	    < 0 ) {
    fprintf( stderr, "Couldn't set %s to '%s'.\n", pcm_name,
    		    (*stereoptr ? "stereo" : "mono") );
    ch = *stereoptr ? 1 : 2;		/* try with opposite */
    if( snd_pcm_hw_params_set_channels( pcm_handle, hw_params, ch )
	    < 0 ) {
      ui_error( UI_ERROR_ERROR, "couldn't set %s to '%s'.", pcm_name,
    		    (*stereoptr ? "stereo" : "mono") );
      settings_current.sound = 0;
      snd_pcm_close( pcm_handle );
      init_running = 0;
      return 1;
    }
    *stereoptr = *stereoptr ? 0 : 1;		/* write back */
  }

  framesize = ch << 1;			/* we always use 16 bit sorry :-( */
/* Set sample rate. If the exact rate is not supported */
/* by the hardware, use nearest possible rate.         */ 
  exact_rate = *freqptr;
  if( snd_pcm_hw_params_set_rate_near( pcm_handle, hw_params, &exact_rate,
							NULL ) < 0) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set rate %d on '%s'.",
						*freqptr, pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }
  if( first_init && *freqptr != exact_rate ) {
    fprintf( stderr, 
              "The rate %d Hz is not supported by your hardware. "
              "Using %d Hz instead.\n", *freqptr, exact_rate );
    *freqptr = exact_rate;
  }

  if( bsize != 0 ) {
    exact_periodsize = sound_periodsize = bsize / nperiods;
    if( bsize < 1 ) {
      fprintf( stderr,
                "bad value for ALSA buffer size %i, using default.\n",
		val );
      bsize = 0;
    }
  }

  if( bsize == 0 ) {
    /* Adjust relative processor speed to deal with adjusting sound generation
       frequency against emulation speed (more flexible than adjusting generated
       sample rate) */
    hz = (float)sound_get_effective_processor_speed() /
              machine_current->timings.tstates_per_frame;
    /* Amount of audio data we will accumulate before yielding back to the OS.
       Not much point having more than 100Hz playback, we probably get
       downgraded by the OS as being a hog too (unlimited Hz limits playback
       speed to about 2000% on my Mac, 100Hz allows up to 5000% for me) */
    if( hz > 100.0 ) hz = 100.0;
    exact_periodsize = sound_periodsize = *freqptr / hz;
  }

  dir = -1;
  if( snd_pcm_hw_params_set_period_size_near( pcm_handle, hw_params,
					    &exact_periodsize, &dir ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set period size %d on '%s'.",
                              (int)sound_periodsize, pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  if( first_init && ( exact_periodsize < sound_periodsize / 1.5 ||
		      exact_periodsize > sound_periodsize * 1.5    ) ) {
    fprintf( stderr,
              "The period size %d is not supported by your hardware. "
              "Using %d instead.\n", (int)sound_periodsize,
              (int)exact_periodsize );
  }

  periods = nperiods;
/* Set number of periods. Periods used to be called fragments. */
  if( snd_pcm_hw_params_set_periods_near( pcm_handle, hw_params, &periods,
                                          NULL ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR, "couldn't set periods on '%s'.", pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  if( first_init && periods != nperiods ) {
    fprintf( stderr, "%d periods is not supported by your hardware. "
                    	     "Using %d instead.\n", nperiods, periods );
  }

  snd_pcm_hw_params_get_buffer_size( hw_params, &exact_bsize );

  /* Apply HW parameter settings to */
  /* PCM device and prepare device  */

  if( snd_pcm_hw_params( pcm_handle, hw_params ) < 0 ) {
    settings_current.sound = 0;
    ui_error( UI_ERROR_ERROR,"couldn't set hw_params on %s", pcm_name );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  snd_pcm_sw_params_alloca( &sw_params );
  if( ( err = snd_pcm_sw_params_current( pcm_handle, sw_params ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't get sw_params from %s: %s", pcm_name,
              snd_strerror ( err ) );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  if( ( err = snd_pcm_sw_params_set_start_threshold( pcm_handle,
		     sw_params, exact_periodsize * ( nperiods - 1 ) ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set start_treshold on %s: %s", pcm_name,
              snd_strerror ( err ) );
    snd_pcm_close( pcm_handle );
    init_running = 0;
    return 1;
  }

  if( !avail_min )
    avail_min = exact_periodsize >> 1;
  if( snd_pcm_sw_params_set_avail_min( pcm_handle,
		    sw_params, avail_min ) < 0 ) {
#if SND_LIB_VERSION < 0x10010
    if( ( err = snd_pcm_sw_params_set_sleep_min( pcm_handle,
    		    sw_params, 1 ) ) < 0 ) {
	fprintf( stderr, "Unable to set minimal sleep 1 for %s: %s\n", pcm_name,
              snd_strerror ( err ) );
    }
#else
    fprintf( stderr, "Unable to set avail min %s: %s\n", pcm_name,
    	     snd_strerror( err ) );
#endif
  }

#if SND_LIB_VERSION < 0x10010
  if( ( err = snd_pcm_sw_params_set_xfer_align( pcm_handle, sw_params, 1 ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set xfer_allign on %s: %s", pcm_name,
              snd_strerror ( err ) );
    init_running = 0;
    return 1;
  }
#endif
  
  if( ( err = snd_pcm_sw_params( pcm_handle, sw_params ) ) < 0 ) {
    ui_error( UI_ERROR_ERROR,"couldn't set sw_params on %s: %s", pcm_name,
              snd_strerror ( err ) );
    init_running = 0;
    return 1;
  }

  if( first_init ) snd_output_stdio_attach(&output, stdout, 0);

  first_init = 0;
  init_running = 0;
  return 0;	/* success */
}



void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int ret = 0;
  len /= ch;	/* now in frames */

/*	to measure sound lag :-)
  snd_pcm_status_t *status;
  snd_pcm_sframes_t delay;
    
  snd_pcm_status_alloca( &status );
  snd_pcm_status( pcm_handle, status ); 
  delay = snd_pcm_status_get_delay( status );
  fprintf( stderr, "%d ", (int)delay );
*/

  while( ( ret = snd_pcm_writei( pcm_handle, data, len ) ) != len ) {
    if( ret < 0 ) {
      snd_pcm_prepare( pcm_handle );
      if( verb )
        fprintf( stderr, "ALSA: *buffer underrun*!\n" );
    } else {
        len -= ret;
    }
  }
}
