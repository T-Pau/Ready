/* blipbuffer.c

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

Original C++ source:
 Blip_Buffer 0.4.0. http://www.slack.net/~ant/

partially reimplemented in C by Gergely Szasz for FUSE

*/

#include <config.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "blipbuffer.h"


static void _blip_synth_init( Blip_Synth_ * synth_, short *impulses );

inline void
blip_buffer_set_clock_rate( Blip_Buffer * buff, long cps )
{
  buff->factor_ = blip_buffer_clock_rate_factor( buff, buff->clock_rate_ =
                                                 cps );
}

inline long
blip_buffer_samples_avail( Blip_Buffer * buff )
{
  return ( long )( buff->offset_ >> BLIP_BUFFER_ACCURACY );
}

void
blip_synth_set_output( Blip_Synth * synth, Blip_Buffer * b )
{
  synth->impl.buf = b;
  synth->impl.last_amp = 0;
}

void
blip_synth_set_volume( Blip_Synth * synth, double v )
{
  _blip_synth_volume_unit( &synth->impl,
                           v * ( 1.0 /
                                 ( BLIP_SYNTH_RANGE <
                                   0 ? -( BLIP_SYNTH_RANGE ) :
                                   BLIP_SYNTH_RANGE ) ) );
}

#define BLIP_FWD( i )                     \
	t0 = i0 * delta + buf[fwd + i];   \
	t1 = imp[BLIP_RES * (i + 1)] * delta + buf[fwd + 1 + i]; \
	i0 = imp[BLIP_RES * (i + 2)];          \
	buf[fwd + i] = t0;                     \
	buf[fwd + 1 + i] = t1

#define BLIP_REV( r ) \
	t0 = i0 * delta + buf[rev - r];   \
	t1 = imp[BLIP_RES * r] * delta + buf[rev + 1 - r];   \
	i0 = imp[BLIP_RES * (r - 1)];          \
	buf[rev - r] = t0;                     \
	buf[rev + 1 - r] = t1

inline void
blip_synth_offset_resampled( Blip_Synth * synth, blip_resampled_time_t time,
                             int delta, Blip_Buffer * blip_buf )
{
  int phase, fwd, rev, mid;

  imp_t *imp;

  long *buf, i0, t0, t1;

  delta *= synth->impl.delta_factor;
  phase =
    ( int )( time >> ( BLIP_BUFFER_ACCURACY - BLIP_PHASE_BITS ) &
             ( BLIP_RES - 1 ) );
  imp = synth->impulses + BLIP_RES - phase;
  buf = blip_buf->buffer_ + ( time >> BLIP_BUFFER_ACCURACY );
  i0 = *imp;

  fwd = ( BLIP_WIDEST_IMPULSE_ - BLIP_SYNTH_QUALITY ) / 2;
  rev = fwd + BLIP_SYNTH_QUALITY - 2;

  BLIP_FWD( 0 );
  if( BLIP_SYNTH_QUALITY > 8 ) {
    BLIP_FWD( 2 );
  }
  if( BLIP_SYNTH_QUALITY > 12 ) {
    BLIP_FWD( 4 );
  }

  mid = BLIP_SYNTH_QUALITY / 2 - 1;
  t0 = i0 * delta + buf[fwd + mid - 1];
  t1 = imp[BLIP_RES * mid] * delta + buf[fwd + mid];
  imp = synth->impulses + phase;
  i0 = imp[BLIP_RES * mid];
  buf[fwd + mid - 1] = t0;
  buf[fwd + mid] = t1;

  if( BLIP_SYNTH_QUALITY > 12 ) {
    BLIP_REV( 6 );
  }
  if( BLIP_SYNTH_QUALITY > 8 ) {
    BLIP_REV( 4 );
  }
  BLIP_REV( 2 );

  t0 = i0 * delta + buf[rev];
  t1 = *imp * delta + buf[rev + 1];
  buf[rev] = t0;
  buf[rev + 1] = t1;
}

#undef BLIP_FWD
#undef BLIP_REV

void
blip_synth_update( Blip_Synth * synth, blip_time_t t, int amp )
{
  int delta = amp - synth->impl.last_amp;

  synth->impl.last_amp = amp;
  blip_synth_offset_resampled( synth,
                               t * synth->impl.buf->factor_ +
                               synth->impl.buf->offset_, delta,
                               synth->impl.buf );
}

int
_blip_synth_impulses_size( Blip_Synth_ * synth_ )
{
  return BLIP_RES / 2 * BLIP_SYNTH_WIDTH + 1;
}

void
blip_synth_set_treble_eq( Blip_Synth * synth, double treble )
{
  blip_eq_t eq = { 0.0, 0, 44100, 0 };

  eq.treble = treble;

  _blip_synth_treble_eq( &synth->impl, &eq );
}

