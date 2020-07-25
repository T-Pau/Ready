/* osssound.c: OSS (e.g. Linux) sound I/O
   Copyright (c) 2000-2004 Russell Marks, Matan Ziv-Av, Philip Kendall

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>

#include "settings.h"
#include "sound.h"
#include "spectrum.h"
#include "ui/ui.h"

/* using (8) 64 byte frags for 8kHz, scale up for higher */
#define BASE_SOUND_FRAG_PWR	6


static int soundfd=-1;
static int sixteenbit=1;


/* returns 0 on *success*, and adjusts freq/stereo args to reflect
 * what we've actually got.
 */
int sound_lowlevel_init(const char *device,int *freqptr,int *stereoptr)
{
int frag,tmp,flags;

/* select a default device if we weren't explicitly given one */
if(device==NULL) device = "/dev/dsp";

/* Open the sound device non-blocking to avoid hangs if it is being
 * used by something else, but then set it blocking again as that's what
 * we actually want
 */
if((soundfd=open(device,O_WRONLY|O_NONBLOCK))==-1)
  {
  settings_current.sound = 0;
  ui_error(UI_ERROR_ERROR,"couldn't open sound device '%s': %s",device,strerror(errno));
  return 1;
  }
if((flags=fcntl(soundfd,F_GETFL))==-1)
  {
  settings_current.sound = 0;
  ui_error(UI_ERROR_ERROR,"couldn't get flags from sound device: %s",strerror(errno));
  close(soundfd);
  return 1;
  }
flags &= ~O_NONBLOCK;
if(fcntl(soundfd,F_SETFL,flags)==-1)
  {
  settings_current.sound = 0;
  ui_error(UI_ERROR_ERROR,"couldn't set sound device non-blocking: %s",strerror(errno));
  close(soundfd);
  return 1;
  }

tmp=AFMT_S16_NE;
sixteenbit=1;
if(settings_current.sound_force_8bit || ioctl(soundfd,SNDCTL_DSP_SETFMT,&tmp)==-1)
  {
  /* try 8-bit - may be an 8-bit-only device */
  tmp=AFMT_U8;
  if((ioctl(soundfd,SNDCTL_DSP_SETFMT,&tmp))==-1)
    {
    settings_current.sound = 0;
    ui_error(UI_ERROR_ERROR,"couldn't set sound device into 8-bit mode: %s",strerror(errno));
    close(soundfd);
    return 1;
    }

  sixteenbit=0;
  }

/* XXX should it warn if it didn't get the stereoness it wanted? */
tmp=(*stereoptr)?1:0;
if(ioctl(soundfd,SNDCTL_DSP_STEREO,&tmp)<0)
  {
  /* if it failed, make sure the opposite is ok */
  tmp=(*stereoptr)?0:1;
  if(ioctl(soundfd,SNDCTL_DSP_STEREO,&tmp)<0)
    {
    settings_current.sound = 0;
    ui_error(UI_ERROR_ERROR,"couldn't set sound device into either mono or stereo mode: %s",strerror(errno));
    close(soundfd);
    return 1;
    }
  *stereoptr=tmp;
  }

frag=(0x80000|BASE_SOUND_FRAG_PWR);

if(ioctl(soundfd,SNDCTL_DSP_SPEED,freqptr)<0)
  {
  settings_current.sound = 0;
  ui_error(UI_ERROR_ERROR,"couldn't set sound device speed to %d: %s",*freqptr, strerror(errno));
  close(soundfd);
  return 1;
  }

if(*freqptr>8250) frag++;
if(*freqptr>16500) frag++;
if(*freqptr>33000) frag++;
if(*stereoptr) frag++;
if(sixteenbit) frag++;

/* FIXME: OSS API docs say you should write to the soundcard in frag size
   blocks */
if(ioctl(soundfd,SNDCTL_DSP_SETFRAGMENT,&frag)<0)
  {
  settings_current.sound = 0;
  ui_error(UI_ERROR_ERROR,"couldn't set sound device fragment size to %d: %s",frag,strerror(errno));
  close(soundfd);
  return 1;
  }

return 0;	/* success */
}


void sound_lowlevel_end(void)
{
if(soundfd!=-1)
  close(soundfd);
}


void sound_lowlevel_frame(libspectrum_signed_word *data,int len)
{
static unsigned char buf8[4096];
unsigned char *data8=(unsigned char *)data;
int ret=0,ofs=0;

len<<=1;	/* now in bytes */

if(!sixteenbit)
  {
  libspectrum_signed_word *src;
  unsigned char *dst;
  int f;

  src=data; dst=buf8;
  len>>=1;
  for(f=0;f<len;f++)
    *dst++=128+(int)((*src++)/256);
  
  data8=buf8;
  }

while(len)
  {
  ret=write(soundfd,data8+ofs,len);
  if(ret>0)
    ofs+=ret,len-=ret;
  }
}
