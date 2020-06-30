/* disk.c: Routines for handling disk images
   Copyright (c) 2007-2017 Gergely Szasz

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

#include <string.h>
#ifdef HAVE_STRINGS_STRCASECMP
#include <strings.h>
#endif      /* #ifdef HAVE_STRINGS_STRCASECMP */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libspectrum.h>

#include "bitmap.h"
#include "crc.h"
#include "disk.h"
#include "settings.h"
#include "trdos.h"
#include "ui/ui.h"
#include "utils.h"

/* The ordering of these strings must match the order of the 
 * disk_error_t enumeration in disk.h */

static const char * const disk_error[] = {
  "OK",				/* DISK_OK */
  "Feature not implemented",	/* DISK_IMPL */
  "Out of memory",		/* DISK_MEM */
  "Invalid disk geometry",	/* DISK_GEOM */
  "Cannot open disk image",	/* DISK_OPEN */
  "Unsupported file feature",	/* DISK_UNSUP */
  "Read only disk",		/* DISK_RDONLY */
  "Cannot close file",		/* DISK_CLOSE */
  "Cannot write disk image",	/* DISK_WRFILE */
  "Partially written file",	/* DISK_WRPART */

  "Unknown error code"		/* DISK_LAST_ERROR */
};

static const int disk_bpt[] = {
  6250,				/* AUTO assumes DD */
  5208,				/* 8" SD */
  10416,			/* 8" DD */
  3125,				/* SD */
  6250,				/* DD */
  6500,				/* DD+ e.g. Coin Op Hits */
  12500,			/* HD */
};

static unsigned char head[256];

typedef struct disk_gap_t {
  int gap;			/* gap byte */
  int sync;			/* sync byte */
  int sync_len;
  int mark;			/* mark byte 0xa1 for MFM -1 for MF */
  int len[4];
} disk_gap_t;

disk_gap_t gaps[] = {
  { 0x4e, 0x00, 12, 0xa1, {  0, 60, 22, 24 } },			/* MGT MFM */
  { 0x4e, 0x00, 12, 0xa1, {  0, 10, 22, 60 } },			/* TRD MFM */
  { 0xff, 0x00,  6, -1,   { 40, 26, 11, 27 } },			/* IBM3740 FM */
  { 0x4e, 0x00, 12, 0xa1, { 80, 50, 22, 54 } },			/* IBM34 MFM */
  { 0xff, 0x00,  6, -1,   {  0, 16, 11, 10 } },			/* MINIMAL FM */
  { 0x4e, 0x00, 12, 0xa1, {  0, 32, 22, 24 } },			/* MINIMAL MFM */
};

#define GAP_MGT_PLUSD	0
#define GAP_TRDOS	1
#define GAP_IBM3740	2
#define GAP_IBM34	3
#define GAP_MINIMAL_FM  4
#define GAP_MINIMAL_MFM 5

#define buffavail(buffer) ( buffer->file.length - buffer->index )
/* data buffer */
#define buff ( buffer->file.buffer + buffer->index )

typedef struct buffer_t {		/* to store buffer data */
  utils_file file;			/* buffer, length */
  size_t index;
} buffer_t;

void disk_update_tlens( disk_t *d );

const char *
disk_strerror( int error )
{
  if( error < 0 || error > DISK_LAST_ERROR )
    error = DISK_LAST_ERROR;
  return disk_error[ error ];
}

static int
buffread( void *data, size_t len, buffer_t *buffer )
{
  if( len > buffer->file.length - buffer->index )
    return 0;
  memcpy( data, buffer->file.buffer + buffer->index, len );
  buffer->index += len;
  return 1;
}

static int
buffseek( buffer_t *buffer, long offset, int whence )
{
  if( whence == SEEK_CUR )
    offset += buffer->index;
  if( offset >= buffer->file.length )
    return -1;
  buffer->index = offset;
  return 0;
}

static void
position_context_save( const disk_t *d, disk_position_context_t *c )
{
  c->track  = d->track;
  c->clocks = d->clocks;
  c->fm     = d->fm;
  c->weak   = d->weak;
  c->i      = d->i;
}

static void
position_context_restore( disk_t *d, const disk_position_context_t *c )
{
  d->track  = c->track;
  d->clocks = c->clocks;
  d->fm     = c->fm;
  d->weak   = c->weak;
  d->i      = c->i;
}

static int
id_read( disk_t *d, int *head, int *track, int *sector, int *length )
{
  int a1mark = 0;

  while( d->i < d->bpt ) {
    if( d->track[ d->i ] == 0xa1 && 
	bitmap_test( d->clocks, d->i ) ) {		/* 0xa1 with clock */
      a1mark = 1;
    } else if( d->track[ d->i ] == 0xfe && 
	( bitmap_test( d->clocks, d->i ) ||		/* 0xfe with clock */
	  a1mark ) ) {						/* or 0xfe with 0xa1 */
      d->i++;
      *track  = d->track[ d->i++ ];
      *head   = d->track[ d->i++ ];
      *sector = d->track[ d->i++ ];
      *length = d->track[ d->i++ ];
      d->i += 2;	/* skip CRC */
      return 1;
    } else {
      a1mark = 0;
    }
    d->i++;
  }
  return 0;
}

static int
datamark_read( disk_t *d, int *deleted )
{
  int a1mark = 0;

  while( d->i < d->bpt ) {
    if( d->track[ d->i ] == 0xa1 && 
	bitmap_test( d->clocks, d->i ) ) { /* 0xa1 with clock */
      a1mark = 1;
    } else if( d->track[ d->i ] >= 0xf8 && d->track[ d->i ] <= 0xfe &&
	       ( bitmap_test( d->clocks, d->i ) || a1mark ) ) {
      /* 0xfe with clock or 0xfe after 0xa1 mark */
      *deleted = d->track[ d->i ] == 0xf8 ? 1 : 0;
      d->i++;
      return 1;
    } else {
      a1mark = 0;
    }
    d->i++;
  }
  return 0;
}


static int
id_seek( disk_t *d, int sector )
{
  int h, t, s, b;
  d->i = 0;	/* start of the track */
  while( id_read( d, &h, &t, &s, &b ) ) {
    if( s == sector )
      return 1;
  }
  return 0;
}

/* seclen 00 -> 128, 01 -> 256 ... byte */
static int
data_write_file( disk_t *d, FILE *file, int seclen )
{
  int len = 0x80 << seclen;
  if( fwrite( &d->track[ d->i ], len, 1, file ) != 1 )
    return 1;
  return 0;
}

static int
savetrack( disk_t *d, FILE *file, int head, int track, 
	    int sector_base, int sectors, int seclen )
{
  int s;
  int del;

  DISK_SET_TRACK( d, head, track );
  d->i = 0;
  for( s = sector_base; s < sector_base + sectors; s++ ) {
    if( id_seek( d, s ) ) {
      if( datamark_read( d, &del ) ) {		/* write data if we have data */
	if( data_write_file( d, file, seclen ) )
	  return 1;
      }
    } else {
      return 1;
    }
  }
  return 0;
}

static int
saverawtrack( disk_t *d, FILE *file, int head, int track  )
{
  int h, t, s, seclen;
  int del;

  DISK_SET_TRACK( d, head, track );
  d->i = 0;
  while( id_read( d, &h, &t, &s, &seclen ) ) {
    if( datamark_read( d, &del ) ) {		/* write data if we have data */
      if( data_write_file( d, file, seclen ) )
	return 1;
    }
  }
  return 0;
}

#define DISK_ID_NOTMATCH 1
#define DISK_SECLEN_VARI 2
#define DISK_SPT_VARI 4
#define DISK_SBASE_VARI 8
#define DISK_MFM_VARI 16
#define DISK_DDAM 32
#define DISK_CORRUPT_SECTOR 64
#define DISK_UNFORMATTED_TRACK 128
#define DISK_FM_DATA 256
#define DISK_WEAK_DATA 512

static int
guess_track_geom( disk_t *d, int head, int track, int *sector_base,
		    int *sectors, int *seclen, int *mfm )
{
  int r = 0;
  int h, t, s, sl;
  int del = 0;
  *sector_base = -1;
  *sectors = 0;
  *seclen = -1;
  *mfm = -1;

  DISK_SET_TRACK( d, head, track );
  d->i = 0;
  while( id_read( d, &h, &t, &s, &sl ) ) {
    if( *sector_base == -1 )
      *sector_base = s;
    if( *seclen == -1 )
      *seclen = sl;
    if( *mfm == -1 )
      *mfm = d->track[ d->i ] == 0x4e ? 1 : 0;	/* not so robust */
    if( !datamark_read( d, &del ) )
      r |= DISK_CORRUPT_SECTOR;
    if( t != track )
      r |= DISK_ID_NOTMATCH;
    if( s < *sector_base )
      *sector_base = s;
    if( sl != *seclen ) {
      r |= DISK_SECLEN_VARI;
      if( sl > *seclen )
	*seclen = sl;
    }
    if( del )
      r |= DISK_DDAM;
    *sectors += 1;
  }
  return r;
}

static void
update_tracks_mode( disk_t *d )
{
  int i, j, bpt;
  int mfm, fm, weak;

  for( i = 0; i < d->cylinders * d->sides; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    mfm = 0, fm = 0, weak = 0;
    bpt = d->track[-3] + 256 * d->track[-2];
    for( j = DISK_CLEN( bpt ) - 1; j >= 0; j-- ) {
      mfm  |= ~d->fm[j];
      fm   |= d->fm[j];
      weak |= d->weak[j];
    }
    if( mfm && !fm ) d->track[-1] = 0x00;
    if( !mfm && fm ) d->track[-1] = 0x01;
    if( mfm &&  fm ) d->track[-1] = 0x02;
    if( weak ) {
      d->track[-1] |= 0x80;
      d->have_weak = 1;
    }
  }
}

static int
check_disk_geom( disk_t *d, int *sector_base, int *sectors,
		 int *seclen, int *mfm, int *unf )
{
  int h, t, s, slen, sbase, m;
  int r = 0;

  DISK_SET_TRACK_IDX( d, 0 );
  d->i = 0;
  *sector_base = -1;
  *sectors = -1;
  *seclen = -1;
  *mfm = -1;
  *unf = -1;
  for( t = 0; t < d->cylinders; t++ ) {
    for( h = 0; h < d->sides; h++ ) {
      r |= ( d->track[-1] & 0x80 ) ? DISK_WEAK_DATA : 0;
      r |= ( d->track[-1] & 0x03 ) == 0x02 ? DISK_MFM_VARI : 0;
      r |= ( d->track[-1] & 0x03 ) == 0x01 ? DISK_FM_DATA : 0;
      r |= guess_track_geom( d, h, t, &sbase, &s, &slen, &m );
      if( *sector_base == -1 )
	*sector_base = sbase;
      if( *sectors == -1 )
	*sectors = s;
      if( *seclen == -1 )
	*seclen = slen;
      if( *mfm == -1 )
	*mfm = m;
      if( sbase == -1 ) {		/* unformatted */
        if( *unf == -1 && h > 0 ) *unf = -2;
        if( *unf == -1 ) *unf = t;
        continue;
      }
      if( *unf > -1 ) *unf = -2;
      if( sbase != *sector_base ) {
	r |= DISK_SBASE_VARI;
	if( sbase < *sector_base )
	  *sector_base = sbase;
      }
      if( s != *sectors ) {
	r |= DISK_SPT_VARI;
	if( s > *sectors )
	  *sectors = s;
      }
      if( slen != *seclen ) {
	r |= DISK_SECLEN_VARI;
	if( slen > *seclen )
	  *seclen = slen;
      }
      if( m != *mfm ) {
	r |= DISK_MFM_VARI;
	*mfm = 1;
      }
    }
  }
  if( *unf == -2 ) {
    r |= DISK_UNFORMATTED_TRACK;
    *unf = -1;
  }

  return r;
}

static int
gap_add( disk_t *d, int gap, int gaptype )
{
  disk_gap_t *g = &gaps[ gaptype ];
  if( d->i + g->len[gap]  >= d->bpt )  /* too many data bytes */
    return 1;
/*-------------------------------- given gap --------------------------------*/
  memset( d->track + d->i, g->gap,  g->len[gap] ); d->i += g->len[gap];
  return 0;
}

static int
preindex_len( disk_t *d, int gaptype )		/* preindex gap and index mark */
{
  disk_gap_t *g = &gaps[ gaptype ];
  return g->len[0] + g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 1;
}

/*
  [ ....GAP.... ] [ ... SYNC ... ] [ . MARK . ]
  |------------------------------------------->
		Preindex 
*/
static int
preindex_add( disk_t *d, int gaptype )		/* preindex gap and index mark */
{
  disk_gap_t *g = &gaps[ gaptype ];
  if( d->i + preindex_len( d, gaptype ) >= d->bpt )
    return 1;
/*------------------------------ pre-index gap -------------------------------*/
  if( gap_add( d, 0, gaptype ) )
    return 1;
/*------------------------------   sync    ---------------------------*/
  memset( d->track + d->i, g->sync, g->sync_len ); d->i += g->sync_len;
  if( g->mark >= 0 ) {
    memset( d->track + d->i , g->mark, 3 );
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
  }
/*------------------------------     mark     ------------------------------*/
  if( g->mark < 0 )				/* FM */
    bitmap_set( d->clocks, d->i ); 		/* set clock mark */
  d->track[ d->i++ ] = 0xfc;			/* index mark */
  return 0;
}

