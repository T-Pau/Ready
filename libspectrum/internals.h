/* internals.h: functions which need to be called inter-file by libspectrum
                routines, but not by user code
   Copyright (c) 2001-2015 Philip Kendall, Darren Salt

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

#ifndef LIBSPECTRUM_INTERNALS_H
#define LIBSPECTRUM_INTERNALS_H

#ifdef HAVE_LIB_GLIB		/* Only if we've got the real glib */
#include <glib.h>
#endif				/* #ifdef HAVE_LIB_GLIB */

#ifndef LIBSPECTRUM_LIBSPECTRUM_H
#include "libspectrum.h"
#endif				/* #ifndef LIBSPECTRUM_LIBSPECTRUM_H */

#ifdef __GNUC__
#define GCC_UNUSED __attribute__ ((unused))
#define GCC_PRINTF( fmtstring, args ) __attribute__ ((format( printf, fmtstring, args )))
#else				/* #ifdef __GNUC__ */
#define GCC_UNUSED
#define GCC_PRINTF( fmtstring, args )
#endif				/* #ifdef __GNUC__ */

#ifdef _MSC_VER
#if _MSC_VER > 1200		/* VC2005 or later */
#define __func__ __FUNCTION__
#else				/* #if _MSC_VER > 1200 */
#define __func__ "__func__"
#endif				/* _MSC_VER > 1200 */
#endif				/* #ifdef _MSC_VER */

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
  #define GNUC_VERSION \
    (__GNUC__ << 16) + __GNUC_MINOR__
  #define GNUC_PREREQ(maj, min) \
    (GNUC_VERSION >= ((maj) << 16) + (min))
#else
  #define GNUC_PREREQ(maj, min) 0
#endif

#define BUILD_BUG_ON_ZERO(e) \
  (sizeof(struct { int:-!!(e) * 1234; }))

#if !GNUC_PREREQ(3, 1) || defined( __STRICT_ANSI__ )
  #define MUST_BE_ARRAY(a) \
    BUILD_BUG_ON_ZERO(sizeof(a) % sizeof(*a))
#else
  #define SAME_TYPE(a, b) \
    __builtin_types_compatible_p(typeof(a), typeof(b))
  #define MUST_BE_ARRAY(a) \
    BUILD_BUG_ON_ZERO(SAME_TYPE((a), &(*a)))
#endif

#define ARRAY_SIZE(a) ( \
  (sizeof(a) / sizeof(*a)) \
   + MUST_BE_ARRAY(a))

/* C90 lacks SIZE_MAX.  size_t is always unsigned so this is safe. */
#ifndef SIZE_MAX
#define SIZE_MAX (~(size_t)0)
#endif

/* VC6 lacks M_LN2, and VS2003+ require _USE_MATH_DEFINES defined before math.h
 */
#ifndef M_LN2
#define M_LN2 0.693147180559945309417
#endif

/* On Win32 systems, map snprintf -> _snprintf, strcasecmp -> _stricmp and
   strncasecmp -> _strnicmp */
#if !defined(HAVE_SNPRINTF) && defined(HAVE__SNPRINTF)
#define snprintf _snprintf
#endif		/* #if !defined(HAVE_SNPRINTF) && defined(HAVE__SNPRINTF) */

#if !defined(HAVE_STRCASECMP) && defined(HAVE__STRICMP)
#define strcasecmp _stricmp
#endif		/* #if !defined(HAVE_STRCASECMP) && defined(HAVE__STRICMP) */

#if !defined(HAVE_STRNCASECMP) && defined(HAVE__STRNICMP)
#define strncasecmp _strnicmp
#endif		/* #if !defined(HAVE_STRNCASECMP) && defined(HAVE__STRNICMP) */

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/* Print using the user-provided error function */
libspectrum_error
libspectrum_print_error( libspectrum_error error, const char *format, ... )
     GCC_PRINTF( 2, 3 );

/* Acquire more memory for a buffer */
void libspectrum_make_room( libspectrum_byte **dest, size_t requested,
			    libspectrum_byte **ptr, size_t *allocated );

/* Read and write (d)words */
libspectrum_word libspectrum_read_word( const libspectrum_byte **buffer );
libspectrum_dword libspectrum_read_dword( const libspectrum_byte **buffer );
int libspectrum_write_word( libspectrum_byte **buffer, libspectrum_word w );
int libspectrum_write_dword( libspectrum_byte **buffer, libspectrum_dword d );

