/* disk.h: Routines for handling disk images
   Copyright (c) 2007-2015 Gergely Szasz

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

#ifndef FUSE_DISK_H
#define FUSE_DISK_H

static const unsigned int DISK_FLAG_NONE = 0x00;
static const unsigned int DISK_FLAG_PLUS3_CPC = 0x01;	/* try to fix some CPC issue */
static const unsigned int DISK_FLAG_OPEN_DS = 0x02;	/* try to open the other side too */

typedef enum disk_error_t {
  DISK_OK = 0,
  DISK_IMPL,
  DISK_MEM,
  DISK_GEOM,
  DISK_OPEN,
  DISK_UNSUP,
  DISK_RDONLY,
  DISK_CLOSE,
  DISK_WRFILE,
  DISK_WRPART,

  DISK_LAST_ERROR,
} disk_error_t;

typedef enum disk_type_t {
  DISK_TYPE_NONE = 0,
  DISK_UDI,		/* raw track disk image (our format :-) */
  DISK_FDI,		/* Full Disk Image ALT */
  DISK_TD0,

  /* DISCiPLE / +D / SAM Coupe */
  DISK_MGT,		/* ALT */
  DISK_IMG,		/* OUT-OUT */
  DISK_SAD,		/* OUT-OUT with geometry header */

  DISK_CPC,
  DISK_ECPC,

  /* Beta disk interface (TR-DOS) */
  DISK_TRD,
  DISK_SCL,

  /* Opus Discovery */
  DISK_OPD,

  /* Didaktik 40/80 */
  DISK_D40,
  DISK_D80,

  /* Log disk structure (.log) */
  DISK_LOG,

  DISK_TYPE_LAST,
} disk_type_t;

typedef enum disk_dens_t {
  DISK_DENS_AUTO = 0,
  DISK_8_SD,		/* 8" SD floppy 5208 MF */
  DISK_8_DD,		/* 8" DD floppy 10416 */
  DISK_SD,		/* 3125 bpt MF */
  DISK_DD,		/* 6250 bpt */
  DISK_DD_PLUS,		/* 6500 bpt e.g. Coin Op Hits */
  DISK_HD,		/* 12500 bpt*/
} disk_dens_t;

typedef struct disk_t {
  char *filename;	/* original filename */
  int sides;		/* 1 or 2 */
  int cylinders;	/* tracks per side  */
  int bpt;		/* bytes per track */
  int wrprot;		/* disk write protect */
  int dirty;		/* disk changed */
  int have_weak;	/* disk contain weak sectors */
  unsigned int flag;
  disk_error_t status;		/* last error code */
  libspectrum_byte *data;	/* disk data */
/* private part */
  int tlen;			/* length of a track with clock and other marks (bpt + 3/8bpt) */
  libspectrum_byte *track;	/* current track data bytes */
  libspectrum_byte *clocks;	/* clock marks bits */
  libspectrum_byte *fm;		/* FM/MFM marks bits */
  libspectrum_byte *weak;	/* weak marks bits/weak data */
  int i;			/* index for track and clocks */
  disk_type_t type;		/* DISK_UDI, ... */
  disk_dens_t density;		/* DISK_SD DISK_DD, or DISK_HD */
} disk_t;

/* every track data:
TRACK_LEN TYPE TRACK......DATA CLOCK..MARKS MF..MARKS WEAK..MARKS
               ^               ^            ^         ^
               |__ track       |__ clocks   |__ mf    |__ weak
  so, track[-1] = TYPE
  TLEN = track[-3] + tarck 256 * track[-2]
  TYPE is Track type as in UDI spec (0x00, 0x01, 0x02, 0x80, 0x81, 0x82) after update_tracks_mode() !!!
*/

#define DISK_CLEN( bpt ) ( ( bpt ) / 8 + ( ( bpt ) % 8 ? 1 : 0 ) )

#define DISK_SET_TRACK_IDX( d, idx ) \
   d->track = d->data + 3 + ( idx ) * d->tlen; \
   d->clocks = d->track  + d->bpt; \
   d->fm     = d->clocks + DISK_CLEN( d->bpt ); \
   d->weak   = d->fm     + DISK_CLEN( d->bpt )

#define DISK_SET_TRACK( d, head, cyl ) \
   DISK_SET_TRACK_IDX( (d), (d)->sides * cyl + head )

typedef struct disk_position_context_t {
  libspectrum_byte *track;   /* current track data bytes */
  libspectrum_byte *clocks;  /* clock marks bits */
  libspectrum_byte *fm;      /* FM/MFM marks bits */
  libspectrum_byte *weak;    /* weak marks bits/weak data */
  int i;                     /* index for track and clocks */
} disk_position_context_t;

const char *disk_strerror( int error );
/* create an unformatted disk sides -> (1/2) cylinders -> track/side,
   dens -> 'density' related to unformatted length of a track (SD = 3125,
   DD = 6250, HD = 12500, type -> if write this disk we want to convert
   into this type of image file
*/
int disk_new( disk_t *d, int sides, int cylinders, disk_dens_t dens, disk_type_t type );
/* open a disk image file. if preindex = 1 and the image file not UDI then
   pre-index gap generated with index mark (0xfc)
   this time only .mgt(.dsk)/.img/.udi and CPC/extended CPC file format
   supported
*/
int disk_open( disk_t *d, const char *filename, int preindex, int disk_merge );
/* merge two one sided disk (d1, d2) to a two sided one (d),
   after merge closes d1 and d2
*/
int disk_merge_sides( disk_t *d, disk_t *d1, disk_t *d2, int autofill );
/* write a disk image file (from the disk buffer). the d->type
   gives the format of file. if it DISK_TYPE_AUTO, disk_write
   try to guess from the file name (extension). if fail save as
   UDI.
*/
int disk_write( disk_t *d, const char *filename );
/* format disk to plus3 accept for formatting
*/
int disk_preformat( disk_t *d );
/* close a disk and free buffers
*/
void disk_close( disk_t *d );

#endif /* FUSE_DISK_H */
