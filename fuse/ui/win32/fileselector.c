/* fileselector.c: Win32 fileselector routines
   Copyright (c) 2008 Marek Januszewski
   Copyright (c) 2016 Sergio Baldoví

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

#include <config.h>

#include <windows.h>

#include "utils.h"
#include "win32internals.h"

/* FIXME: remember the last directory when opening/saving
static char *current_folder;
*/

/* TODO: poll libspectrum for supported file extensions and avoid duplication
   between UIs */
static LPCTSTR file_filter = TEXT(
"Supported Files\0"
"*.mgtsnp;*.slt;*.sna;*.snapshot;*.snp;*.sp;*.szx;*.z80;*.zx-state;"
"*.csw;*.ltp;*.pzx;*.raw;*.spc;*.sta;*.tzx;*.tap;*.wav;"
"*.d40;*.d80;*.dsk;*.fdi;*.img;*.mgt;*.opd;*.opu;*.sad;*.scl;*.td0;*.trd;*.udi;"
"*.dck;*.rom;*.hdf;*.mdr;*.fmf;*.rzx;"
"*.bin;*.log;*.mlt;*.png;*.pok;*.scr;*.svg"
#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
";*.gz;*.zip"
#endif
#ifdef LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION
";*.bz2"
#endif
"\0"
"All Files (*.*)\0"
"*.*\0"
"Auxiliary Files (*.scr;*.mlt;*.pok;*.png;*.svg;...)\0"
"*.bin;*.log;*.mlt;*.png;*.pok;*.scr;*.svg\0"
#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
"Compressed files (*.gz;*.zip;...)\0"
"*.gz;*.zip"
#ifdef LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION
";*.bz2"
#endif
"\0"
#endif
"Disk Files (*.dsk;*.udi;*.scl;*.trd;*.fdi;...)\0"
"*.d40;*.d80;*.dsk;*.fdi;*.img;*.mgt;*.opd;*.opu;*.sad;*.scl;*.td0;*.trd;*.udi\0"
"Dock Files (*.dck;*.rom)\0"
"*.dck;*.rom\0"
"Harddisk Files (*.hdf)\0"
"*.hdf\0"
"Microdrive Files (*.mdr)\0"
"*.mdr\0"
"Movie Files (*.fmf)\0"
"*.fmf\0"
"Recording Files (*.rzx)\0"
"*.rzx\0"
"Snapshot Files (*.szx;*.z80;*.sna;...)\0"
"*.mgtsnp;*.slt;*.sna;*.snapshot;*.snp;*.sp;*.szx;*.z80;*.zx-state\0"
"Tape Files (*.tap;*.tzx;*.pzx;*.wav;*.csw;...)\0"
"*.csw;*.ltp;*.pzx;*.raw;*.spc;*.sta;*.tzx;*.tap;*.wav\0"
"\0" );

static DWORD filter_index = 0;

static char*
run_dialog( const char *title, int is_saving )
{
  OPENFILENAME ofn;
  char szFile[512];
  int result;

  memset( &ofn, 0, sizeof( ofn ) );
  szFile[0] = '\0';

  ofn.lStructSize = sizeof( ofn );
  ofn.hwndOwner = fuse_hWnd;
  ofn.lpstrFilter = file_filter;
  ofn.lpstrCustomFilter = NULL;
  /* TODO: select filter based on UI operation (snapshot, recording, screenshot) */
  /* TODO: custom filter based file action (open, save) */
  ofn.nFilterIndex = filter_index;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof( szFile );
  ofn.lpstrFileTitle = NULL;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = /* OFN_DONTADDTORECENT | */ OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  if( is_saving ) {
    ofn.Flags |= OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
  } else {
    ofn.Flags |= OFN_FILEMUSTEXIST;
  }
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
/* ofn.pvReserved = NULL; */
/* ofn.FlagsEx = 0; */

  if( is_saving ) {
    result = GetSaveFileName( &ofn );
  } else {
    result = GetOpenFileName( &ofn );
  }

  filter_index = ofn.nFilterIndex;

  if( !result ) {
    return NULL;
  } else {
    return utils_safe_strdup( ofn.lpstrFile );
  }
}

char*
ui_get_open_filename( const char *title )
{
  return run_dialog( title, 0 );
}

char*
ui_get_save_filename( const char *title )
{
  return run_dialog( title, 1 );
}