static int
postindex_len( disk_t *d, int gaptype )		/* preindex gap and index mark */
{
  disk_gap_t *g = &gaps[ gaptype ];
  return g->len[1];
}

static int
postindex_add( disk_t *d, int gaptype )		/* postindex gap */
{
  return gap_add( d, 1, gaptype );
}

static int
gap4_add( disk_t *d, int gaptype )
{
  int len = d->bpt - d->i;
  disk_gap_t *g = &gaps[ gaptype ];

  if( len < 0 ) {
    return 1;
  }
/*------------------------------     GAP IV     ------------------------------*/
  memset( d->track + d->i, g->gap, len ); /* GAP IV fill until end of track */
  d->i = d->bpt;
  return 0;
}

#define SECLEN_128 0x00
#define SECLEN_256 0x01
#define SECLEN_512 0x02
#define SECLEN_1024 0x03
#define CRC_OK 0
#define CRC_ERROR 1

/*
  [ ....GAP.... ] [ ... SYNC ... ] [ . MARK . ] [ .. DATA .. ] [ . CRC . ]
                 |------------------------------------------------------->
		                    ID 
*/
static int
id_add( disk_t *d, int h, int t, int s, int l, int gaptype, int crc_error )
{
  libspectrum_word crc = 0xffff;
  disk_gap_t *g = &gaps[ gaptype ];
  if( d->i + g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 7 >= d->bpt )
    return 1;
/*------------------------------   sync    ---------------------------*/
  memset( d->track + d->i, g->sync, g->sync_len ); d->i += g->sync_len;
  if( g->mark >= 0 ) {
    memset( d->track + d->i , g->mark, 3 );
    bitmap_set( d->clocks, d->i ); d->i++;
    crc = crc_fdc( crc, g->mark );
    bitmap_set( d->clocks, d->i ); d->i++;
    crc = crc_fdc( crc, g->mark );
    bitmap_set( d->clocks, d->i ); d->i++;
    crc = crc_fdc( crc, g->mark );
  }
/*------------------------------     mark     ------------------------------*/
  if( g->mark < 0 )			/* FM */
    bitmap_set( d->clocks, d->i ); 	/* set clock mark */
  d->track[ d->i++ ] = 0xfe;		/* ID mark */
  crc = crc_fdc( crc, 0xfe );
/*------------------------------     header     ------------------------------*/
  d->track[ d->i++ ] = t; crc = crc_fdc( crc, t );
  d->track[ d->i++ ] = h; crc = crc_fdc( crc, h );
  d->track[ d->i++ ] = s; crc = crc_fdc( crc, s );
  d->track[ d->i++ ] = l; crc = crc_fdc( crc, l );
  d->track[ d->i++ ] = crc >> 8;
  if( crc_error ) {
    d->track[ d->i++ ] = (~crc) & 0xff;	/* record a CRC error */
  } else {
    d->track[ d->i++ ] = crc & 0xff;	/* CRC */
  }
/*------------------------------     GAP II     ------------------------------*/
  return gap_add( d, 2, gaptype );
}

/*
  [ ....GAP.... ] [ ... SYNC ... ] [ . MARK . ] [ .. DATA .. ] [ . CRC . ]
 |-------------------------------------------->
                  datamark
*/
static int
datamark_add( disk_t *d, int ddam, int gaptype )
{
  disk_gap_t *g = &gaps[ gaptype ];
  if( d->i + g->len[2] + g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 1 >= d->bpt )
    return 1;
/*------------------------------   sync    ---------------------------*/
  memset( d->track + d->i, g->sync, g->sync_len ); d->i += g->sync_len;
  if( g->mark >= 0 ) {
    memset( d->track + d->i , g->mark, 3 );
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
    bitmap_set( d->clocks, d->i ); d->i++;
  }
/*------------------------------     mark     ------------------------------*/
  if( g->mark < 0 )			/* FM */
    bitmap_set( d->clocks, d->i ); 	/* set clock mark */
  d->track[ d->i++ ] = ddam ? 0xf8 : 0xfb;	/* DATA mark 0xf8 -> deleted data */
  return 0;
}

#define NO_DDAM 0
#define DDAM 1
#define NO_AUTOFILL -1
/* copy data from *buffer and update *buffer->index */  
/* if 'buffer' == NULL, then copy data bytes from 'data' */  
static int
data_add( disk_t *d, buffer_t *buffer, unsigned char *data, int len, int ddam,
		int gaptype, int crc_error, int autofill, int *start_data )
{
  int length;
  libspectrum_word crc = 0xffff;
  disk_gap_t *g = &gaps[ gaptype ];

  if( datamark_add( d, ddam, gaptype ) )
    return 1;

  if( g->mark >= 0 ) {
    crc = crc_fdc( crc, g->mark );
    crc = crc_fdc( crc, g->mark );
    crc = crc_fdc( crc, g->mark );
  }
  crc = crc_fdc( crc, ddam ? 0xf8 : 0xfb );	/* deleted or normal */
  if( len < 0 )
    goto header_crc_error;			/* CRC error */
  if( d->i + len + 2 >= d->bpt )  		/* too many data bytes */
    return 1;
/*------------------------------      data      ------------------------------*/
  if( start_data != NULL ) *start_data = d->i;	/* record data start position */
  if( buffer == NULL ) {
    memcpy( d->track + d->i, data, len );
    length = len;
  } else {
    length = buffavail( buffer );
    if( length > len ) length = len;
    buffread( d->track + d->i, length, buffer );
  }
  if( length < len ) {	/* autofill with 'autofill' */
    if( autofill < 0 )
      return 1;
    while( length < len ) {
      d->track[ d->i + length ] = autofill;
      length++;
    }
  }
  length = 0;
  while( length < len ) {	/* calculate CRC */
    crc = crc_fdc( crc, d->track[ d->i ] );
    d->i++;
    length++;
  }
  if( crc_error ) crc ^= 1;	/* mess up CRC */
  d->track[ d->i++ ] = crc >> 8; d->track[ d->i++ ] = crc & 0xff;    /* CRC */
/*------------------------------     GAP III    ------------------------------*/
header_crc_error:
  return ( gap_add( d, 3, gaptype ) );
}

static int
calc_sectorlen( int mfm, int sector_length, int gaptype )
{
  int len = 0;
  disk_gap_t *g = &gaps[ gaptype ];
  
/*------------------------------     ID        ------------------------------*/
  len += g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 7;
/*------------------------------     GAP II    ------------------------------*/
  len += g->len[2];
/*---------------------------------  data   ---------------------------------*/
  len += g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 1;		/* DAM */
  len += sector_length;
  len += 2;		/* CRC */
/*------------------------------    GAP III    ------------------------------*/
  len += g->len[3];
  return len;
}

static int
calc_lenid( int sector_length )
{
  int id = 0;

  while( sector_length > 0x80 ) {
    id++;
    sector_length >>= 1;
  }

  return id;
}

#define NO_INTERLEAVE 1
#define INTERLEAVE_2 2
#define INTERLEAVE_OPUS 13
#define NO_PREINDEX 0
#define PREINDEX 1

static int
trackgen( disk_t *d, buffer_t *buffer, int head, int track,
	  int sector_base, int sectors, int sector_length, int preindex,
	  int gap, int interleave, int autofill )
{
  int i, s, pos;
  int slen = calc_sectorlen( ( d->density != DISK_SD && d->density != DISK_8_SD ),
			     sector_length, gap );
  int idx;

  d->i = 0;
  DISK_SET_TRACK( d, head, track );
  if( preindex && preindex_add( d, gap ) )
    return 1;
  if( postindex_add( d, gap ) )
    return 1;

  idx = d->i;
  pos = i = 0;
  for( s = sector_base; s < sector_base + sectors; s++ ) {
    d->i = idx + pos * slen;
    if( id_add( d, head, track, s, calc_lenid( sector_length ), gap, CRC_OK ) )
      return 1;
    if( data_add( d, buffer, NULL, sector_length, NO_DDAM, gap, CRC_OK, autofill, NULL ) )
      return 1;
    pos += interleave;
    if( pos >= sectors ) {	/* wrap around */
      pos -= sectors;
      if( pos <= i ) {		/* we fill this pos already */
        pos++;			/* skip one more pos */
        i++;
      }
    }
  }
  d->i = idx + sectors * slen;
  return gap4_add( d, gap );
}

/* close and destroy a disk structure and data */
void
disk_close( disk_t *d )
{
  if( d->data != NULL ) {
    libspectrum_free( d->data );
    d->data = NULL;
  }
  if( d->filename != NULL ) {
    libspectrum_free( d->filename );
    d->filename = NULL;
  }
  d->type = DISK_TYPE_NONE;
}

/*
 *  if d->density == DISK_DENS_AUTO => 
 *                            use d->tlen if d->bpt == 0
 *                             or use d->bpt to determine d->density
 *  or use d->density
 */
static int
disk_alloc( disk_t *d )
{
  size_t dlen;

  if( d->density != DISK_DENS_AUTO ) {
    d->bpt = disk_bpt[ d->density ];
  } else if( d->bpt > 12500 ) {
    return d->status = DISK_UNSUP;
  } else if( d->bpt > 10416 ) {
    d->density = DISK_HD;
    d->bpt = disk_bpt[ DISK_HD ];
  } else if( d->bpt > 6500 ) {
    d->density = DISK_8_DD;
    d->bpt = disk_bpt[ DISK_8_DD ];
  } else if( d->bpt > 6250 ) {
    d->density = DISK_DD_PLUS;
    d->bpt = disk_bpt[ DISK_DD_PLUS ];
  } else if( d->bpt > 5208 ) {
    d->density = DISK_DD;
    d->bpt = disk_bpt[ DISK_DD ];
  } else if( d->bpt > 3125 ) {
    d->density = DISK_8_SD;
    d->bpt = disk_bpt[ DISK_8_SD ];
  } else if( d->bpt > 0 ) {
    d->density = DISK_SD;
    d->bpt = disk_bpt[ DISK_SD ];
  }

  if( d->bpt > 0 )
    d->tlen = 4 + d->bpt + 3 * DISK_CLEN( d->bpt );

  dlen = d->sides * d->cylinders * d->tlen;	/* track len with clock and other marks */
  if( dlen == 0 ) return d->status = DISK_GEOM;

  d->data = libspectrum_new0( libspectrum_byte, dlen );

  return d->status = DISK_OK;
}

/* create a new unformatted disk  */
int
disk_new( disk_t *d, int sides, int cylinders,
	     disk_dens_t density, disk_type_t type )
{
  d->filename = NULL;
  if( density < DISK_DENS_AUTO || density > DISK_HD ||	/* unknown density */
      type <= DISK_TYPE_NONE || type >= DISK_TYPE_LAST || /* unknown type */
      sides < 1 || sides > 2 ||				/* 1 or 2 side */
      cylinders < 35 || cylinders > 83 )		/* 35 .. 83 cylinder */
    return d->status = DISK_GEOM;

  d->type = type;
  d->density = density == DISK_DENS_AUTO ? DISK_DD : density;
  d->sides = sides;
  d->cylinders = cylinders;

  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  d->wrprot = 0;
  d->dirty = 1;
  disk_update_tlens( d );
  return d->status = DISK_OK;
}

static int
alloc_uncompress_buffer( unsigned char **buffer, int length )
{
  unsigned char *b;

  if( *buffer != NULL )				/* return if allocated */
    return 0;
  b = libspectrum_new0( unsigned char, length );
  if( b == NULL )
    return 1;
  *buffer = b;
  return 0;
}

int disk_preformat( disk_t *d )
{
  buffer_t buffer;

  buffer.file.length = 0;
  buffer.index = 0;

  if( d->sides == 2 ) {
    if( trackgen( d, &buffer, 1, 0, 0xff, 1, 128,
                  NO_PREINDEX, GAP_MINIMAL_MFM, NO_INTERLEAVE, 0xff ) )
    return DISK_GEOM;

    if( trackgen( d, &buffer, 1, 2, 0xfe, 1, 128,
                  NO_PREINDEX, GAP_MINIMAL_MFM, NO_INTERLEAVE, 0xff ) )
    return DISK_GEOM;
  }

  if( trackgen( d, &buffer, 0, 0, 0xff, 1, 128,
		      NO_PREINDEX, GAP_MINIMAL_MFM, NO_INTERLEAVE, 0xff ) )
    return DISK_GEOM;
  if( trackgen( d, &buffer, 0, 2, 0xfe, 1, 128,
		      NO_PREINDEX, GAP_MINIMAL_MFM, NO_INTERLEAVE, 0xff ) )
    return DISK_GEOM;

  return DISK_OK;
}

/* open a disk image */
#define GEOM_CHECK \
    if( d->sides < 1 || d->sides > 2 || \
       d->cylinders < 1 || d->cylinders > 85 ) return d->status = DISK_GEOM

#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
static int
udi_read_compressed( const libspectrum_byte *buffer,
		   size_t compr_size, size_t uncompr_size,
		   libspectrum_byte **data, size_t *data_size )
{
  libspectrum_error error;
  libspectrum_byte *tmp;
  size_t olength = uncompr_size;

  tmp = NULL;

  error = libspectrum_zlib_inflate( buffer, compr_size, &tmp, &olength );
  if( error ) return error;

  if( *data_size < uncompr_size ) {
    *data = libspectrum_renew( libspectrum_byte, *data, uncompr_size );
    *data_size = uncompr_size;
  }
  memcpy( *data, tmp, uncompr_size );
  libspectrum_free( tmp );

  return 0;
}

