/* coreaudiosound.c: Mac OS X CoreAudio sound I/O
   Copyright (c) 2006 Fredrick Meunier

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

#include <errno.h>
#include <unistd.h>

#include <AssertMacros.h>

#include <AudioToolbox/AudioToolbox.h>

#include "settings.h"
#include "sfifo.h"
#include "sound.h"
#include "ui/ui.h"

sfifo_t sound_fifo;

/* Number of Spectrum frames audio latency to use */
#define NUM_FRAMES 2

static
OSStatus coreaudiowrite( void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,                       
                         AudioBufferList *ioData );

/* info about the format used for writing to output unit */
static AudioStreamBasicDescription deviceFormat;

/* converts from Fuse format (signed 16 bit ints) to CoreAudio format (floats)
 */
static AudioUnit gOutputUnit;

/* Records sound writer status information */
static int audio_output_started;

/* get the default output device for the HAL */
static int
get_default_output_device(AudioDeviceID* device)
{
  OSStatus err = kAudioHardwareNoError;
  UInt32 count;

  AudioObjectPropertyAddress property_address = { 
    kAudioHardwarePropertyDefaultOutputDevice, 
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  }; 

  /* get the default output device for the HAL */
  count = sizeof( *device );
  err = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property_address,
                                    0, NULL, &count, device); 
  if ( err != kAudioHardwareNoError && device != kAudioObjectUnknown ) {
    ui_error( UI_ERROR_ERROR,
              "get kAudioHardwarePropertyDefaultOutputDevice error %ld",
              (long)err );
    return 1;
  }

  return 0;
}

/* get the nominal sample rate used by the supplied device */
static int
get_default_sample_rate( AudioDeviceID device, Float64 *rate )
{
  OSStatus err = kAudioHardwareNoError;
  UInt32 count;

  AudioObjectPropertyAddress property_address = { 
    kAudioDevicePropertyNominalSampleRate,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  }; 

  /* get the default output device for the HAL */
  count = sizeof( *rate );
  err = AudioObjectGetPropertyData( device, &property_address, 0, NULL, &count,
                                    rate);
  if ( err != kAudioHardwareNoError ) {
    ui_error( UI_ERROR_ERROR,
              "get kAudioDevicePropertyNominalSampleRate error %ld",
              (long)err );
    return 1;
  }

  return 0;
}

int
sound_lowlevel_init( const char *dev, int *freqptr, int *stereoptr )
{
  OSStatus err = kAudioHardwareNoError;
  AudioDeviceID device = kAudioObjectUnknown; /* the default device */
  int error;
  float hz;
  int sound_framesiz;

  if( get_default_output_device(&device) ) return 1;
  if( get_default_sample_rate( device, &deviceFormat.mSampleRate ) ) return 1;

  *freqptr = deviceFormat.mSampleRate;

  deviceFormat.mFormatID =  kAudioFormatLinearPCM;
  deviceFormat.mFormatFlags =  kLinearPCMFormatFlagIsSignedInteger
#ifdef WORDS_BIGENDIAN
                    | kLinearPCMFormatFlagIsBigEndian
#endif      /* #ifdef WORDS_BIGENDIAN */
                    | kLinearPCMFormatFlagIsPacked;
  deviceFormat.mBytesPerPacket = *stereoptr ? 4 : 2;
  deviceFormat.mFramesPerPacket = 1;
  deviceFormat.mBytesPerFrame = *stereoptr ? 4 : 2;
  deviceFormat.mBitsPerChannel = 16;
  deviceFormat.mChannelsPerFrame = *stereoptr ? 2 : 1;

  /* Open the default output unit */
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext( NULL, &desc );
  if( comp == NULL ) {
    ui_error( UI_ERROR_ERROR, "AudioComponentFindNext" );
    return 1;
  }

  err = AudioComponentInstanceNew( comp, &gOutputUnit );
  if( comp == NULL ) {
    ui_error( UI_ERROR_ERROR, "AudioComponentInstanceNew=%ld", (long)err );
    return 1;
  }

  /* Set up a callback function to generate output to the output unit */
  AURenderCallbackStruct input;
  input.inputProc = coreaudiowrite;
  input.inputProcRefCon = NULL;

  err = AudioUnitSetProperty( gOutputUnit,                       
                              kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input,
                              0,
                              &input,
                              sizeof( input ) );
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitSetProperty-CB=%ld", (long)err );
    return 1;
  }

  err = AudioUnitSetProperty( gOutputUnit,
                              kAudioUnitProperty_StreamFormat,
                              kAudioUnitScope_Input,
                              0,
                              &deviceFormat,
                              sizeof( AudioStreamBasicDescription ) );
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitSetProperty-SF=%4.4s, %ld", (char*)&err,
              (long)err );
    return 1;
  }

  err = AudioUnitInitialize( gOutputUnit );
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitInitialize=%ld", (long)err );
    return 1;
  }

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
  sound_framesiz = deviceFormat.mSampleRate / hz;

  if( ( error = sfifo_init( &sound_fifo, NUM_FRAMES
                                         * deviceFormat.mBytesPerFrame
                                         * deviceFormat.mChannelsPerFrame
                                         * sound_framesiz + 1 ) ) ) {
    ui_error( UI_ERROR_ERROR, "Problem initialising sound fifo: %s",
              strerror ( error ) );
    return 1;
  }

  /* wait to run sound until we have some sound to play */
  audio_output_started = 0;

  return 0;
}