/* (de)compression routines */

libspectrum_error
libspectrum_uncompress_file( unsigned char **new_buffer, size_t *new_length,
			     char **new_filename, libspectrum_id_t type,
			     const unsigned char *old_buffer,
			     size_t old_length, const char *old_filename );

libspectrum_error
libspectrum_gzip_inflate( const libspectrum_byte *gzptr, size_t gzlength,
			  libspectrum_byte **outptr, size_t *outlength );

libspectrum_error
libspectrum_bzip2_inflate( const libspectrum_byte *bzptr, size_t bzlength,
			   libspectrum_byte **outptr, size_t *outlength );

libspectrum_error
libspectrum_zip_inflate( const libspectrum_byte *zipptr, size_t ziplength,
			  libspectrum_byte **outptr, size_t *outlength );

libspectrum_error
libspectrum_zip_blind_read( const libspectrum_byte *zipptr, size_t ziplength,
                            libspectrum_byte **outptr, size_t *outlength );

/* The TZX file signature */

extern const char * const libspectrum_tzx_signature;

/* Convert a 48K memory dump into separate RAM pages */

int libspectrum_split_to_48k_pages( libspectrum_snap *snap,
				    const libspectrum_byte* data );

/* Sizes of some of the arrays in the snap structure */
#define SNAPSHOT_RAM_PAGES 16
#define SNAPSHOT_SLT_PAGES 256
#define SNAPSHOT_ZXATASP_PAGES 32
#define SNAPSHOT_ZXCF_PAGES 64
#define SNAPSHOT_DOCK_EXROM_PAGES 8
#define SNAPSHOT_JOYSTICKS 7
#define SNAPSHOT_DIVIDE_PAGES 4
#define SNAPSHOT_DIVMMC_PAGES 64

/* Get memory for a snap */

libspectrum_snap* libspectrum_snap_alloc_internal( void );

libspectrum_error
libspectrum_snap_write_buffer( libspectrum_buffer *buffer, int *out_flags,
                               libspectrum_snap *snap, libspectrum_id_t type,
                               libspectrum_creator *creator, int in_flags );

/* Format specific snapshot routines */