static int
udi_write_compressed( const libspectrum_byte *buffer,
		      size_t uncompr_size, size_t *compr_size,
		      libspectrum_byte **data, size_t *data_size )
{
  libspectrum_error error;
  libspectrum_byte *tmp;

  tmp = NULL;
  error = libspectrum_zlib_compress( buffer, uncompr_size,
                                     &tmp, compr_size );
  if( error ) return error;

  if( *data_size < *compr_size ) {
    *data = libspectrum_renew( libspectrum_byte, *data, *compr_size );
    *data_size = *compr_size;
  }
  memcpy( *data, tmp, *compr_size );
  libspectrum_free( tmp );

  return LIBSPECTRUM_ERROR_NONE;
}
#endif			/* #ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */

static void
udi_pack_tracks( disk_t *d )
{
  int i, tlen, clen, ttyp;
  libspectrum_byte *tmp;

  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    tmp = d->track;
    ttyp = tmp[-1];
    tlen = tmp[-3] + 256 * tmp[-2];
    clen = DISK_CLEN( tlen );
    tmp += tlen;
    /* copy clock if needed */
    if( tmp != d->clocks )
      memcpy( tmp, d->clocks, clen );
    if( ttyp == 0x00 || ttyp == 0x01 ) continue;
    tmp += clen;
    if( ttyp & 0x02 ) {		/* copy FM marks */
      if( tmp != d->fm )
        memcpy( tmp, d->fm, clen );
      tmp += clen;
    }
    if( ! ( ttyp & 0x80 ) ) continue;
    if( tmp != d->weak )		/* copy WEAK marks*/
      memcpy( tmp, d->weak, clen );
  }
}

static void
udi_unpack_tracks( disk_t *d )
{
  int i, tlen, clen, ttyp;
  libspectrum_byte *tmp;
  libspectrum_byte mask[] = { 0xff, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    tmp = d->track;
    ttyp = tmp[-1];
    tlen = tmp[-3] + 256 * tmp[-2];
    clen = DISK_CLEN( tlen );
    tmp += tlen;
    if( ttyp & 0x80 ) tmp += clen;
    if( ttyp & 0x02 ) tmp += clen;
    if( ( ttyp & 0x80 ) ) {	/* copy WEAK marks*/
      if( tmp != d->weak )
        memcpy( d->weak, tmp, clen );
      tmp -= clen;
    } else {			/* clear WEAK marks*/
      memset( d->weak, 0, clen );
    }
    if( ttyp & 0x02 ) {		/* copy FM marks */
      if( tmp != d->fm )
        memcpy( d->fm, tmp, clen );
      tmp -= clen;
    } else {			/* set/clear FM marks*/
      memset( d->fm, ttyp & 0x01 ? 0xff : 0, clen );
      if( tlen % 8 ) {		/* adjust last byte */
        d->fm[clen - 1] &= mask[ tlen % 8 ];
      }
    }
    /* copy clock if needed */
    if( tmp != d->clocks )
      memcpy( d->clocks, tmp, clen );
  }
}

/* calculate track len from type, if type eq. 0x00/0x01/0x02/0x80/0x81/0x82
   !!! not for 0x83 nor 0xf0 !!!
*/
#define UDI_TLEN( type, bpt ) ( ( bpt ) + DISK_CLEN( bpt ) * ( 1 + \
					( type & 0x02 ? 1 : 0 ) + \
					( type & 0x80 ? 1 : 0 ) ) )

static int
udi_uncompress_tracks( disk_t *d )
{
  int i;
  libspectrum_byte *data = NULL;
#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  size_t data_size = 0;
  int bpt, tlen, clen, ttyp;
#endif			/* #ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */

  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    if( d->track[-1] != 0xf0 ) continue;	/* if not compressed */

#ifndef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
    /* if libspectrum cannot support */
    return d->status = DISK_UNSUP;
#else 			/* #ifndef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */
    clen = d->track[-3] + 256 * d->track[-2] + 1;
    ttyp = d->track[0];				/* compressed track type   */
    bpt = d->track[1] + 256 * d->track[2];	/* compressed track len... */
    tlen = UDI_TLEN( ttyp, bpt );
    d->track[-1] = ttyp;
    d->track[-3] = d->track[1];
    d->track[-2] = d->track[2];
    if( udi_read_compressed( d->track + 3, clen, tlen, &data, &data_size ) ) {
      if( data ) libspectrum_free( data );
      return d->status = DISK_UNSUP;
    }
    memcpy( d->track, data, tlen );		/* read track */
#endif			/* #ifndef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */
  }
  if( data ) libspectrum_free( data );
  return DISK_OK;
}

#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
static int
udi_compress_tracks( disk_t *d )
{
  int i, tlen;
  libspectrum_byte *data = NULL;
  size_t clen, data_size = 0;

  for( i = 0; i < d->sides * d->cylinders; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    if( d->track[-1] == 0xf0 ) continue;	/* already compressed??? */

    tlen = UDI_TLEN( d->track[-1], d->track[-3] + 256 * d->track[-2] );
	/* if fail to compress, skip ... */
    if( udi_write_compressed( d->track, tlen, &clen, &data, &data_size ) ||
							clen < 1 ) continue;
	/* if compression too large, skip... */
    if( clen > 65535 || clen >= tlen ) continue;
    d->track[0] = d->track[-1];			/* track type... */
    d->track[1] = d->track[-3];			/* compressed track len... */
    d->track[2] = d->track[-2];			/* compressed track len... */
    memcpy( d->track + 3, data, clen );		/* read track */
    clen--;
    d->track[-1] = 0xf0;
    d->track[-3] = clen & 0xff;
    d->track[-2] = ( clen >> 8 ) & 0xff;
  }
  if( data ) libspectrum_free( data );
  return DISK_OK;
}
#endif			/* #ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */

static int
open_udi( buffer_t *buffer, disk_t *d )
{
  int i, bpt, ttyp, tlen, error;
  size_t eof;
  libspectrum_dword crc;

  crc = ~(libspectrum_dword) 0;

  /* check file length */
  eof = buff[4] + 256 * buff[5] + 65536 * buff[6] + 16777216 * buff[7];
  if( eof != buffer->file.length - 4 )
    return d->status = DISK_OPEN;
  /* check CRC32 */
  for( i = 0; i < eof; i++ )
    crc = crc_udi( crc, buff[i] );
  if( crc != buff[eof] + 256 * buff[eof + 1] + 65536 * buff[eof + 2] +
						16777216 * buff[eof + 3] )
    return d->status = DISK_OPEN;

  d->sides = buff[10] + 1;
  d->cylinders = buff[9] + 1;
  GEOM_CHECK;
  d->density = DISK_DENS_AUTO;
  buffer->index = 16;
  d->bpt = 0;

  /* scan file for the longest track */
  for( i = 0; buffer->index < eof; i++ ) {
    if( buffavail( buffer ) < 3 )
      return d->status = DISK_OPEN;
    ttyp = buff[0];
    if( ttyp != 0x00 && ttyp != 0x01 && ttyp != 0x02 && ttyp != 0x80 &&
        ttyp != 0x81 && ttyp != 0x82 && ttyp != 0x83 && ttyp != 0xf0 )
      return d->status = DISK_UNSUP;

    /* if libspectrum cannot suppot*/
#ifndef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
    if( ttyp == 0xf0 ) d->status = DISK_UNSUP;
#endif			/* #ifndef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */
    if( ttyp == 0x83 ) {			/* multiple read */
      if( i == 0 ) return d->status = DISK_GEOM;	/* cannot be first track */
      i--; bpt = 0;					/* not a real track */
      tlen = buff[1] + 256 * buff[2];		/* current track len... */
      tlen = ( tlen & 0xfff8 ) * ( tlen & 0x07 );
    } else if( ttyp == 0xf0 ) {			/* compressed track */
      if( buffavail( buffer ) < 7 )
        return d->status = DISK_OPEN;
      bpt = buff[4] + 256 * buff[5];
      tlen = 7 + buff[1] + 256 * buff[2];
    } else {
      bpt = buff[1] + 256 * buff[2];		/* current track len... */
      tlen = 3 + UDI_TLEN( ttyp, bpt );
    }
    if( bpt > d->bpt )
      d->bpt = bpt;
    if( buffseek( buffer, tlen, SEEK_CUR ) == -1 )
      return d->status = DISK_OPEN;
  }

  if( d->bpt == 0 )
    return d->status = DISK_GEOM;

  bpt = d->bpt;		/* save the maximal value */
  d->tlen = 3 + bpt + 3 * DISK_CLEN( bpt );
  d->bpt = 0;		/* we know exactly the track len... */
  if( disk_alloc( d ) != DISK_OK )
    return d->status;
  d->bpt = bpt;		/* restore the maximal byte per track */
  buffer->index = 16;

  for( i = 0; buffer->index < eof; i++ ) {
    DISK_SET_TRACK_IDX( d, i );
    ttyp = buff[0];
    bpt = buff[1] + 256 * buff[2];		/* current track len... */

    memset( d->track, 0x4e, d->bpt );		/* fillup */
						/* read track + clocks */
    if( ttyp == 0x83 ) {			/* multiple read */
      i--;					/* not a real track */
      DISK_SET_TRACK_IDX( d, i );		/* back to previouse track */
      d->weak += buff[3] + 256 * buff[4];	/* add offset to weak */
      tlen = ( buff[1] + 256 * buff[2] ) >> 3;	/* weak len in bytes */
      for( tlen--; tlen >= 0; tlen-- )
        d->weak[tlen] = 0xff;
      tlen = buff[1] + 256 * buff[2];		/* current track len... */
      tlen = ( tlen & 0xfff8 ) * ( tlen & 0x07 );
      buffseek( buffer, tlen, SEEK_CUR );
    } else {
      if( ttyp == 0xf0 )			/* compressed */
        tlen = bpt + 4;
      else
        tlen = UDI_TLEN( ttyp, bpt );
      d->track[-1] = ttyp;
      d->track[-3] = buff[1];
      d->track[-2] = buff[2];
      buffer->index += 3;
      buffread( d->track, tlen, buffer );	/* first read data */
    }
  }
  error = udi_uncompress_tracks( d );
  if( error ) return error;
  udi_unpack_tracks( d );

  return d->status = DISK_OK;
}