/* Support pre Xcode 9 SDK */
#ifndef __Verify_noErr
#define __Verify_noErr((a))  verify_noerr((a))
#endif

void
sound_lowlevel_end( void )
{
  OSStatus err;

  if( audio_output_started )
    __Verify_noErr( AudioOutputUnitStop( gOutputUnit ) );

  err = AudioUnitUninitialize( gOutputUnit );
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioUnitUninitialize=%ld", (long)err );
  }

  err = AudioComponentInstanceDispose( gOutputUnit );
  if( err ) {
    ui_error( UI_ERROR_ERROR, "AudioComponentInstanceDispose=%ld", (long)err );
  }

  sfifo_flush( &sound_fifo );
  sfifo_close( &sound_fifo );
}

/* Copy data to fifo */
void
sound_lowlevel_frame( libspectrum_signed_word *data, int len )
{
  int i = 0;

  /* Convert to bytes */
  libspectrum_signed_byte* bytes = (libspectrum_signed_byte*)data;
  len <<= 1;

  while( len ) {
    if( ( i = sfifo_write( &sound_fifo, bytes, len ) ) < 0 ) {
      break;
    } else if( !i ) {
      usleep( 10000 );
    }
    bytes += i;
    len -= i;
  }
  if( i < 0 ) {
    ui_error( UI_ERROR_ERROR, "Couldn't write sound fifo: %s",
              strerror( i ) );
  }

  if( !audio_output_started ) {
    /* Start the rendering
       The DefaultOutputUnit will do any format conversions to the format of the
       default device */
    OSStatus err = AudioOutputUnitStart( gOutputUnit );
    if( err ) {
      ui_error( UI_ERROR_ERROR, "AudioOutputUnitStart=%ld", (long)err );
      return;
    }

    audio_output_started = 1;
  }
}

#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/* This is the audio processing callback. */
OSStatus coreaudiowrite( void *inRefCon,
                         AudioUnitRenderActionFlags *ioActionFlags,
                         const AudioTimeStamp *inTimeStamp,
                         UInt32 inBusNumber,
                         UInt32 inNumberFrames,                       
                         AudioBufferList *ioData )
{
  int f;
  int len = deviceFormat.mBytesPerFrame * inNumberFrames;
  uint8_t* out = ioData->mBuffers[0].mData;

  /* Try to only read an even number of bytes so as not to fragment a sample */
  len = MIN( len, sfifo_used( &sound_fifo ) );
  len &= sound_stereo_ay != SOUND_STEREO_AY_NONE ? 0xfffc : 0xfffe;

  /* Read input_size bytes from fifo into sound stream */
  while( ( f = sfifo_read( &sound_fifo, out, len ) ) > 0 ) {
    out += f;
    len -= f;
  }

  /* If we ran out of sound, make do with silence :( */
  if( f < 0 ) {
    for( f=0; f<len; f++ ) {
      *out++ = 0;
    }
  }

  return noErr;
}
