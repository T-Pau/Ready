/* blipbuffer.h
 Band-limited sound synthesis and buffering

 Blip_Buffer 0.4.0

Original C++ source:
Blip_Buffer 0.4.0. http://www.slack.net/~ant/

Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

partially reimplemented in C by Gergely Szasz

There is a lot of ambiguous thing in the original C++ source, some of them `rectified'
(e.g. `int const blahblah = xxx` in the header translated to #define ), others not
(e.g. blip_sample_t/imp_t/short or Blip_Synth_ in Blip_Synth, or the whole
blip_eq_t struct ...).

Functions originally implemented in the header are moved to .c.
Classes converted to structs, member functions prefixed with: blip_buff_
_blip_synth_ and blip_synth_ resp.

set_treble_eq() implemented with last argument as `double` instead
of `blip_eq_t` because this function always call just with treble value,
and C lack of `plain' type -> `struct' magic conversion/construction
capabilities.

The source now almost according to C89. (except of course `inline')

*/
#ifndef BLIP_BUFFER_H
#define BLIP_BUFFER_H

/*
 Time unit at source clock rate
*/
typedef long blip_time_t;

/*  Output samples are 16-bit signed, with a range of -32767 to 32767 */
typedef short blip_sample_t;

typedef const char *blargg_err_t;

typedef long buf_t_;

typedef unsigned long blip_resampled_time_t;

# define BLIP_BUFFER_DEF_MSEC_LENGTH ( 1000 / 4 )
# define BLIP_BUFFER_DEF_STEREO 0
# define BLIP_BUFFER_DEF_ENTIRE_BUFF 1

typedef struct Blip_Buffer_s {
  unsigned long factor_;
  blip_resampled_time_t offset_;
  buf_t_ *buffer_;
  long buffer_size_;
  long reader_accum;
  int bass_shift;
  long sample_rate_;
  long clock_rate_;
  int bass_freq_;
  int length_;
} Blip_Buffer;

Blip_Buffer *new_Blip_Buffer( void );

void delete_Blip_Buffer( Blip_Buffer ** buff );

/*  Set output sample rate and buffer length in milliseconds (1/1000 sec, defaults
 to 1/4 second), then clear buffer. Returns NULL on success, otherwise if there
 isn't enough memory, returns error without affecting current buffer setup.
*/
blargg_err_t blip_buffer_set_sample_rate( Blip_Buffer * buff,
                                          long samples_per_sec,
                                          int msec_length );

/*  Set number of source time units per second
*/
void blip_buffer_set_clock_rate( Blip_Buffer * buff, long rate );

/*  End current time frame of specified duration and make its samples available
 (along with any still-unread samples) for reading with read_samples(). Begins
 a new time frame at the end of the current frame.
*/
void blip_buffer_end_frame( Blip_Buffer * buff, blip_time_t time );

/*  Read at most 'max_samples' out of buffer into 'dest', removing them from from
 the buffer. Returns number of samples actually read and removed. If stereo is
 true, increments 'dest' one extra time after writing each sample, to allow
 easy interleving of two channels into a stereo output buffer.
*/
long blip_buffer_read_samples( Blip_Buffer * buff, blip_sample_t * dest,
                               long max_samples, int stereo );

/*  Additional optional features */

/*  Set frequency high-pass filter frequency, where higher values reduce bass more */
void blip_buffer_set_bass_freq( Blip_Buffer * buff, int frequency );

/*  Remove all available samples and clear buffer to silence. If 'entire_buffer' is
 false, just clears out any samples waiting rather than the entire buffer.
*/
void blip_buffer_clear( Blip_Buffer * buff, int entire_buffer );

/*  Number of samples available for reading with read_samples()
*/
long
blip_buffer_samples_avail( Blip_Buffer * buff );

/*  Remove 'count' samples from those waiting to be read
*/
void blip_buffer_remove_samples( Blip_Buffer * buff, long count );

/*  Experimental features */

void blip_buffer_remove_silence( Blip_Buffer * buff, long count );

blip_resampled_time_t blip_buffer_clock_rate_factor( Blip_Buffer * buff,
                                                    long clock_rate );

/*
 Number of bits in resample ratio fraction. Higher values give a more accurate ratio
 but reduce maximum buffer size.
*/
#ifndef BLIP_BUFFER_ACCURACY
#define BLIP_BUFFER_ACCURACY 16
#endif
/*
 Number bits in phase offset. Fewer than 6 bits (64 phase offsets) results in
 noticeable broadband noise when synthesizing high frequency square waves.
 Affects size of Blip_Synth objects since they store the waveform directly.
*/
#ifndef BLIP_PHASE_BITS
#define BLIP_PHASE_BITS 6
#endif

/*	 Internal */
#define BLIP_WIDEST_IMPULSE_ 16
#define BLIP_RES (1 << BLIP_PHASE_BITS)

struct blip_eq_s;

typedef struct Blip_Synth_s_ {
  double volume_unit_;
  short *impulses;
  long kernel_unit;

  Blip_Buffer *buf;
  int last_amp;
  int delta_factor;
} Blip_Synth_;

int _blip_synth_impulses_size( Blip_Synth_ * synth_ );

void _blip_synth_adjust_impulse( Blip_Synth_ * synth_ );

void _blip_synth_treble_eq( Blip_Synth_ * synth_, struct blip_eq_s *eq );

void _blip_synth_volume_unit( Blip_Synth_ * synth_, double v );

/*  Quality level. Start with blip_good_quality. */
#define BLIP_MED_QUALITY 8
#define BLIP_GOOD_QUALITY 12
#define BLIP_HIGH_QUALITY 16

/*  Range specifies the greatest expected change in amplitude. Calculate it
 by finding the difference between the maximum and minimum expected
 amplitudes (max - min). */

typedef short imp_t;

typedef struct Blip_Synth_s {
  imp_t *impulses;
  Blip_Synth_ impl;
} Blip_Synth;

void blip_synth_set_volume( Blip_Synth * synth, double v );

/*  Configure low-pass filter (see notes.txt)*/
void blip_synth_set_treble_eq( Blip_Synth * synth, double treble );

/*  Get/set Blip_Buffer used for output */
void blip_synth_set_output( Blip_Synth * synth, Blip_Buffer * b );

/*  Update amplitude of waveform at given time. Using this requires a separate
Blip_Synth for each waveform. */
void blip_synth_update( Blip_Synth * synth, blip_time_t time,
                        int amplitude );

/*  Low-level interface */

void blip_synth_offset_resampled( Blip_Synth * synth,
                                 blip_resampled_time_t, int delta,
                                 Blip_Buffer * buff );
Blip_Synth *new_Blip_Synth( void );

void delete_Blip_Synth( Blip_Synth ** synth );

#define BLIP_EQ_DEF_CUTOFF 0
/*  Low-pass equalization parameters */

typedef struct blip_eq_s {
  double treble;
  long rolloff_freq;
  long sample_rate;
  long cutoff_freq;
} blip_eq_t;

#define BLIP_SAMPLE_BITS 30

/*  End of public interface */

#define BLIP_UNSCALED 65535
#define BLIP_MAX_LENGTH 0

#define BLIP_SYNTH_QUALITY BLIP_GOOD_QUALITY
#define BLIP_SYNTH_RANGE BLIP_UNSCALED
#define BLIP_SYNTH_WIDTH BLIP_SYNTH_QUALITY    /* `quality' of synth as `width' of synth_ */

#endif