static int
open_img_mgt_opd( buffer_t *buffer, disk_t *d )
{
  int i, j, sectors, seclen;

  buffer->index = 0;

  /* guess geometry of disk:
   * 2*80*10*512, 1*80*10*512, 1*40*10*512, 1*40*18*256, 1*80*18*256,
   * 2*80*18*256
   */
  if( buffer->file.length == 2*80*10*512 ) {
    d->sides = 2; d->cylinders = 80; sectors = 10; seclen = 512;
  } else if( buffer->file.length == 1*80*10*512 ) {
    /* we cannot distinguish between a single sided 80 track image
     * and a double sided 40 track image (2*40*10*512) */
    d->sides = 1; d->cylinders = 80; sectors = 10; seclen = 512;
  } else if( buffer->file.length == 1*40*10*512 ) {
    d->sides = 1; d->cylinders = 40; sectors = 10; seclen = 512;
  } else if( buffer->file.length == 1*40*18*256 ) {
    d->sides = 1; d->cylinders = 40; sectors = 18; seclen = 256;
  } else if( buffer->file.length == 1*80*18*256 ) {
    /* we cannot distinguish between a single sided 80 track image
     * and a double sided 40 track image (2*40*18*256) */
    d->sides = 1; d->cylinders = 80; sectors = 18; seclen = 256;
  } else if( buffer->file.length == 2*80*18*256 ) {
    d->sides = 2; d->cylinders = 80; sectors = 18; seclen = 256;
  } else {
    return d->status = DISK_GEOM;
  }

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  if( d->type == DISK_IMG ) {	/* IMG out-out */
    for( j = 0; j < d->sides; j++ ) {
      for( i = 0; i < d->cylinders; i++ ) {
	if( trackgen( d, buffer, j, i, 1, sectors, seclen,
		      NO_PREINDEX, GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	  return d->status = DISK_GEOM;
      }
    }
  } else {			/* MGT / OPD alt */
    for( i = 0; i < d->cylinders; i++ ) {
      for( j = 0; j < d->sides; j++ ) {
        if( trackgen( d, buffer, j, i, d->type == DISK_MGT ? 1 : 0, sectors, seclen,
		      NO_PREINDEX, GAP_MGT_PLUSD,
		      d->type == DISK_MGT ? NO_INTERLEAVE : INTERLEAVE_OPUS, NO_AUTOFILL ) )
	  return d->status = DISK_GEOM;
      }
    }
  }

  return d->status = DISK_OK;
}

static int
open_d40_d80( buffer_t *buffer, disk_t *d )
{
  int i, j, sectors, seclen;

  if( buffavail( buffer ) < 180 )
    return d->status = DISK_OPEN;

  /* guess geometry of disk */
  d->sides =     buff[0xb1] & 0x10 ? 2 : 1;
  d->cylinders = buff[0xb2];
  sectors =      buff[0xb3];

  if( d->sides < 1 || d->sides > 2 || d->cylinders > 83 || sectors > 127 )
    return d->status = DISK_GEOM;

  seclen = 512;

  buffer->index = 0;

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  for( i = 0; i < d->cylinders; i++ ) {
      for( j = 0; j < d->sides; j++ ) {
        if( trackgen( d, buffer, j, i, 1, sectors, seclen,
		      NO_PREINDEX, GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	  return d->status = DISK_GEOM;
      }
  }

  return d->status = DISK_OK;
}

static int
open_sad( buffer_t *buffer, disk_t *d, int preindex )
{
  int i, j, sectors, seclen;

  d->sides = buff[18];
  d->cylinders = buff[19];
  GEOM_CHECK;
  sectors = buff[20];
  seclen = buff[21] * 64;
  buffer->index = 22;

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  for( j = 0; j < d->sides; j++ ) {
    for( i = 0; i < d->cylinders; i++ ) {
      if( trackgen( d, buffer, j, i, 1, sectors, seclen, preindex, 
		    GAP_MGT_PLUSD, NO_INTERLEAVE, NO_AUTOFILL ) )
	return d->status = DISK_GEOM;
    }
  }

  return d->status = DISK_OK;
}

/* 1 RANDOMIZE USR 15619: REM : RUN "        " */
static libspectrum_byte beta128_boot_loader[] = {
  0x00, 0x01, 0x1c, 0x00, 0xf9, 0xc0, 0x31, 0x35, 0x36, 0x31, 0x39, 0x0e, 
  0x00, 0x00, 0x03, 0x3d, 0x00, 0x3a, 0xea, 0x3a, 0xf7, 0x22, 0x20, 0x20, 
  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x0d, 
};

static int
trdos_insert_basic_file( disk_t *d, trdos_spec_t *spec,
                         const libspectrum_byte *data, unsigned int size )
{
  unsigned int fat_sector, fat_entry, n_sec, n_bytes, n_copied;
  int i, t, s, slen, len_pre_dam, len_pre_data;
  disk_gap_t *g = &gaps[ GAP_TRDOS ];
  trdos_dirent_t entry;
  libspectrum_byte trailing_data[] = { 0x80, 0xaa, 0x01, 0x00 }; /* line 1 */

  /* Check free FAT entries (we don't purge deleted files) */
  if( spec->file_count >= 128 )
    return DISK_UNSUP;

  /* Check free sectors */
  n_sec = ( size + ARRAY_SIZE( trailing_data ) + 255 ) / 256;
  if( spec->free_sectors < n_sec )
    return DISK_UNSUP;

  /* Calculate sector raw length */
  slen = calc_sectorlen( ( d->density != DISK_SD && d->density != DISK_8_SD ),
                         256, GAP_TRDOS );

  /* Calculate initial gap before data in a sector */
  len_pre_dam = 0;
  len_pre_dam += g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 7;     /* ID */
  len_pre_dam += g->len[2];                                      /* GAP II */

  len_pre_data = len_pre_dam;
  len_pre_data += g->sync_len + ( g->mark >= 0 ? 3 : 0 ) + 1;    /* DAM */

  /* Write file data */
  n_copied = 0;
  s = spec->first_free_sector;
  t = spec->first_free_track;
  DISK_SET_TRACK_IDX( d, t );

  for( i = 0; i < n_sec; i++ ) {
    memset( head, 0, 256 );
    n_bytes = 0;

    /* Copy chunk of file body */
    if( n_copied < size ) {
      n_bytes = ( size - n_copied > 256 )? 256 : size - n_copied;
      memcpy( head, data + n_copied, n_bytes );
      n_copied += n_bytes;
    }

    /* Copy trailing parameters */
    if( n_copied >= size ) {
      while( n_copied - size < ARRAY_SIZE( trailing_data ) && n_bytes < 256 ) {
        head[ n_bytes ] = trailing_data[ n_copied - size ];
        n_copied++;
        n_bytes++;
      }
    }

    /* Write buffer to disk */
    d->i = g->len[1] + ( s % 8 * 2 + s / 8 ) * slen;    /* 1 9 2 10 3 ... */  
    d->i += len_pre_dam;
    data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL,
              NULL );

    /* Next sector */
    s = ( s + 1 ) % 16;

    /* Next track */
    if( s == 0 ) {
      t = t + 1;
      if( t >= d->cylinders ) return DISK_UNSUP;
      DISK_SET_TRACK_IDX( d, t );
    }
  }

  /* Write FAT entry */
  memcpy( entry.filename, "boot    ", 8 );
  entry.file_extension = 'B';
  entry.param1         = size; /* assumes variables = 0 */
  entry.param2         = size;
  entry.file_length    = n_sec;
  entry.start_sector   = spec->first_free_sector;
  entry.start_track    = spec->first_free_track;

  /* Copy sector to buffer, modify and write back to disk recalculating CRCs */
  DISK_SET_TRACK_IDX( d, 0 );
  fat_sector = spec->file_count / 16;
  d->i = g->len[1] + ( ( fat_sector ) % 8 * 2 + ( fat_sector ) / 8 ) * slen;  
  memcpy( head, d->track + d->i + len_pre_data, 256 );

  fat_entry  = spec->file_count % 16;
  trdos_write_dirent( head + fat_entry * 16, &entry );

  d->i += len_pre_dam;
  data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL, NULL );

  /* Write specification sector */
  spec->file_count       += 1;
  spec->free_sectors     -= n_sec;
  spec->first_free_sector = s;
  spec->first_free_track  = t;
  trdos_write_spec( head, spec );

  d->i = g->len[1] + slen + len_pre_dam;    /* sector-9: 1 9 2 10 3 ... */  
  data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL, NULL );

  return DISK_OK;
}

static void
trdos_insert_boot_loader( disk_t *d )
{
  trdos_spec_t spec;
  trdos_boot_info_t info;
  int slen, del;

  /* TR-DOS specification sector */
  DISK_SET_TRACK_IDX( d, 0 );
  if( !id_seek( d, 9 ) || !datamark_read( d, &del ) )
    return;

  if( trdos_read_spec( &spec, d->track + d->i ) )
    return;

  /* Check free FAT entries (we don't purge deleted files) */
  if( spec.file_count >= 128 )
    return;

  /* Check there is at least one free sector */
  if( spec.free_sectors == 0 )
    return;
  /* TODO: stealth mode? some boot loaders hide between sectors 10-16 */

  /* Calculate sector raw length */
  slen = calc_sectorlen( ( d->density != DISK_SD && d->density != DISK_8_SD ),
                         256, GAP_TRDOS );

  /* Read FAT entries */
  if( !id_seek( d, 1 ) || !datamark_read( d, &del ) )
    return;

  if( trdos_read_fat( &info, d->track + d->i, slen ) )
    return;

  /* Check actual boot file (nothing to do) */
  if( info.have_boot_file )
    return;

  /* Insert a simple boot loader that runs the first program */
  if( info.basic_files_count >= 1 ) {
    memcpy( beta128_boot_loader + 22, info.first_basic_file, 8 );

    trdos_insert_basic_file( d, &spec, beta128_boot_loader,
                             ARRAY_SIZE( beta128_boot_loader ) );
  }

  /* TODO: use also a boot loader that can handle multiple basic pograms */
}

static int
open_trd( buffer_t *buffer, disk_t *d )
{
  int i, j, sectors, seclen;
  disk_position_context_t context;

  if( buffseek( buffer, 8*256, SEEK_CUR ) == -1 )
      return d->status = DISK_OPEN;
  if( buffavail( buffer ) < 256 )
    return d->status = DISK_OPEN;

  if( buff[231] != 0x10 || buff[227] < 0x16 || buff[227] > 0x19 )
    return d->status = DISK_OPEN;	/*?*/

  /* guess geometry of disk */
  d->sides =  buff[227] & 0x08 ? 1 : 2;
  d->cylinders = buff[227] & 0x01 ? 40 : 80;
  /* we have more tracks then on a standard disk... */
  if( buffer->file.length > d->sides * d->cylinders * 16 * 256 ) {
    for( i = d->cylinders + 1; i < 83; i++ ) {
      if( d->sides * i * 16 * 256 >= buffer->file.length ) break;
    }
    d->cylinders = i;
  }
  sectors = 16; seclen = 256;

  /* create a DD disk */
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  buffer->index = 0;
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( trackgen( d, buffer, j, i, 1, sectors, seclen,
                    NO_PREINDEX, GAP_TRDOS, INTERLEAVE_2, 0x00 ) )
        return d->status = DISK_GEOM;
    }
  }
  
  if( settings_current.auto_load ) {
    position_context_save( d, &context );
    trdos_insert_boot_loader( d );
    position_context_restore( d, &context );
  }

  return d->status = DISK_OK;
}

static int
open_fdi( buffer_t *buffer, disk_t *d, int preindex )
{
  int i, j, h, gap;
  int bpt, bpt_fm, max_bpt = 0, max_bpt_fm = 0;
  int data_offset, track_offset, head_offset, sector_offset;

  d->wrprot = buff[0x03] == 1 ? 1 : 0;
  d->sides = buff[0x06] + 256 * buff[0x07];
  d->cylinders = buff[0x04] + 256 * buff[0x05];
  GEOM_CHECK;
  data_offset = buff[0x0a] + 256 * buff[0x0b];
  h = 0x0e + buff[0x0c] + 256 * buff[0x0d];		/* save head start */
  head_offset = h;

  /* first determine the longest track */
  d->bpt = 0;
  for( i = 0; i < d->cylinders * d->sides; i++ ) {	/* ALT */
    buffer->index = head_offset;
    if( buffread( head, 7, buffer ) != 1 )	/* 7 := track head  */
      return d->status = DISK_OPEN;
    bpt = postindex_len( d, GAP_MINIMAL_MFM ) +
	  ( preindex ? preindex_len( d, GAP_MINIMAL_MFM ) : 0 ) + 6; /* +gap4 */
    bpt_fm = postindex_len( d, GAP_MINIMAL_FM ) +
	     ( preindex ? preindex_len( d, GAP_MINIMAL_FM ) : 0 ) + 3;  /* +gap4 */
    for( j = 0; j < head[0x06]; j++ ) {		/* calculate track len */
      if( j % 35 == 0 ) {				/* 35-sector header */
	if( buffread( head + 7, 245, buffer ) != 1 )	/* 7*35 := max 35 sector head */
	  return d->status = DISK_OPEN;
      }
      if( ( head[ 0x0b + 7 * ( j % 35 ) ] & 0x3f ) != 0 ) {
	bpt += calc_sectorlen( 1, 0x80 << head[ 0x0a + 7 * ( j % 35 ) ], GAP_MINIMAL_MFM );
	bpt_fm += calc_sectorlen( 0, 0x80 << head[ 0x0a + 7 * ( j % 35 ) ], GAP_MINIMAL_FM );
      }
    }
    if( bpt > max_bpt )
      max_bpt = bpt;
    if( bpt_fm > max_bpt_fm )
      max_bpt_fm = bpt_fm;

    head_offset += 7 + 7 * head[ 0x06 ];
  }

  if( max_bpt == 0 || max_bpt_fm == 0 )
    return d->status = DISK_GEOM;

  d->density = DISK_DENS_AUTO;		/* disk_alloc use d->bpt */
  if( max_bpt_fm < 3000 ) {		/* we choose an SD disk with FM */
    d->bpt = max_bpt_fm;
    gap = GAP_MINIMAL_FM;
  } else {
    d->bpt = max_bpt;
    gap = GAP_MINIMAL_MFM;
  }
  if( disk_alloc( d ) != DISK_OK )
    return d->status;
    /* start reading the tracks */

  head_offset = h;			/* restore head start */
  for( i = 0; i < d->cylinders * d->sides; i++ ) {	/* ALT */
    buffer->index = head_offset;
    buffread( head, 7, buffer );	/* 7 = track head */
    track_offset = head[0x00] + 256 * head[0x01] +
    			 65536 * head[0x02] + 16777216 * head[0x03];
    DISK_SET_TRACK_IDX( d, i );
    d->i = 0;
    if( preindex )
      preindex_add( d, gap );
    postindex_add( d, gap );
    for( j = 0; j < head[0x06]; j++ ) {
      if( j % 35 == 0 ) {			/* if we have more than 35 sector in a track,
						   we have to seek back to the next sector
						   headers and read it ( max 35 sector header */
	buffer->index = head_offset + 7 *( j + 1 );
	buffread( head + 7, 245, buffer );	/* 7*35 := max 35 sector head */
      }
      id_add( d, head[ 0x08 + 7 * ( j % 35 ) ], head[ 0x07 + 7*( j % 35 ) ],
	      head[ 0x09 + 7*( j % 35 ) ], head[ 0x0a + 7*( j % 35 ) ], gap, 
	      ( head[ 0x0b + 7*( j % 35 ) ] & 0x3f ) ? CRC_OK : CRC_ERROR );
      sector_offset = head[ 0x0c + 7 * ( j % 35 ) ] +
		      256 * head[ 0x0d + 7 * ( j % 35 ) ];
      buffer->index = data_offset + track_offset + sector_offset;
      data_add( d, buffer, NULL, ( head[ 0x0b + 7 * ( j % 35 ) ] & 0x3f ) == 0 ? 
			       -1 : 0x80 << head[ 0x0a + 7 * ( j % 35 ) ],
		head[ 0x0b + 7 * ( j % 35 ) ] & 0x80 ? DDAM : NO_DDAM,
		gap, CRC_OK, NO_AUTOFILL, NULL );
    }
    head_offset += 7 + 7 * head[0x06];
    gap4_add( d, gap );
  }
  return d->status = DISK_OK;
}