#define BUFFER_EXTRA ( BLIP_WIDEST_IMPULSE_ + 2 )

static void
blip_buffer_init( Blip_Buffer * buff )
{
  buff->factor_ = LONG_MAX;
  buff->offset_ = 0;
  buff->buffer_ = NULL;
  buff->buffer_size_ = 0;
  buff->sample_rate_ = 0;
  buff->reader_accum = 0;
  buff->bass_shift = 0;
  buff->clock_rate_ = 0;
  buff->bass_freq_ = 16;
  buff->length_ = 0;
}

static void
blip_buffer_end( Blip_Buffer * buff )
{
  if( buff->buffer_ )
    free( buff->buffer_ );
  buff->buffer_ = NULL;
}

Blip_Buffer *
new_Blip_Buffer( void )
{
  Blip_Buffer *ret;

  ret = malloc( sizeof( Blip_Buffer ) );
  if( ret ) {
    blip_buffer_init( ret );
  }
  return ret;
}

void
delete_Blip_Buffer( Blip_Buffer ** buff )
{
  if( !*buff )
    return;

  blip_buffer_end( *buff );
  free( *buff );
  *buff = NULL;
}

static void
blip_synth_init( Blip_Synth * synth )
{
  synth->impulses =
    malloc( ( BLIP_RES * ( BLIP_SYNTH_QUALITY / 2 ) +
              1 ) * sizeof( imp_t ) * 4 );
  if( synth->impulses ) {
    _blip_synth_init( &synth->impl, ( short * )synth->impulses );       /* sorry, somewhere imp_t, somewhere short ???? */
  }
}

static void
blip_synth_end( Blip_Synth * synth )
{
  if( synth->impulses ) {
    free( synth->impulses );
    synth->impulses = NULL;
  }
}

Blip_Synth *
new_Blip_Synth( void )
{
  Blip_Synth *ret;

  ret = malloc( sizeof( Blip_Synth ) );
  if( ret ) {
    blip_synth_init( ret );
    if( !ret->impulses ) {
      free( ret );
      return NULL;
    }
  }
  return ret;
}

void
delete_Blip_Synth( Blip_Synth ** synth )
{
  if( !*synth )
    return;

  blip_synth_end( *synth );
  free( *synth );
  *synth = NULL;
}

void
blip_buffer_clear( Blip_Buffer * buff, int entire_buffer )
{
  buff->offset_ = 0;
  buff->reader_accum = 0;
  if( buff->buffer_ ) {
    long count =
      ( entire_buffer ? buff->
        buffer_size_ : blip_buffer_samples_avail( buff ) );
    memset( buff->buffer_, 0, ( count + BUFFER_EXTRA ) * sizeof( buf_t_ ) );
  }
}

blargg_err_t
blip_buffer_set_sample_rate( Blip_Buffer * buff, long new_rate, int msec )
{
  /* start with maximum length that resampled time can represent */
  long new_size = ( ULONG_MAX >> BLIP_BUFFER_ACCURACY ) - BUFFER_EXTRA - 64;

  if( msec != BLIP_MAX_LENGTH ) {
    long s = ( new_rate * ( msec + 1 ) + 999 ) / 1000;

    if( s < new_size )
      new_size = s;
  }

  if( buff->buffer_size_ != new_size ) {
    void *p =
      realloc( buff->buffer_,
               ( new_size + BUFFER_EXTRA ) * sizeof( buf_t_ ) );
    if( !p )
      return "Out of memory";
    buff->buffer_ = ( buf_t_ * ) p;
  }

  buff->buffer_size_ = new_size;

  /* update things based on the sample rate */
  buff->sample_rate_ = new_rate;
  buff->length_ = new_size * 1000 / new_rate - 1;
  if( buff->clock_rate_ )
    blip_buffer_set_clock_rate( buff, buff->clock_rate_ );
  blip_buffer_set_bass_freq( buff, buff->bass_freq_ );

  blip_buffer_clear( buff, BLIP_BUFFER_DEF_ENTIRE_BUFF );

  return 0;                     /*  success */
}

blip_resampled_time_t
blip_buffer_clock_rate_factor( Blip_Buffer * buff, long clock_rate )
{
  double ratio = ( double )buff->sample_rate_ / clock_rate;

  long factor = ( long )floor( ratio * ( 1L << BLIP_BUFFER_ACCURACY ) + 0.5 );

  return ( blip_resampled_time_t ) factor;
}

void
blip_buffer_set_bass_freq( Blip_Buffer * buff, int freq )
{
  int shift = 31;

  long f;

  buff->bass_freq_ = freq;
  if( freq > 0 ) {
    shift = 13;
    f = ( freq << 16 ) / buff->sample_rate_;
    while( ( f >>= 1 ) && --shift ) {};
  }

  buff->bass_shift = shift;
}