libspectrum_error
libspectrum_plusd_read( libspectrum_snap *snap,
			const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
internal_sna_read( libspectrum_snap *snap,
		   const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
libspectrum_sna_write( libspectrum_buffer *buffer, int *out_flags,
                       libspectrum_snap *snap, int in_flags );
libspectrum_error
libspectrum_snp_read( libspectrum_snap *snap,
		      const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
libspectrum_sp_read( libspectrum_snap *snap,
		     const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
libspectrum_szx_read( libspectrum_snap *snap,
		      const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
libspectrum_szx_write( libspectrum_buffer *buffer, int *out_flags,
                       libspectrum_snap *snap, libspectrum_creator *creator,
                       int in_flags );
libspectrum_error
internal_z80_read( libspectrum_snap *snap,
		   const libspectrum_byte *buffer, size_t buffer_length );
libspectrum_error
libspectrum_z80_write2( libspectrum_buffer *buffer, int *out_flags,
                        libspectrum_snap *snap, int in_flags );
libspectrum_error
libspectrum_zxs_read( libspectrum_snap *snap,
		      const libspectrum_byte *buffer, size_t buffer_length );

/*** Tape constants ***/

/* The timings for the standard ROM loader */
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_PILOT;
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC1;
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_SYNC2;
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA0;
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_DATA1;
extern const libspectrum_dword LIBSPECTRUM_TAPE_TIMING_TAIL;

/* Tape routines */

void libspectrum_tape_block_zero( libspectrum_tape_block *block );

libspectrum_error libspectrum_tape_block_read_symbol_table_parameters(
  libspectrum_tape_block *block, int pilot, const libspectrum_byte **ptr );

libspectrum_error
libspectrum_tape_block_read_symbol_table(
  libspectrum_tape_generalised_data_symbol_table *table,
  const libspectrum_byte **ptr, size_t length );

void libspectrum_init_bits_set( void );

/* Format specific tape routines */
  
libspectrum_error
internal_tap_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		   const size_t length, libspectrum_id_t type );

libspectrum_error
internal_tap_write( libspectrum_buffer *buffer, libspectrum_tape *tape,
                    libspectrum_id_t type );

libspectrum_error
internal_tzx_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
		   const size_t length );

libspectrum_error
internal_tzx_write( libspectrum_buffer *buffer, libspectrum_tape *tape );

libspectrum_error
internal_warajevo_read( libspectrum_tape *tape,
			const libspectrum_byte *buffer, size_t length );

libspectrum_error
libspectrum_z80em_read( libspectrum_tape *tape,
                        const libspectrum_byte *buffer, size_t length );

libspectrum_error
libspectrum_csw_read( libspectrum_tape *tape,
                      const libspectrum_byte *buffer, size_t length );

libspectrum_error
libspectrum_csw_write( libspectrum_buffer *buffer, libspectrum_tape *tape );

libspectrum_error
libspectrum_wav_read( libspectrum_tape *tape, const char *filename );

libspectrum_error
internal_pzx_read( libspectrum_tape *tape, const libspectrum_byte *buffer,
                   const size_t length );

libspectrum_tape_block*
libspectrum_tape_block_internal_init(
                                libspectrum_tape_block_state *iterator,
                                libspectrum_tape *tape );

libspectrum_error
libspectrum_tape_get_next_edge_internal( libspectrum_dword *tstates, int *flags,
                                         libspectrum_tape *tape,
                                         libspectrum_tape_block_state *it );
/* Disk routines */

typedef struct libspectrum_hdf_header {

  libspectrum_byte signature[0x06];
  libspectrum_byte id;
  libspectrum_byte revision;
  libspectrum_byte flags;
  libspectrum_byte datastart_low;
  libspectrum_byte datastart_hi;
  libspectrum_byte reserved[0x0b];
  libspectrum_byte drive_identity[0x6a];

} libspectrum_hdf_header;
  
typedef struct libspectrum_ide_drive {

  /* HDF filepointer and information */
  FILE *disk;
  libspectrum_word data_offset;
  libspectrum_word sector_size;
  libspectrum_hdf_header hdf;
  
  /* Drive geometry */
  int cylinders;
  int heads;
  int sectors;

  libspectrum_byte error;
  libspectrum_byte status;
  
} libspectrum_ide_drive;

libspectrum_error
libspectrum_ide_insert_into_drive( libspectrum_ide_drive *drv,
                                   const char *filename );

libspectrum_error
libspectrum_ide_eject_from_drive( libspectrum_ide_drive *drv,
                                  GHashTable *cache );

int
libspectrum_ide_read_sector_from_hdf(
    libspectrum_ide_drive *drv,
    GHashTable *cache,
    libspectrum_dword sector_number,
    libspectrum_byte *dest );

void
libspectrum_ide_write_sector_to_hdf(
    libspectrum_ide_drive *drv,
    GHashTable *cache,
    libspectrum_dword sector_number,
    libspectrum_byte *src );

void
libspectrum_ide_commit_drive( libspectrum_ide_drive *drv, GHashTable *cache );

/* Crypto functions */

libspectrum_error
libspectrum_sign_data( libspectrum_byte **signature, size_t *signature_length,
		       libspectrum_byte *data, size_t data_length,
		       libspectrum_rzx_dsa_key *key );

/* Utility functions */

libspectrum_dword 
libspectrum_ms_to_tstates( libspectrum_dword ms );

libspectrum_dword 
libspectrum_tstates_to_ms( libspectrum_dword tstates );

void
libspectrum_set_pause_ms( libspectrum_tape_block *block,
                          libspectrum_dword pause_ms );

void
libspectrum_set_pause_tstates( libspectrum_tape_block *block,
                               libspectrum_dword pause_tstates );

size_t
libspectrum_bits_to_bytes( size_t bits );

extern const int LIBSPECTRUM_BITS_IN_BYTE;

/* glib replacement functions */

#ifndef HAVE_LIB_GLIB		/* Only if we are using glib replacement */
void
libspectrum_slist_cleanup( void );

void
libspectrum_hashtable_cleanup( void );

#ifdef HAVE_STDATOMIC_H
#include <stdatomic.h>

void
atomic_lock( atomic_char *lock_ptr );

void
atomic_unlock( atomic_char *lock_ptr );

#endif				/* #ifdef HAVE_STDATOMIC_H */

#endif				/* #ifndef HAVE_LIB_GLIB */

#endif				/* #ifndef LIBSPECTRUM_INTERNALS_H */