static void
cpc_set_weak_range( disk_t *d, int idx, buffer_t *buffer, int n, int len )
{
  int i, j, first = -1, last = -1;
  libspectrum_byte *t, *w;

  t = d->track + idx;
  w = buffer->file.buffer + buffer->index;

  for( i = 0; i < len; i++, t++, w++ ) {
    for( j = 0; j < n - 1; j ++ ) {
      if( *t != w[j * len] ) {
        if( first == -1 ) first = idx + i;
        last = idx + i;
      }
    }
  }
  if( first == -1 || last == -1 ) {
    return;
  }
  for( ; first <= last; first++ ) {
    bitmap_set( d->weak, first );
  }
}

#define CPC_ISSUE_NONE 0
#define CPC_ISSUE_1 1
#define CPC_ISSUE_2 2
#define CPC_ISSUE_3 3
#define CPC_ISSUE_4 4
#define CPC_ISSUE_5 5

static int
open_cpc( buffer_t *buffer, disk_t *d, int preindex )
{
  int i, j, seclen, idlen, gap, sector_pad, idx;
  int bpt, max_bpt = 0, trlen;
  int fix[84], plus3_fix;
  unsigned char *hdrb;

  d->sides = buff[0x31];
  d->cylinders = buff[0x30];			/* maximum number of tracks */
  GEOM_CHECK;
  buffer->index = 256;
/* first scan for the longest track */
  for( i = 0; i < d->sides*d->cylinders; i++ ) {
    /* ignore Sector Offset block */
    if( buffavail( buffer ) >= 13 && !memcmp( buff, "Offset-Info\r\n", 13 ) ) {
      buffer->index = buffer->file.length;
    }

      /* sometimes in the header there are more track than in the file */
    if( buffavail( buffer ) == 0 ) {			/* no more data */
      d->cylinders = i / d->sides + i % d->sides;	/* the real cylinder number */
      break;
    }
    if( buffavail( buffer ) < 256 ||
	memcmp( buff, "Track-Info", 10 ) )		/* check track header */
      return d->status = DISK_OPEN;

    gap = (unsigned char)buff[0x16] == 0xff ? GAP_MINIMAL_FM :
    					    GAP_MINIMAL_MFM;
    plus3_fix = trlen = 0;
    while( i < buff[0x10] * d->sides + buff[0x11] ) {
      if( i < 84 ) fix[i] = 0;
      i++;
    }
    if( i >= d->sides*d->cylinders || 
	i != buff[0x10] * d->sides + buff[0x11] )	/* problem with track idx. */
      return d->status = DISK_OPEN;

    bpt = postindex_len( d, gap ) +
	    ( preindex ? preindex_len( d, gap ) : 0 ) +
		( gap == GAP_MINIMAL_MFM ? 6 : 3 );	/* gap4 */
    sector_pad = 0;
    for( j = 0; j < buff[0x15]; j++ ) {			/* each sector */
      seclen = d->type == DISK_ECPC ? buff[ 0x1e + 8 * j ] +
				      256 * buff[ 0x1f + 8 * j ]
				    : 0x80 << buff[ 0x1b + 8 * j ];
      idlen = 0x80 << buff[ 0x1b + 8 * j ];	/* sector length from ID */
      if( idlen != 0 && idlen <= ( 0x80 << 0x08 ) && 		/* idlen is o.k. */
          seclen > idlen && seclen % idlen )			/* seclen != N * len */
	return d->status = DISK_OPEN;

      bpt += calc_sectorlen( gap == GAP_MINIMAL_MFM ? 1 : 0, seclen > idlen ? idlen : seclen, gap );
      if( i < 84 && d->flag & DISK_FLAG_PLUS3_CPC ) {
        if( j == 0 && buff[ 0x1b + 8 * j ] == 6 && seclen > 6144 )
	  plus3_fix = CPC_ISSUE_4;
	else if( j == 0 && buff[ 0x1b + 8 * j ] == 6 )
	  plus3_fix = CPC_ISSUE_1;
	else if( j == 0 &&
		 buff[ 0x18 + 8 * j ] == j && buff[ 0x19 + 8 * j ] == j &&
		 buff[ 0x1a + 8 * j ] == j && buff[ 0x1b + 8 * j ] == j ) 
	  plus3_fix = CPC_ISSUE_3;
	else if( j == 1 && plus3_fix == CPC_ISSUE_1 &&
                 buff[ 0x1b + 8 * j ] == 2 )
	  plus3_fix = CPC_ISSUE_2;
	else if( i == 38 && j == 0 && buff[ 0x1b + 8 * j ] == 2 )
	  plus3_fix = CPC_ISSUE_5;
	else if( j > 1 && plus3_fix == CPC_ISSUE_2 && buff[ 0x1b + 8 * j ] != 2 )
	  plus3_fix = CPC_ISSUE_NONE;
	else if( j > 0 && plus3_fix == CPC_ISSUE_3 &&
		 ( buff[ 0x18 + 8 * j ] != j || buff[ 0x19 + 8 * j ] != j ||
		   buff[ 0x1a + 8 * j ] != j || buff[ 0x1b + 8 * j ] != j ) )
	  plus3_fix = CPC_ISSUE_NONE;
	else if( j > 10 && plus3_fix == CPC_ISSUE_2 )
	  plus3_fix = CPC_ISSUE_NONE;
	else if( i == 38 && j > 0 && plus3_fix == CPC_ISSUE_5 &&
		 buff[ 0x1b + 8 * j ] != 2 - ( j & 1 ) )
	  plus3_fix = CPC_ISSUE_NONE;
      }
      trlen += seclen;
      if( seclen % 0x100 )		/* every? 128/384/...byte length sector padded */
	sector_pad++;
    }
    if( i < 84 ) {
      fix[i] = plus3_fix;
      if( fix[i] == CPC_ISSUE_4 )         bpt = 6500;/* Type 1 variant DD+ (e.g. Coin Op Hits) */
      else if( fix[i] != CPC_ISSUE_NONE ) bpt = 6250;/* we assume a standard DD track */
    }
    buffer->index += trlen + sector_pad * 128 + 256;
    if( bpt > max_bpt )
      max_bpt = bpt;
  }
  if( max_bpt == 0 )
    return d->status = DISK_GEOM;

  d->density = DISK_DENS_AUTO;			/* disk_alloc use d->bpt */
  d->bpt = max_bpt;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  DISK_SET_TRACK_IDX( d, 0 );
  buffer->index = 256;				/* rewind to first track */
  for( i = 0; i < d->sides*d->cylinders; i++ ) {
    hdrb = buff;
    buffer->index += 256;		/* skip to data */
    gap = (unsigned char)hdrb[0x16] == 0xff ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM;

    i = hdrb[0x10] * d->sides + hdrb[0x11];		/* adjust track No. */
    DISK_SET_TRACK_IDX( d, i );
    d->i = 0;
    if( preindex)
      preindex_add( d, gap );
    postindex_add( d, gap );

    sector_pad = 0;
    for( j = 0; j < hdrb[0x15]; j++ ) {			/* each sector */
      seclen = d->type == DISK_ECPC ? hdrb[ 0x1e + 8 * j ] +	/* data length in sector */
				      256 * hdrb[ 0x1f + 8 * j ]
				    : 0x80 << hdrb[ 0x1b + 8 * j ];
      idlen = 0x80 << hdrb[ 0x1b + 8 * j ];		/* sector length from ID */

      if( idlen == 0 || idlen > ( 0x80 << 0x08 ) )      /* error in sector length code -> ignore */
        idlen = seclen;

      if( i < 84 && fix[i] == 2 && j == 0 ) {	/* repositionate the dummy track  */
        d->i = 8;
      }
      id_add( d, hdrb[ 0x19 + 8 * j ], hdrb[ 0x18 + 8 * j ],
		 hdrb[ 0x1a + 8 * j ], hdrb[ 0x1b + 8 * j ], gap,
                 hdrb[ 0x1c + 8 * j ] & 0x20 && !( hdrb[ 0x1d + 8 * j ] & 0x20 ) ? 
                 CRC_ERROR : CRC_OK );

      if( i < 84 && fix[i] == CPC_ISSUE_1 && j == 0 ) {	/* 6144 */
        data_add( d, buffer, NULL, seclen, 
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap, 
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, NULL );
      } else if( i < 84 && fix[i] == CPC_ISSUE_2 && j == 0 ) {	/* 6144, 10x512 */
        datamark_add( d, hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap );
        gap_add( d, 2, gap );
        buffer->index += seclen;
      } else if( i < 84 && fix[i] == CPC_ISSUE_3 ) {	/* 128, 256, 512, ... 4096k */
        data_add( d, buffer, NULL, 128, 
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap, 
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, NULL );
        buffer->index += seclen - 128;
      } else if( i < 84 && fix[i] == CPC_ISSUE_4 ) {	/* Nx8192 (max 6384 byte ) */
        data_add( d, buffer, NULL, 6384,
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap, 
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, NULL );
        buffer->index += seclen - 6384;
      } else if( i < 84 && fix[i] == CPC_ISSUE_5 ) {	/* 9x512 */
      /* 512 256 512 256 512 256 512 256 512 */
        if( idlen == 256 ) {
          data_add( d, NULL, buff, 512,
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap,
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, NULL );
	  buffer->index += idlen;
        } else {
          data_add( d, buffer, NULL, idlen,
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap,
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, NULL );
	}
      } else {
        data_add( d, buffer, NULL, seclen > idlen ? idlen : seclen,
		hdrb[ 0x1d + 8 * j ] & 0x40 ? DDAM : NO_DDAM, gap,
		hdrb[ 0x1c + 8 * j ] & 0x20 && hdrb[ 0x1d + 8 * j ] & 0x20 ?
		CRC_ERROR : CRC_OK, 0x00, &idx );
        if( seclen > idlen ) {		/* weak sector with multiple copy  */
          cpc_set_weak_range( d, idx, buffer, seclen / idlen, idlen );
          buffer->index += ( seclen / idlen - 1 ) * idlen;
					/* ( ( N * len ) / len - 1 ) * len */
        }
      }
      if( seclen % 0x100 )		/* every? 128/384/...byte length sector padded */
	sector_pad++;
    }
    gap4_add( d, gap );
    buffer->index += sector_pad * 0x80;
  }
  return d->status = DISK_OK;
}

static int
open_scl( buffer_t *buffer, disk_t *d )
{
  int i, j, s, sectors, seclen;
  int scl_deleted, scl_files, scl_i;
  disk_position_context_t context;

  d->sides = 2;
  d->cylinders = 80;
  d->density = DISK_DD;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

/*
 TR-DOS:
- Root directory is 8 sectors long starting from track 0, sector 1
- Max root entries are 128
- Root entry dimension is 16 bytes (16*128 = 2048 => 8 sector
- Logical sector(start from 0) 8th (9th physical) holds disc info
- Logical sectors from 9 to 15 are unused
- Files are *NOT* fragmented, and start on track 1, sector 1...

*/

  if( ( scl_files = buff[8] ) > 128 || scl_files < 1 ) 	/* number of files */
    return d->status = DISK_GEOM;	/* too many file */
  buffer->index = 9;		/* read SCL entries */

  DISK_SET_TRACK_IDX( d, 0 );
  d->i = 0;
  postindex_add( d, GAP_TRDOS );
  scl_i = d->i;			/* the position of first sector */
  s = 1;			/* start sector number */
  scl_deleted = 0;		/* deleted files */
  sectors = 0;			/* allocated sectors */
				/* we use 'head[]' to build
				   TR-DOS directory */
  j = 0;			/* index for head[] */
  memset( head, 0, 256 );
  seclen = calc_sectorlen( 1, 256, GAP_TRDOS );	/* one sector raw length */
  for( i = 0; i < scl_files; i++ ) {	/* read all entry and build TR-DOS dir */
    if( buffread( head + j, 14, buffer ) != 1 )
      return d->status = DISK_OPEN;
    head[ j + 14 ] = sectors % 16; /* ( sectors + 16 ) % 16 := sectors % 16
    							 starting sector */
    head[ j + 15 ] = sectors / 16 + 1; /* ( sectors + 16 ) / 16 := sectors / 16 + 1
    							 starting track */
    sectors += head[ j + 13 ];
    if( d->data[j] == 0x01 )		/* deleted file */
      scl_deleted++;
    if( sectors > 16 * 159 ) 	/* too many sectors needed */
      return d->status = DISK_MEM;	/* or DISK_GEOM??? */

    j += 16;
    if( j == 256 ) {			/* one sector ready */
      d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
      id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
      data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL, NULL );
      memset( head, 0, 256 );
      s++;
      j = 0;
    }
  }

  if( j != 0 ) {	/* we have to add this sector  */
    d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
    id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
    data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL, NULL );
    s++;
  }
			/* and add empty sectors up to No. 16 */
  memset( head, 0, 256 );
  for( ; s <= 16; s++ ) {			/* finish the first track */
    d->i = scl_i + ( ( s - 1 ) % 8 * 2 + ( s - 1 ) / 8 ) * seclen;	/* 1 9 2 10 3 ... */
    id_add( d, 0, 0, s, SECLEN_256, GAP_TRDOS, CRC_OK );
    if( s == 9 ) {			/* TR-DOS descriptor */
      head[225] = sectors % 16;	/* first free sector */
      head[226] = sectors / 16 + 1;	/* first free track */
      head[227] = 0x16;		/* 80 track 2 side disk */
      head[228] = scl_files;		/* number of files */
      head[229] = ( 2544 - sectors ) % 256;	/* free sectors */
      head[230] = ( 2544 - sectors ) / 256;
      head[231] = 0x10;		/* TR-DOS ID byte */
      memset( head + 234, 32, 9 );
      head[244] = scl_deleted;	/* number of deleted files */
      memcpy( head + 245, "FUSE-SCL", 8 );
    }
    data_add( d, NULL, head, 256, NO_DDAM, GAP_TRDOS, CRC_OK, NO_AUTOFILL, NULL );
    if( s == 9 )
      memset( head, 0, 256 );		/* clear sector data... */
  }
  gap4_add( d, GAP_TRDOS );

  /* now we continue with the data */
  for( i = 1; i < d->sides * d->cylinders; i++ ) {
    if( trackgen( d, buffer, i % 2, i / 2, 1, 16, 256,
                  NO_PREINDEX, GAP_TRDOS, INTERLEAVE_2, 0x00 ) )
      return d->status = DISK_OPEN;
  }

  if( settings_current.auto_load ) {
    position_context_save( d, &context );
    trdos_insert_boot_loader( d );
    position_context_restore( d, &context );
  }

  return d->status = DISK_OK;
}

