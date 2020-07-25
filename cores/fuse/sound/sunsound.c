/* sunsound.c: OpenBSD sound I/O
   Copyright (c) 2002-2004 Alexander Yurchenko, Russell Marks, Philip Kendall

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

#if defined AUDIO_SETINFO || defined HAVE_SYS_AUDIOIO_H

#if defined(__SVR4) && defined(__sun)
#define solaris
#else
#undef solaris
#endif

#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <string.h>
#ifdef solaris
#include <stropts.h>
#endif
#include <unistd.h>

#include "settings.h"
#include "sound.h"
#include "ui/ui.h"

/* using (8) 64 byte frags for 8kHz, scale up for higher */
#define BASE_SOUND_FRAG_PWR	6

static int soundfd = -1;
static int sixteenbit = 0;

int
sound_lowlevel_init( const char *device, int *freqptr, int *stereoptr )
{
#ifndef solaris
	int frag;
#endif
	int flags;
	struct audio_info ai;

	if (device == NULL)
		device = "/dev/audio";

	/* Open the sound device non-blocking to avoid hangs if it is
	   being used by something else, but then set it blocking
	   again as that's what we actually want */
	if ((soundfd = open(device, O_WRONLY | O_NONBLOCK )) == -1) {
		settings_current.sound = 0;
	        ui_error( UI_ERROR_ERROR, "Couldn't open sound device '%s'",
			  device );
		return 1;
	}
	if ((flags = fcntl(soundfd, F_GETFL)) == -1) {
		settings_current.sound = 0;
	        ui_error( UI_ERROR_ERROR, "Couldn't fcntl sound device '%s'",
			  device );
	        close(soundfd);
	        return 1;
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(soundfd, F_SETFL, flags) == -1) {
		settings_current.sound = 0;
	        ui_error( UI_ERROR_ERROR,
			  "Couldn't set sound device '%s' blocking",
			  device );
	        close(soundfd);
		return 1;
	}

	AUDIO_INITINFO(&ai);
	
	ai.play.encoding = AUDIO_ENCODING_LINEAR;	
	ai.play.precision = 16;
	sixteenbit = 1;
	if (settings_current.sound_force_8bit ||
	    ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
		/* try 8-bit */
		ai.play.encoding = AUDIO_ENCODING_LINEAR8;
		ai.play.precision = 8;
		sixteenbit = 0;
		if (ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
			settings_current.sound = 0;
		        ui_error( UI_ERROR_ERROR,
				"Couldn't set bit size of sound device '%s'",
				  device );
			close(soundfd);
			return 1;
		}
	}

	ai.play.channels = *stereoptr ? 2 : 1;
	if (ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
		/* if it failed make sure the opposite is ok */
		ai.play.channels = *stereoptr ? 1 : 2;
		if (ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
			settings_current.sound = 0;
		        ui_error( UI_ERROR_ERROR,
				"Couldn't set channels of sound device '%s'",
				  device );
			close(soundfd);
			return 1;
		}
		/* FIXME: is this line correct? */
		*stereoptr = *stereoptr ? 1 : 2;
	}

	ai.play.sample_rate = *freqptr;
	if (ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
		settings_current.sound = 0;
	        ui_error( UI_ERROR_ERROR,
			  "Couldn't set sample rate of sound device '%s'",
			  device );
		close(soundfd);
		return 1;
	}

#ifndef solaris
	frag = 0x80000 | BASE_SOUND_FRAG_PWR;
	if (*freqptr > 8250)
		frag++;
	if (*freqptr > 16500)
		frag++;
	if (*freqptr > 33000)
		frag++;
	if (*stereoptr)
		frag++;
	if (sixteenbit)
		frag++;
	ai.blocksize = 1 << (frag & 0xffff);
	ai.hiwat = ((unsigned)frag >> 16) & 0x7fff;
	if (ai.hiwat == 0)
		ai.hiwat = 65536;
	if (ioctl(soundfd, AUDIO_SETINFO, &ai) == -1) {
		settings_current.sound = 0;
	        ui_error( UI_ERROR_ERROR,
			  "Couldn't set block size of sound device '%s'",
			  device );
		close(soundfd);
		return 1;
	}
#endif

	return 0;
}

void
sound_lowlevel_end( void )
{
#ifdef solaris
	ioctl(soundfd, I_FLUSH, FLUSHW);
#else
	ioctl(soundfd, AUDIO_FLUSH);
#endif
	close(soundfd);
}

void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
	static unsigned char buf8[4096];
	unsigned char *data8=(unsigned char *)data;
	int ret=0, ofs=0;

	len <<= 1;		/* now in bytes */

	if (!sixteenbit) {
		libspectrum_signed_word *src;
		unsigned char *dst;
		int f;

		src = data; dst = buf8;
		len >>= 1;
		for (f=0; f < len; f++)
			*dst++ = 128 + (int)((*src++)/256);

		data8 = buf8;
	}	

	while (len) {
		ret = write (soundfd, data8 + ofs, len);
		if (ret > 0) {
			ofs += ret;
			len -= ret;
		}
	}
}

#endif		/* #if defined AUDIO_SETINFO || defined HAVE_SYS_AUDIOIO_H */