void
blip_buffer_end_frame( Blip_Buffer * buff, blip_time_t t )
{
  buff->offset_ += t * buff->factor_;
}

inline void
blip_buffer_remove_silence( Blip_Buffer * buff, long count )
{
  buff->offset_ -= ( blip_resampled_time_t ) count << BLIP_BUFFER_ACCURACY;
}

inline void
blip_buffer_remove_samples( Blip_Buffer * buff, long count )
{
  long remain;

  if( count ) {
    blip_buffer_remove_silence( buff, count );

    /*  copy remaining samples to beginning and clear old samples */
    remain = blip_buffer_samples_avail( buff ) + BUFFER_EXTRA;
    memmove( buff->buffer_, buff->buffer_ + count,
             remain * sizeof( buf_t_ ) );
    memset( buff->buffer_ + remain, 0, count * sizeof( buf_t_ ) );
  }
}

/*  Blip_Synth_ */

void
_blip_synth_init( Blip_Synth_ * synth_, short *p )
{
  synth_->impulses = p;
  synth_->volume_unit_ = 0.0;
  synth_->kernel_unit = 0;
  synth_->buf = NULL;
  synth_->last_amp = 0;
  synth_->delta_factor = 0;
}

#define PI 3.1415926535897932384626433832795029

static void
gen_sinc( float *out, int count, double oversample, double treble,
          double cutoff )
{
  int i;

  double maxh, rolloff, pow_a_n, to_angle;

  if( cutoff > 0.999 )
    cutoff = 0.999;
  if( treble < -300.0 )
    treble = -300.0;
  if( treble > 5.0 )
    treble = 5.0;

  maxh = 4096.0;
  rolloff = pow( 10.0, 1.0 / ( maxh * 20.0 ) * treble / ( 1.0 - cutoff ) );
  pow_a_n = pow( rolloff, maxh - maxh * cutoff );
  to_angle = PI / 2 / maxh / oversample;
  for( i = 0; i < count; i++ ) {
    double angle, c, cos_nc_angle, cos_nc1_angle, cos_angle, d, b, a;

    angle = ( ( i - count ) * 2 + 1 ) * to_angle;
    c = rolloff * cos( ( maxh - 1.0 ) * angle ) - cos( maxh * angle );
    cos_nc_angle = cos( maxh * cutoff * angle );
    cos_nc1_angle = cos( ( maxh * cutoff - 1.0 ) * angle );
    cos_angle = cos( angle );

    c = c * pow_a_n - rolloff * cos_nc1_angle + cos_nc_angle;
    d = 1.0 + rolloff * ( rolloff - cos_angle - cos_angle );
    b = 2.0 - cos_angle - cos_angle;
    a = 1.0 - cos_angle - cos_nc_angle + cos_nc1_angle;

    out[i] = ( float )( ( a * d + c * b ) / ( b * d ) );        /*  a / b + c / d */
  }
}

static void
blip_eq_generate( blip_eq_t * eq, float *out, int count )
{
  /* lower cutoff freq for narrow kernels with their wider transition band
     (8 points->1.49, 16 points->1.15) */
  int i;

  double to_fraction, cutoff;

  double oversample = BLIP_RES * 2.25 / count + 0.85;

  double half_rate = eq->sample_rate * 0.5;

  if( eq->cutoff_freq )
    oversample = half_rate / eq->cutoff_freq;

  cutoff = eq->rolloff_freq * oversample / half_rate;

  gen_sinc( out, count, BLIP_RES * oversample, eq->treble, cutoff );

  /* apply (half of) hamming window */
  to_fraction = PI / ( count - 1 );
  for( i = count; i--; )
    out[i] *= 0.54 - 0.46 * cos( i * to_fraction );

}

void
_blip_synth_adjust_impulse( Blip_Synth_ * synth_ )
{
  /* sum pairs for each phase and add error correction to end of first half */
  int size = _blip_synth_impulses_size( synth_ );

  int i, p, p2, error;

  for( p = BLIP_RES; p-- >= BLIP_RES / 2; ) {
    error = synth_->kernel_unit;
    p2 = BLIP_RES - 2 - p;
    for( i = 1; i < size; i += BLIP_RES ) {
      error -= synth_->impulses[i + p];
      error -= synth_->impulses[i + p2];
    }

    if( p == p2 )
      error /= 2;    /*  phase = 0.5 impulse uses same half for both sides */

    synth_->impulses[size - BLIP_RES + p] += error;
  }
}