static int
open_td0( buffer_t *buffer, disk_t *d, int preindex )
{
  int i, j, s, sectors, seclen, bpt, gap, mfm, mfm_old;
  int data_offset, track_offset, sector_offset;
  unsigned char *uncomp_buff, *hdrb;

  if( buff[0] == 't' )		/* signature "td" -> advanced compression */
    return d->status = DISK_IMPL;	/* not implemented */

  uncomp_buff = NULL;			/* we may use this buffer */
  mfm_old = buff[5] & 0x80 ? 0 : 1;	/* td0notes say: may older teledisk
					   indicate the SD on high bit of
					   data rate */
  d->sides = buff[9];			/* 1 or 2 */
  if( d->sides < 1 || d->sides > 2 )
    return d->status = DISK_GEOM;
					/* skip comment block if any */
  data_offset = track_offset = 12 + ( buff[7] & 0x80 ? 
					10 + buff[14] + 256 * buff[15] : 0 );

  /* determine the greatest track length */
  d->bpt = 0;
  d->cylinders = 0;
  seclen = 0;
  while( 1 ) {
    buffer->index = track_offset;
    if( buffavail( buffer ) < 1 )
      return d->status = DISK_OPEN;
    if( ( sectors = buff[0] ) == 255 ) /* sector number 255 => end of tracks */
      break;
    if( buffavail( buffer ) < 4 )	/* check track header is avail. */
      return d->status = DISK_OPEN;
    if( buff[1] + 1 > d->cylinders )	/* find the biggest cylinder number */
      d->cylinders = buff[1] + 1;
    sector_offset = track_offset + 4;
    mfm = buff[2] & 0x80 ? 0 : 1;	/* 0x80 == 1 => SD track */
    bpt = postindex_len( d, mfm_old || mfm ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM ) +
	  ( preindex ? 
	    preindex_len( d, mfm_old || mfm ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM ) :
	    0 ) +
	  mfm_old || mfm ? 6 : 3;
    for( s = 0; s < sectors; s++ ) {
      buffer->index = sector_offset;
      if( buffavail( buffer ) < 6 )		/* check sector header is avail. */
	return d->status = DISK_OPEN;
      if( !( buff[4] & 0x30 ) ) {		/* only if we have data */
	if( buffavail( buffer ) < 9  )		/* check data header is avail. */
	  return d->status = DISK_OPEN;

	bpt += calc_sectorlen( mfm_old || mfm, 0x80 << buff[3], 
				    mfm_old || mfm ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM );
	if( buff[3] > seclen )
	  seclen = buff[3];			/* biggest sector */
	sector_offset += buff[6] + 256 * buff[7] - 1;
      }
      sector_offset += 9;
    }
    if( bpt > d->bpt )
      d->bpt = bpt;
    track_offset = sector_offset;
  }

  if( d->bpt == 0 )
    return d->status = DISK_GEOM;

  d->density = DISK_DENS_AUTO;
  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  DISK_SET_TRACK_IDX( d, 0 );

  buffer->index = data_offset;		/* first track header */
  while( 1 ) {
    if( ( sectors = buff[0] ) == 255 ) /* sector number 255 => end of tracks */
      break;

    DISK_SET_TRACK( d, ( buff[2] & 0x01 ), buff[1] );
    d->i = 0;
    			/* later teledisk -> if buff[2] & 0x80 -> FM track */
    gap = mfm_old || buff[2] & 0x80 ? GAP_MINIMAL_FM : GAP_MINIMAL_MFM;
    postindex_add( d, gap );

    buffer->index += 4;		/* sector header*/
    for( s = 0; s < sectors; s++ ) {
      hdrb = buff;
      buffer->index += 9;		/* skip to data */
      if( !( hdrb[4] & 0x40 ) )		/* if we have id we add */
	id_add( d, hdrb[1], hdrb[0], hdrb[2], hdrb[3], gap,
				   hdrb[4] & 0x02 ? CRC_ERROR : CRC_OK );
      if( hdrb[4] & 0x40 ) {		/* if we have _no_ id we drop data... */
	buffer->index += hdrb[6] + 256 * hdrb[7] - 1;
	continue;		/* next sector */
      }
      if( !( hdrb[4] & 0x30 ) ) {		/* only if we have data */
        seclen = 0x80 << hdrb[3];

        switch( hdrb[8] ) {
	case 0:				/* raw sector data */
	  if( hdrb[6] + 256 * hdrb[7] - 1 != seclen ) {
	    if( uncomp_buff )
	      libspectrum_free( uncomp_buff );
	    return d->status = DISK_OPEN;
	  }
	  if( data_add( d, buffer, NULL, hdrb[6] + 256 * hdrb[7] - 1,
			hdrb[4] & 0x04 ? DDAM : NO_DDAM, gap, CRC_OK, NO_AUTOFILL, NULL ) ) {
	    if( uncomp_buff )
	      libspectrum_free( uncomp_buff );
	    return d->status = DISK_OPEN;
	  }
	  break;
	case 1:				/* Repeated 2-byte pattern */
	  if( uncomp_buff == NULL && alloc_uncompress_buffer( &uncomp_buff, 8192 ) )
	    return d->status = DISK_MEM;
	  for( i = 0; i < seclen; ) {			/* fill buffer */
	    if( buffavail( buffer ) < 13 ) { 		/* check block header is avail. */
	      libspectrum_free( uncomp_buff );
	      return d->status = DISK_OPEN;
	    }
	    if( i + 2 * ( hdrb[9] + 256*hdrb[10] ) > seclen ) {
						  /* too many data bytes */
	      libspectrum_free( uncomp_buff );
	      return d->status = DISK_OPEN;
	    }
	    /* ab ab ab ab ab ab ab ab ab ab ab ... */
	    for( j = 1; j < hdrb[9] + 256 * hdrb[10]; j++ )
	      memcpy( uncomp_buff + i + j * 2, &hdrb[11], 2 );
	    i += 2 * ( hdrb[9] + 256 * hdrb[10] );
	  }
	  if( data_add( d, NULL, uncomp_buff, hdrb[6] + 256 * hdrb[7] - 1,
		      hdrb[4] & 0x04 ? DDAM : NO_DDAM, gap, CRC_OK, NO_AUTOFILL, NULL ) ) {
	    libspectrum_free( uncomp_buff );
	    return d->status = DISK_OPEN;
	  }
	  break;
	case 2:				/* Run Length Encoded data */
	  if( uncomp_buff == NULL && alloc_uncompress_buffer( &uncomp_buff, 8192 ) )
	    return d->status = DISK_MEM;
	  for( i = 0; i < seclen; ) {			/* fill buffer */
	    if( buffavail( buffer ) < 11 ) {		/* check block header is avail */
	      libspectrum_free( uncomp_buff );
	      return d->status = DISK_OPEN;
	    }
	    if( hdrb[9] == 0 ) {		/* raw bytes */
	      if( i + hdrb[10] > seclen ||	/* too many data bytes */
		      buffread( uncomp_buff + i, hdrb[10], buffer ) != 1 ) {
	        libspectrum_free( uncomp_buff );
	        return d->status = DISK_OPEN;
	      }
	      i += hdrb[10];
	    } else {				/* repeated samples */
	      if( i + 2 * hdrb[9] * hdrb[10] > seclen || /* too many data bytes */
		      buffread( uncomp_buff + i, 2 * hdrb[9], buffer ) != 1 ) {
	        libspectrum_free( uncomp_buff );
	        return d->status = DISK_OPEN;
	      }
	      /*
		 abcdefgh abcdefg abcdefg abcdefg ...
		 \--v---/ 
		  2*hdrb[9]
		 |        |       |       |           |
		 +- 0     +- 1    +- 2    +- 3    ... +- hdrb[10]-1
	      */
	      for( j = 1; j < hdrb[10]; j++ ) /* repeat 'n' times */
	        memcpy( uncomp_buff + i + j * 2 * hdrb[9], uncomp_buff + i, 2 * hdrb[9] );
	      i += 2 * hdrb[9] * hdrb[10];
	    }
	  }
	  if( data_add( d, NULL, uncomp_buff, hdrb[6] + 256 * hdrb[7] - 1,
	      hdrb[4] & 0x04 ? DDAM : NO_DDAM, gap, CRC_OK, NO_AUTOFILL, NULL ) ) {
	    libspectrum_free( uncomp_buff );
	    return d->status = DISK_OPEN;
	  }
	  break;
	default:
	  if( uncomp_buff )
	    libspectrum_free( uncomp_buff );
	  return d->status = DISK_OPEN;
	  break;
	}
      }
    }
    gap4_add( d, gap );
  }

  if( uncomp_buff )
    libspectrum_free( uncomp_buff );
  return d->status = DISK_OK;
}

/* update tracks TLEN */
void
disk_update_tlens( disk_t *d )
{
  int i;

  for( i = 0; i < d->sides * d->cylinders; i++ ) {	/* check tracks */
    DISK_SET_TRACK_IDX( d, i );
    if( d->track[-3] + 256 * d->track[-2] == 0 ) {
      d->track[-3] = d->bpt & 0xff;
      d->track[-2] = ( d->bpt >> 8 ) & 0xff;
    }
  }
}

/* open a disk image file, read and convert to our format
 * if preindex != 0 we generate preindex gap if needed
 */
static int
disk_open2( disk_t *d, const char *filename, int preindex )
{
  buffer_t buffer;
  libspectrum_id_t type;
  int error;

#ifdef GEKKO		/* Wii doesn't have access() */
  d->wrprot = 0;
#else			/* #ifdef GEKKO */
  if( access( filename, W_OK ) == -1 )		/* file read only */
    d->wrprot = 1;
  else
    d->wrprot = 0;
#endif			/* #ifdef GEKKO */

  if( utils_read_file( filename, &buffer.file ) )
    return d->status = DISK_OPEN;

  buffer.index = 0;

  error = libspectrum_identify_file_raw( &type, filename,
					 buffer.file.buffer, buffer.file.length );
  if( error ) return d->status = DISK_OPEN;
  d->type = DISK_TYPE_NONE;
  switch ( type ) {
  case LIBSPECTRUM_ID_DISK_UDI:
    d->type = DISK_UDI;
    open_udi( &buffer, d );
    break;
  case LIBSPECTRUM_ID_DISK_OPD:
    d->type = DISK_OPD;
  case LIBSPECTRUM_ID_DISK_MGT:
    if( d->type == DISK_TYPE_NONE) d->type = DISK_MGT;
  case LIBSPECTRUM_ID_DISK_IMG:
    if( d->type == DISK_TYPE_NONE) d->type = DISK_IMG;
    open_img_mgt_opd( &buffer, d );
    break;
  case LIBSPECTRUM_ID_DISK_SAD:
    d->type = DISK_SAD;
    open_sad( &buffer, d, preindex );
    break;
  case LIBSPECTRUM_ID_DISK_TRD:
    d->type = DISK_TRD;
    open_trd( &buffer, d );
    break;
  case LIBSPECTRUM_ID_DISK_FDI:
    d->type = DISK_FDI;
    open_fdi( &buffer, d, preindex );
    break;
  case LIBSPECTRUM_ID_DISK_CPC:
    d->type = DISK_CPC;
  case LIBSPECTRUM_ID_DISK_ECPC:
    if( d->type == DISK_TYPE_NONE) d->type = DISK_ECPC;
    open_cpc( &buffer, d, preindex );
    break;
  case LIBSPECTRUM_ID_DISK_SCL:
    d->type = DISK_SCL;
    open_scl( &buffer, d );
    break;
  case LIBSPECTRUM_ID_DISK_TD0:
    d->type = DISK_TD0;
    open_td0( &buffer, d, preindex );
    break;
  case LIBSPECTRUM_ID_DISK_D80:
    d->type = DISK_D80;
    open_d40_d80( &buffer, d );
    break;
  default:
    utils_close_file( &buffer.file );
    return d->status = DISK_OPEN;
  }
  if( d->status != DISK_OK ) {
    if( d->data != NULL )
      libspectrum_free( d->data );
    utils_close_file( &buffer.file );
    return d->status;
  }
  utils_close_file( &buffer.file );
  d->dirty = 0;
  disk_update_tlens( d );
  update_tracks_mode( d );
  d->filename = utils_safe_strdup( filename );
  return d->status = DISK_OK;
}

/*--------------------- other fuctions -----------------------*/

