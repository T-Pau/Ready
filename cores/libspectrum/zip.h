/* zip.h: Routines for accessing zip archives
   Copyright (c) 2012 Sergio Baldoví

   Based on zip routines from ZXDS.
   Copyright (c) 2010 Patrik Rak

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

#ifndef LIBSPECTRUM_ZIP_H
#define LIBSPECTRUM_ZIP_H

#include <libspectrum.h>

#define ZIP_DIRECTORY_INFO_SIG 0x06054b50
#define ZIP_FILE_HEADER_SIG    0x02014b50
#define ZIP_LOCAL_HEADER_SIG   0x04034b50

#define ZIP_SUPPORTED_VERSION  20

#define ZIPFLAG_NODIR    1
#define ZIPFLAG_NOCASE   2
#define ZIPFLAG_AUTOCASE 4

enum {
  ZIP_LOCAL_HEADER_SIZE = 30,
  ZIP_FILE_HEADER_SIZE = 46,
  ZIP_DIRECTORY_INFO_SIZE = 22,
};

/* Local file header */
typedef struct zip_local_header {
    libspectrum_dword magic;             /* local file header signature       */
                                         /*   4 bytes (0x04034b50)            */
    libspectrum_word required_version;   /* version needed to extract 2 bytes */
    libspectrum_word flags;              /* general purpose bit flag  2 bytes */
    libspectrum_word compression;        /* compression method        2 bytes */
    libspectrum_word mod_time;           /* last mod file time        2 bytes */
    libspectrum_word mod_date;           /* last mod file date        2 bytes */
    libspectrum_dword crc;               /* crc-32                    4 bytes */
    libspectrum_dword compressed_size;   /* compressed size           4 bytes */
    libspectrum_dword uncompressed_size; /* uncompressed size         4 bytes */
    libspectrum_word name_size;          /* file name length          2 bytes */
    libspectrum_word extra_field_size;   /* extra field length        2 bytes */
 /* libspectrum_byte name[ name_size ]; */
 /* libspectrum_byte extra_field[ extra_field_size ]; */
} zip_local_header;

/* Central directory file header */
typedef struct zip_file_header {
    libspectrum_dword magic;              /* central file header signature    */
                                          /*   4 bytes (0x02014b50)           */
    libspectrum_word creator_version;     /* version made by          2 bytes */
    libspectrum_word required_version;    /* version needed to extract        */
                                          /*   2 bytes */
    libspectrum_word flags;               /* general purpose bit flag 2 bytes */
    libspectrum_word compression;         /* compression method       2 bytes */
    libspectrum_word mod_time;            /* last mod file time       2 bytes */
    libspectrum_word mod_date;            /* last mod file date       2 bytes */
    libspectrum_dword crc;                /* crc-32                   4 bytes */
    libspectrum_dword compressed_size;    /* compressed size          4 bytes */
    libspectrum_dword uncompressed_size;  /* uncompressed size        4 bytes */
    libspectrum_word name_size;           /* file name length         2 bytes */
    libspectrum_word extra_field_size;    /* extra field length       2 bytes */
    libspectrum_word comment_size;        /* file comment length      2 bytes */
    libspectrum_word disk_index;          /* disk number start        2 bytes */
    libspectrum_word internal_flags;      /* internal file attributes 2 bytes */
    libspectrum_dword external_flags;     /* external file attributes 4 bytes */
    libspectrum_signed_dword file_offset; /* relative offset of local header 4 bytes */
 /* libspectrum_byte name[ name_size ]; */
 /* libspectrum_byte extra_field[ extra_field_size ]; */
 /* libspectrum_byte comment[ comment_size ]; */
} zip_file_header;

/* End of central directory record */
typedef struct zip_directory_info {
    libspectrum_dword magic;               /* end of central dir signature    */
                                           /*   4 bytes (0x06054b50)          */
    libspectrum_word disk_index;           /* number of this disk             */
                                           /*   2 bytes                       */
    libspectrum_word directory_disk_index; /* number of the disk with the     */
                                           /* start of the central directory  */
                                           /*   2 bytes                       */
    libspectrum_word disk_file_count;      /* total number of entries in the  */
                                           /* central directory on this disk  */
                                           /*   2 bytes                       */
    libspectrum_word file_count;           /* total number of entries in the  */
                                           /* central directory               */
                                           /*   2 bytes                       */
    libspectrum_dword directory_size;      /* size of the central directory   */
                                           /*   4 bytes                       */
    libspectrum_dword directory_offset;    /* offset of start of central      */
                                           /* directory with respect to the   */
                                           /* starting disk number            */
                                           /*   4 bytes                       */
    libspectrum_word comment_size;         /* .ZIP file comment length        */
                                           /*   2 bytes                       */
 /* libspectrum_byte comment[ comment_size ]; */
} zip_directory_info;

typedef struct zip_stat {
  char name[1024];
  char *filename;
  size_t size;
  int is_dir;
  libspectrum_word index;
} zip_stat;

typedef struct libspectrum_zip {

  /* State of the parsing process */  
  libspectrum_dword state;

  /* Buffer with the input data to process */
  const libspectrum_byte *input_data;

  /* Size of the input data */
  size_t data_size;

  /* Current processing position of the input data */
  const libspectrum_byte *ptr;

  /* Max position to read */
  const libspectrum_byte *end;

  /* Offset of the beginning central directory. Zero when invalid/not known */
  size_t directory_offset;

  /* Number of files in the central directory */
  unsigned int file_count;

  /* Index of next file to read from the central directory */
  unsigned int file_index;

  /* Info about the current file in the archive */
  zip_file_header file_info;
  char file_name[1024];
  int file_ignore_case;
} libspectrum_zip;

struct libspectrum_zip *
libspectrum_zip_open( const libspectrum_byte *buffer, size_t length );

libspectrum_error
libspectrum_zip_next( struct libspectrum_zip *zip, zip_stat *info );

libspectrum_error
libspectrum_zip_read( struct libspectrum_zip *zip,
                      libspectrum_byte **buffer, size_t *size );

int
libspectrum_zip_locate( struct libspectrum_zip *zip, const char *filename,
                        int flags, zip_stat *info );

libspectrum_error
libspectrum_zip_rewind( struct libspectrum_zip *zip );

void
libspectrum_zip_close( struct libspectrum_zip *zip ) ;

unsigned int
libspectrum_zip_num_entries( struct libspectrum_zip *zip );

#endif				/* #ifndef LIBSPECTRUM_ZIP_H */