void
_blip_synth_treble_eq( Blip_Synth_ * synth_, blip_eq_t * eq )
{
  double total, base_unit, rescale, sum, next, vol;

  int impulses_size, i;

  float fimpulse[BLIP_RES / 2 * ( BLIP_WIDEST_IMPULSE_ - 1 ) + BLIP_RES * 2];

  int half_size = BLIP_RES / 2 * ( BLIP_SYNTH_WIDTH - 1 );

  blip_eq_generate( eq, &fimpulse[BLIP_RES], half_size );

  /* need mirror slightly past center for calculation */
  for( i = BLIP_RES; i--; )
    fimpulse[BLIP_RES + half_size + i] =
      fimpulse[BLIP_RES + half_size - 1 - i];

  /* starts at 0 */
  for( i = 0; i < BLIP_RES; i++ )
    fimpulse[i] = 0.0f;

  /* find rescale factor */
  total = 0.0;
  for( i = 0; i < half_size; i++ )
    total += fimpulse[BLIP_RES + i];

/* double const base_unit = 44800.0 - 128 * 18;  allows treble up to +0 dB
   double const base_unit = 37888.0;  allows treble to +5 dB */
  base_unit = 32768.0;          /*  necessary for blip_unscaled to work */
  rescale = base_unit / 2 / total;
  synth_->kernel_unit = ( long )base_unit;

  /* integrate, first difference, rescale, convert to int */
  sum = 0.0;
  next = 0.0;
  impulses_size = _blip_synth_impulses_size( synth_ );

  for( i = 0; i < impulses_size; i++ ) {
    synth_->impulses[i] = ( short )floor( ( next - sum ) * rescale + 0.5 );
    sum += fimpulse[i];
    next += fimpulse[i + BLIP_RES];
  }

  _blip_synth_adjust_impulse( synth_ );

  /* volume might require rescaling */
  vol = synth_->volume_unit_;
  if( vol ) {
    synth_->volume_unit_ = 0.0;
    _blip_synth_volume_unit( synth_, vol );
  }
}

void
_blip_synth_volume_unit( Blip_Synth_ * synth_, double new_unit )
{
  if( new_unit != synth_->volume_unit_ ) {
    double factor;

    /* use default eq if it hasn't been set yet */
    if( !synth_->kernel_unit ) {
      blip_eq_t eq = { -8.0, 0, 44100, 0 };

      _blip_synth_treble_eq( synth_, &eq );
    }

    synth_->volume_unit_ = new_unit;
    factor = new_unit * ( 1L << BLIP_SAMPLE_BITS ) / synth_->kernel_unit;

    if( factor > 0.0 ) {
      int shift = 0;

      /* if unit is really small, might need to attenuate kernel */
      while( factor < 2.0 ) {
        shift++;
        factor *= 2.0;
      }

      if( shift ) {
        /* keep values positive to avoid round-towards-zero of sign-preserving
         * right shift for negative values */
        long offset = 0x8000 + ( 1 << ( shift - 1 ) );

        long offset2 = 0x8000 >> shift;

        int i;

        synth_->kernel_unit >>= shift;

        for( i = _blip_synth_impulses_size( synth_ ); i--; )
          synth_->impulses[i] =
            ( short )( ( ( synth_->impulses[i] + offset ) >> shift ) -
                       offset2 );

        _blip_synth_adjust_impulse( synth_ );
      }
    }
    synth_->delta_factor = ( int )floor( factor + 0.5 );
  }
}

long
blip_buffer_read_samples( Blip_Buffer * buff, blip_sample_t * out,
                          long max_samples, int stereo )
{
  long count = blip_buffer_samples_avail( buff );

  if( count > max_samples )
    count = max_samples;

  if( count ) {
    int sample_shift = BLIP_SAMPLE_BITS - 16;

    int my_bass_shift = buff->bass_shift;

    long accum = buff->reader_accum;

    buf_t_ *in = buff->buffer_;

    if( !stereo ) {
      int n;

      for( n = count; n--; ) {
        long s = accum >> sample_shift;

        accum -= accum >> my_bass_shift;
        accum += *in++;
        *out++ = ( blip_sample_t ) s;

        /* clamp sample */
        if( ( blip_sample_t ) s != s )
          out[-1] = ( blip_sample_t ) ( 0x7FFF - ( s >> 24 ) );
      }
    } else {
      int n;

      for( n = count; n--; ) {
        long s = accum >> sample_shift;

        accum -= accum >> my_bass_shift;
        accum += *in++;
        *out = ( blip_sample_t ) s;
        out += 2;

        /* clamp sample */
        if( ( blip_sample_t ) s != s )
          out[-2] = ( blip_sample_t ) ( 0x7FFF - ( s >> 24 ) );
      }
    }

    buff->reader_accum = accum;
    blip_buffer_remove_samples( buff, count );
  }

  return count;
}