/* create a two sided disk (d) from two one sided (d1 and d2) */
int
disk_merge_sides( disk_t *d, disk_t *d1, disk_t *d2, int autofill )
{
  int i;
  int clen;

  if( d1->sides != 1 || d2->sides != 1 ||
      d1->bpt != d2->bpt ||
      ( autofill < 0 && d1->cylinders != d2->cylinders ) )
    return DISK_GEOM;

  d->wrprot = 0;
  d->dirty = 0;
  d->sides = 2;
  d->type = d1->type;
  d->cylinders = d2->cylinders > d1->cylinders ? d2->cylinders : d1->cylinders;
  d->bpt = d1->bpt;
  d->density = DISK_DENS_AUTO;

  if( disk_alloc( d ) != DISK_OK )
    return d->status;

  clen = DISK_CLEN( d->bpt );
  d->track = d->data;
  d1->track = d1->data;
  d2->track = d2->data;
  for( i = 0; i < d->cylinders; i++ ) {
    if( i < d1->cylinders )
      memcpy( d->track, d1->track, d->tlen );
    else {
      d->track[0] = d->bpt & 0xff;
      d->track[1] = ( d->bpt >> 8 ) & 0xff;
      d->track[2] = 0x00;
      memset( d->track + 3, autofill & 0xff, d->bpt );		/* fill data */
      memset( d->track + 3 + d->bpt, 0x00, 3 * clen );		/* no clock and other marks */
    }
    d->track += d->tlen;
    d1->track += d1->tlen;
    if( i < d2->cylinders )
      memcpy( d->track, d2->track, d->tlen );
    else {
      d->track[0] = d->bpt & 0xff;
      d->track[1] = ( d->bpt >> 8 ) & 0xff;
      d->track[2] = 0x00;
      memset( d->track + 1, autofill & 0xff, d->bpt );		/* fill data */
      memset( d->track + 1 + d->bpt, 0x00, 3 * clen );		/* no clock and other marks */
    }
    d->track += d->tlen;
    d2->track += d2->tlen;
  }
  disk_close( d1 );
  disk_close( d2 );
  return d->status = DISK_OK;
}

int
disk_open( disk_t *d, const char *filename, int preindex, int merge_disks )
{
  char *filename2;
  char c = ' ';
  int l, g = 0, pos = 0;
  disk_t d1, d2;

  d->filename = NULL;
  if( filename == NULL || *filename == '\0' )
    return d->status = DISK_OPEN;

  l = strlen( filename );

  if( !merge_disks || l < 7 )	/* if we do not want to open two separated disk image as one double sided disk */
    return disk_open2( d, filename, preindex );

  filename2 = (char *)filename + ( l - 1 );
  while( l ) {				/* [Ss]ide[ _][abAB12][ _.] */
    if( g == 0 && ( *filename2 == '.' || *filename2 == '_' ||
		    *filename2 == ' ' ) ) {
      g++;
    } else if( g == 1 && ( *filename2 == '1' || *filename2 == 'a' ||
			   *filename2 == 'A' ) ) {
      g++;
      pos = filename2 - filename;
      c = *filename2 + 1;		/* 1->2, a->b, A->B */
    } else if( g == 1 && ( *filename2 == '2' || *filename2 == 'b' ||
			   *filename2 == 'B' ) ) {
      g++;
      pos = filename2 - filename;
      c = *filename2 - 1;		/* 2->1, b->a, B->A */
    } else if( g == 2 && ( *filename2 == '_' || *filename2 == ' ' ) ) {
      g++;
    } else if( g == 3 && l >= 5 && ( !memcmp( filename2 - 3, "Side", 4 ) ||
				     !memcmp( filename2 - 3, "side", 4 ) ) ) {
      g++;
      break;
    } else {
      g = 0;
    }
    l--;
    filename2--;
  }
  if( g != 4 )
    return d->status = disk_open2( d, filename, preindex );
  d1.data = NULL; d1.flag = d->flag;
  d2.data = NULL; d2.flag = d->flag;
  filename2 = utils_safe_strdup( filename );
  *(filename2 + pos) = c;

  if( settings_current.disk_ask_merge &&
      !ui_query( "Try to merge 'B' side of this disk?" ) ) {
    libspectrum_free( filename2 );
    return d->status = disk_open2( d, filename, preindex );
  }

  if( disk_open2( &d2, filename2, preindex ) ) {
    return d->status = disk_open2( d, filename, preindex );
  }

  if( disk_open2( &d1, filename, preindex ) )
    return d->status = d1.status;

  if( disk_merge_sides( d, &d1, &d2, 0x00 ) ) {
    disk_close( &d2 );
    *d = d1;
  }
/*  fprintf( stderr, "`%s' and `%s' merged\n", filename, filename2 ); */
  libspectrum_free( filename2 );
  return d->status;
}

/*--------------------- start of write section ----------------*/

static int
write_udi( FILE *file, disk_t *d )
{
  int i, j, error;
  size_t len;
  libspectrum_dword crc;

  udi_pack_tracks( d );
#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  udi_compress_tracks( d );
#endif			/* #ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */

  crc = ~( libspectrum_dword ) 0;
  len = 16;

  for( i = 0; i < d->sides * d->cylinders; i++ ) {	/* check tracks */
    DISK_SET_TRACK_IDX( d, i );
    if( d->track[-1] == 0xf0 )
      len += 7 + d->track[-3] + 256 * d->track[-2];
    else
      len += 3 + UDI_TLEN( d->track[-1], d->track[-3] + 256 * d->track[-2] );
  }
  head[0] = 'U';
  head[1] = 'D';
  head[2] = 'I';
  head[3] = '!';
  head[4] = len & 0xff;
  head[5] = ( len >> 8 ) & 0xff;
  head[6] = ( len >> 16 ) & 0xff;
  head[7] = ( len >> 24 ) & 0xff;
  head[8] = 0x00;
  head[9] = d->cylinders - 1;
  head[10] = d->sides - 1;
  head[11] = head[12] = head[13] = head[14] = head[15] = 0;
  if( fwrite( head, 16, 1, file ) != 1 )
    return d->status = DISK_WRPART;
  for( j = 0; j < 16; j++ )
    crc = crc_udi( crc, head[j] );
  for( i = 0; i < d->sides * d->cylinders; i++ ) {	/* write tracks */
    DISK_SET_TRACK_IDX( d, i );
    head[0] = d->track[-1];		/* track type */
    head[1] = d->track[-3];		/* track len  */
    head[2] = d->track[-2];		/* track len2 */
    if( fwrite( head, 3, 1, file ) != 1 )
      return d->status = DISK_WRPART;

    for( j = 0; j < 3; j++ )
      crc = crc_udi( crc, head[j] );

    if( d->track[-1] == 0xf0 )
      len = 4 + d->track[-3] + 256 * d->track[-2];
    else
      len = UDI_TLEN( d->track[-1], d->track[-3] + 256 * d->track[-2] );
    if( fwrite( d->track, len, 1, file ) != 1 )
      return d->status = DISK_WRPART;

    for( j = len; j > 0; j-- ) {
      crc = crc_udi( crc, *d->track );
      d->track++;
    }
  }
  head[0] = crc & 0xff;
  head[1] = ( crc >> 8 ) & 0xff;
  head[2] = ( crc >> 16 ) & 0xff;
  head[3] = ( crc >> 24 ) & 0xff;
  if( fwrite( head, 4, 1, file ) != 1 )		/* CRC */
    fclose( file );

#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  /* Keep tracks uncompressed in memory */
  error = udi_uncompress_tracks( d );
  if( error ) return error;
#endif			/* #ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION */

  udi_unpack_tracks( d );
  return d->status = DISK_OK;
}

static int
write_img_mgt_opd( FILE *file, disk_t *d )
{
  int i, j, sbase, sectors, seclen, mfm, cyl;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl ) ||
      ( d->type != DISK_OPD && ( sbase != 1 || seclen != 2 || sectors != 10 ) ) ||
      ( d->type == DISK_OPD && ( sbase != 0 || seclen != 1 || sectors != 18 ) ) )
    return d->status = DISK_GEOM;

  if( cyl == -1 ) cyl = d->cylinders;
  if( cyl != 40 && cyl != 80 )
    return d->status = DISK_GEOM;

  if( d->type == DISK_IMG ) {	/* out-out */
    for( j = 0; j < d->sides; j++ ) {
      for( i = 0; i < cyl; i++ ) {
	if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	  return d->status = DISK_GEOM;
      }
    }
  } else {			/* alt */
    for( i = 0; i < cyl; i++ ) {	/* MGT */
      for( j = 0; j < d->sides; j++ ) {
	if( savetrack( d, file, j, i, d->type == DISK_MGT ? 1 : 0,
	    sectors, seclen ) )
	  return d->status = DISK_GEOM;
      }
    }
  }
  return d->status = DISK_OK;
}

static int
write_d40_d80( FILE *file, disk_t *d )
{
  int i, j, sbase, sectors, seclen, mfm, cyl;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl ) ||
      ( sbase != 1 ) )
    return d->status = DISK_GEOM;

  if( cyl == -1 ) cyl = d->cylinders;
  if( ( d->type == DISK_D40 && cyl > 43 ) ||
      ( d->type == DISK_D80 && cyl > 83 ) )
    return d->status = DISK_GEOM;

  for( i = 0; i < cyl; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( savetrack( d, file, j, i, 1,
	    sectors, seclen ) )
	  return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
write_trd( FILE *file, disk_t *d )
{
  int i, j, sbase, sectors, seclen, mfm, cyl;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl ) ||
      sbase != 1 || seclen != 1 || sectors != 16 )
    return d->status = DISK_GEOM;

  if( cyl == -1 ) cyl = d->cylinders;
  for( i = 0; i < cyl; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
write_sad( FILE *file, disk_t *d )
{
  int i, j, sbase, sectors, seclen, mfm, cyl;

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl ) || sbase != 1 )
    return d->status = DISK_GEOM;

  if( cyl == -1 ) cyl = d->cylinders;
  memcpy( head, "Aley's disk backup", 18 );
  head[18] = d->sides;
  head[19] = cyl;
  head[20] = sectors;
  head[21] = seclen * 4;
  if( fwrite( head, 22, 1, file ) != 1 )	/* SAD head */
    return d->status = DISK_WRPART;

  for( j = 0; j < d->sides; j++ ) {	/* OUT-OUT */
    for( i = 0; i < cyl; i++ ) {
      if( savetrack( d, file, j, i, 1, sectors, seclen ) )
	return d->status = DISK_GEOM;
    }
  }
  return d->status = DISK_OK;
}

static int
write_fdi( FILE *file, disk_t *d )
{
  int i, j, k, sbase, sectors, seclen, mfm, del;
  int h, t, s, b;
  int toff, soff;

  memset( head, 0, 14 );
  memcpy( head, "FDI", 3 );
  head[0x03] = d->wrprot = 1;
  head[0x04] = d->cylinders & 0xff;
  head[0x05] = d->cylinders >> 8;
  head[0x06] = d->sides & 0xff;
  head[0x07] = d->sides >> 8;
  sectors = 0;
  for( j = 0; j < d->cylinders; j++ ) {	/* count sectors */
    for( i = 0; i < d->sides; i++ ) {
      guess_track_geom( d, i, j, &sbase, &s, &seclen, &mfm );
      sectors += s;
    }
  }
  h = ( sectors + d->cylinders * d->sides ) * 7;	/* track header len */
  head[0x08] = ( h + 0x0e ) & 0xff;	/* description offset */
  head[0x09] = ( h + 0x0e ) >> 8; /* "http://fuse-emulator.sourceforge.net" */
  head[0x0a] = ( h + 0x33 ) & 0xff;	/* data offset */
  head[0x0b] = ( h + 0x33 ) >> 8;
  if( fwrite( head, 14, 1, file ) != 1 )	/* FDI head */
    return d->status = DISK_WRPART;

  /* write track headers */
  toff = 0;			/* offset of track data */
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      DISK_SET_TRACK( d, j, i );
      d->i = 0;
      head[0x00] = toff & 0xff;
      head[0x01] = ( toff >> 8 ) & 0xff;	/* track offset */
      head[0x02] = ( toff >> 16 ) & 0xff;
      head[0x03] = ( toff >> 24 ) & 0xff;
      head[0x04] = 0;
      head[0x05] = 0;
      guess_track_geom( d, j, i, &sbase, &sectors, &seclen, &mfm );
      head[0x06] = sectors;
      if( fwrite( head, 7, 1, file ) != 1 )	/* track header */
	return d->status = DISK_WRPART;

      DISK_SET_TRACK( d, j, i );
      d->i = 0;
      k = 0;
      soff = 0;
      while( sectors > 0 ) {
	while( k < 35 && id_read( d, &h, &t, &s, &b ) ) {
	  head[ 0x00 + k * 7 ] = t;
	  head[ 0x01 + k * 7 ] = h;
	  head[ 0x02 + k * 7 ] = s;
	  head[ 0x03 + k * 7 ] = b;
	  head[ 0x05 + k * 7 ] = soff & 0xff;
	  head[ 0x06 + k * 7 ] = ( soff >> 8 ) & 0xff;
	  if( !datamark_read( d, &del ) ) {
	    head[ 0x04 + k * 7 ] = 0;	/* corrupt sector data */
	  } else {
	    head[ 0x04 + k * 7 ] = 1 << b | ( del ? 0x80 : 0x00 );
	    soff += 0x80 << b;
	  }
	  k++;
	}			/* Sector header */
	if( fwrite( head, 7 * k, 1, file ) != 1 )
	  return d->status = DISK_WRPART;

	sectors -= k;
	k = 0;
      }
      toff += soff;
    }
  }
  if( fwrite( "http://fuse-emulator.sourceforge.net", 37, 1, file ) != 1 )
    return d->status = DISK_WRPART;

  /* write data */
  for( i = 0; i < d->cylinders; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      if( saverawtrack( d, file, j, i ) )
	return d->status = DISK_WRPART;
    }
  }
  return d->status = DISK_OK;
}

static int
write_cpc( FILE *file, disk_t *d )
{
  int i, j, k, sbase, sectors, seclen, mfm, cyl;
  int h, t, s, b;
  size_t len;

  i = check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl );
  if( i & DISK_SECLEN_VARI || i & DISK_SPT_VARI || i & DISK_WEAK_DATA )
    return d->status = DISK_GEOM;

  if( i & DISK_MFM_VARI )
    mfm = -1;
  if( cyl == -1 ) cyl = d->cylinders;

  memset( head, 0, 256 );
  memcpy( head, "MV - CPCEMU Disk-File\r\nDisk-Info\r\n", 34 );
  head[0x30] = cyl;
  head[0x31] = d->sides;
  len = sectors * ( 0x80 << seclen ) + 256;
  head[0x32] = len & 0xff;
  head[0x33] = len >> 8;
  if( fwrite( head, 256, 1, file ) != 1 )	/* CPC head */
    return d->status = DISK_WRPART;

  memset( head, 0, 256 );
  memcpy( head, "Track-Info\r\n", 12 );
  for( i = 0; i < cyl; i++ ) {
    for( j = 0; j < d->sides; j++ ) {
      DISK_SET_TRACK( d, j, i );
      d->i = 0;
      head[0x10] = i;
      head[0x11] = j;
      head[0x14] = seclen;
      head[0x15] = sectors;
      if( mfm != -1 )
	head[0x16] = mfm ? 0x4e : 0xff;
      head[0x17] = 0xe5;
      k = 0;
      while( id_read( d, &h, &t, &s, &b ) ) {
	head[ 0x18 + k * 8 ] = t;
	head[ 0x19 + k * 8 ] = h;
	head[ 0x1a + k * 8 ] = s;
	head[ 0x1b + k * 8 ] = b;
	if( k == 0 && mfm == -1 ) {	/* if mixed MFM/FM tracks */
	  head[0x16] = d->track[ d->i ] == 0x4e ? 0x4e : 0xff;
	}
	k++;
      }
      if( fwrite( head, 256, 1, file ) != 1 )	/* Track head */
	return d->status = DISK_WRPART;

      if( saverawtrack( d, file, j, i ) )
	return d->status = DISK_WRPART;
    }
  }
  return d->status = DISK_OK;
}

static int
write_scl( FILE *file, disk_t *d )
{
  int i, j, k, l, t, s, sbase, sectors, seclen, mfm, del, cyl;
  int entries;
  libspectrum_dword sum = 597;		/* sum of "SINCLAIR" */

  if( check_disk_geom( d, &sbase, &sectors, &seclen, &mfm, &cyl ) ||
      sbase != 1 || seclen != 1 || sectors != 16 )
    return d->status = DISK_GEOM;

  DISK_SET_TRACK_IDX( d, 0 );

  /* TR-DOS descriptor */
  if( !id_seek( d, 9 ) || !datamark_read( d, &del ) )
    return d->status = DISK_GEOM;

  entries = head[8] = d->track[ d->i + 228 ];	/* number of files */

  if( entries > 128 || d->track[ d->i + 231 ] != 0x10 ||
      ( d->track[ d->i + 227 ] != 0x16 && d->track[ d->i + 227 ] != 0x17 &&
	d->track[ d->i + 227 ] != 0x18 && d->track[ d->i + 227 ] != 0x19 ) ||
      d->track[ d->i ] != 0 )
    return d->status = DISK_GEOM;

  memcpy( head, "SINCLAIR", 8 );
  sum += entries;
  if( fwrite( head, 9, 1, file ) != 1 )
    return d->status = DISK_WRPART;

  /* save SCL entries */
  j = 1;			/* sector number */
  k = 0;
  sectors = 0;
  for( i = 0; i < entries; i++ ) {	/* read TR-DOS dir */
    if( j > 8 )		/* TR-DOS dir max 8 sector len */
      return d->status = DISK_GEOM;

    if( k == 0 && ( !id_seek( d, j ) || !datamark_read( d, &del ) ) )
      return d->status = DISK_GEOM;

    if( fwrite( d->track + d->i + k, 14, 1, file ) != 1 )
      return d->status = DISK_WRPART;
    sectors += d->track[ d->i + k + 13 ];	/* file length in sectors */
    for( s = 0; s < 14; s++ ) {
      sum += d->track[ d->i + k ];
      k++;
    }
    k += 2;
    if( k >= 256 ) {		/* end of sector */
      j++;
      k = 0;
    }
  }
  /* save data */
  /* we have to 'defragment' the disk :) */
  j = 1;			/* sector number */
  k = 0;			/* byte offset */
  for( i = 0; i < entries; i++ ) {	/* read TR-DOS dir */
    DISK_SET_TRACK_IDX( d, 0 );
    if( k == 0 ) {
      if ( !id_seek( d, j ) || !datamark_read( d, &del ) )
	return d->status = DISK_GEOM;
      memcpy( head, d->track + d->i, 256 );
    }

    s = head[ k + 14 ];	/* starting sector */
    t = head[ k + 15 ];	/* starting track */
    sectors = head[ k + 13 ] + s;	/* last sector */
    k += 16;
    if( k == 256 ) {
      k = 0;
      j++;
    }
    if( t >= d->sides * d->cylinders )
      return d->status = DISK_GEOM;

    if( s % 16 == 0 )
      t--;
    DISK_SET_TRACK_IDX( d, t );

    for( ; s < sectors; s++ ) {		/* save all sectors */
      if( s % 16 == 0 ) {
	t++;
	if( t >= d->sides * d->cylinders )
	  return d->status = DISK_GEOM;
        DISK_SET_TRACK_IDX( d, t );
      }
      if( id_seek( d, s % 16 + 1 ) ) {
	if( datamark_read( d, &del ) ) {	/* write data if we have data */
	  if( data_write_file( d, file, 1 ) ) {
	    return d->status = DISK_GEOM;
	  } else {
	    for( l = 0; l < 256; l++ )
	      sum += d->track[ d->i + l ];
	  }
	}
      } else {
	return DISK_GEOM;
      }
    }
  }
  head[0] = sum & 0xff;
  head[1] = ( sum >> 8 ) & 0xff;
  head[2] = ( sum >> 16 ) & 0xff;
  head[3] = ( sum >> 24 ) & 0xff;

  if( fwrite( head, 4, 1, file ) != 1 )
    return d->status = DISK_WRPART;

  return d->status = DISK_OK;
}

static int
write_log( FILE *file, disk_t *d )
{
  int i, j, k, del, rev;
  int h, t, s, b;
  char str[17];

  str[16] = '\0';
  fprintf( file, "DISK tracks log!\n" );
  fprintf( file, "Sides: %d, cylinders: %d\n", d->sides, d->cylinders );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      DISK_SET_TRACK( d, i, j );
      d->i = 0;
      fprintf( file, "\n*********\nSide: %d, cylinder: %d type: 0x%02x tlen: %5u\n",
			i, j, d->track[-1], d->track[-3] + 256 * d->track[-2] );
      while( id_read( d, &h, &t, &s, &b ) ) {
	fprintf( file, "  h:%d t:%d s:%d l:%d(%d)", h, t, s, b, 0x80 << b );
	if( datamark_read( d, &del ) )
	  fprintf( file, " %s\n", del ? "deleted" : "normal" );
	else
	  fprintf( file, " noDAM\n" );
      }
    }
  }
  fprintf( file, "\n***************************\nSector Data Dump:\n" );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      DISK_SET_TRACK( d, i, j );
      d->i = 0;
      fprintf( file, "\n*********\nSide: %d, cylinder: %d type: 0x%02x tlen: %5u\n",
			i, j, d->track[-1], d->track[-3] + 256 * d->track[-2] );
      rev = k = 0;
      while( id_read( d, &h, &t, &s, &b ) ) {
	b = 0x80 << b;
	if( datamark_read( d, &del ) )
	  fprintf( file, "  h:%d t:%d s:%d l:%d (%s)\n", h, t, s, b,
		   del ? "deleted" : "normal" );
	else
	  fprintf( file, "  h:%d t:%d s:%d l:%d (missing data)\n", h,
		   t, s, b );
	k = 0;
	while( k < b ) {
	  if( !( k % 16 ) )
	    fprintf( file, "0x%08x:", k );
	  fprintf( file, " 0x%02x", d->track[ d->i ] );
	  str[ k & 0x0f ] = d->track[ d->i ] >= 32 &&
			    d->track[ d->i ] < 127 ? d->track[ d->i ] : '.';
	  k++;
	  if( !( k % 16 ) )
	    fprintf( file, " | %s\n", str );
	  d->i++;
	  if( d->i >= d->bpt ) {
	    d->i = 0;
	    rev++;
	    if( rev == 6 )
	      break;
	  }
	}
      }
    }
  }
  fprintf( file, "\n***************************\n**Full Dump:\n" );
  for( j = 0; j < d->cylinders; j++ ) {	/* ALT :) */
    for( i = 0; i < d->sides; i++ ) {
      DISK_SET_TRACK( d, i, j );
      d->i = 0;
      fprintf( file, "\n*********\nSide: %d, cylinder: %d type: 0x%02x tlen: %5u\n",
			i, j, d->track[-1], d->track[-3] + 256 * d->track[-2] );
      k = 0;
      while( d->i < d->bpt ) {
	if( !( k % 8 ) )
	  fprintf( file, "0x%08x:", d->i );
	fprintf( file, " 0x%04x", d->track[ d->i ] |
		 ( bitmap_test( d->clocks, d->i ) ? 0x0c00 : 0x0000 ) |
		 ( bitmap_test( d->fm, d->i ) ? 0x1000 : 0x0000 ) |
		 ( bitmap_test( d->weak, d->i ) ? 0x8000 : 0x0000 ) );
	k++;
	if( !( k % 8 ) )
	  fprintf( file, "\n" );
	d->i++;
      }
    }
  }
  return d->status = DISK_OK;
}

int
disk_write( disk_t *d, const char *filename )
{
  FILE *file;
  const char *ext;
  size_t namelen;
  libspectrum_byte *t, *c, *f, *w;
  int idx;

  if( ( file = fopen( filename, "wb" ) ) == NULL )
    return d->status = DISK_WRFILE;

  namelen = strlen( filename );
  if( namelen < 4 )
    ext = "";
  else
    ext = filename + namelen - 4;

  if( d->type == DISK_TYPE_NONE ) {
    if( !strcasecmp( ext, ".udi" ) )
      d->type = DISK_UDI;				/* ALT side */
    else if( !strcasecmp( ext, ".dsk" ) )
      d->type = DISK_CPC;				/* ALT side */
    else if( !strcasecmp( ext, ".mgt" ) )
      d->type = DISK_MGT;				/* ALT side */
    else if( !strcasecmp( ext, ".opd" ) || !strcasecmp( ext, ".opu" ) )
      d->type = DISK_OPD;				/* ALT side */
    else if( !strcasecmp( ext, ".img" ) )		/* out-out */
      d->type = DISK_IMG;
    else if( !strcasecmp( ext, ".trd" ) )		/* ALT */
      d->type = DISK_TRD;
    else if( !strcasecmp( ext, ".sad" ) )		/* ALT */
      d->type = DISK_SAD;
    else if( !strcasecmp( ext, ".fdi" ) )		/* ALT */
      d->type = DISK_FDI;
    else if( !strcasecmp( ext, ".d40" ) )		/* ALT side */
      d->type = DISK_D40;
    else if( !strcasecmp( ext, ".d80" ) )		/* ALT side */
      d->type = DISK_D80;
    else if( !strcasecmp( ext, ".scl" ) )		/* not really a disk image */
      d->type = DISK_SCL;
    else if( !strcasecmp( ext, ".td0" ) )		/* not supported */
      d->type = DISK_TD0;
    else if( !strcasecmp( ext, ".log" ) )		/* ALT */
      d->type = DISK_LOG;
    else
      d->type = DISK_UDI;				/* ALT side */
  }

  /* Save position of current data */
  t = d->track;
  c = d->clocks;
  f = d->fm;
  w = d->weak;
  idx = d->i;

  update_tracks_mode( d );
  switch( d->type ) {
  case DISK_UDI:
    write_udi( file, d );
    break;
  case DISK_IMG:
  case DISK_MGT:
  case DISK_OPD:
    write_img_mgt_opd( file, d );
    break;
  case DISK_D40:
  case DISK_D80:
    write_d40_d80( file, d );
    break;
  case DISK_TRD:
    write_trd( file, d );
    break;
  case DISK_SAD:
    write_sad( file, d );
    break;
  case DISK_FDI:
    write_fdi( file, d );
    break;
  case DISK_SCL:
    write_scl( file, d );
    break;
  case DISK_CPC:
    write_cpc( file, d );
    break;
  case DISK_LOG:
    write_log( file, d );
    break;
  default:
    d->status = DISK_WRFILE;
    break;
  }

  /* Restore position of previous data.
     FIXME: This is a workaround. Revisit bug #279 and rethink a proper fix */
  d->track = t;
  d->clocks = c;
  d->fm = f;
  d->weak = w;
  d->i = idx;

  if( d->status != DISK_OK ) {
    fclose( file );
    return d->status;
  }

  if( fclose( file ) == -1 )
    return d->status = DISK_WRFILE;

  return d->status = DISK_OK;
}
