/* fileselector.c: GTK+ fileselector routines
   Copyright (c) 2000-2007 Philip Kendall
   Copyright (c) 2015-2016 Sergio Baldoví

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

#include <gtk/gtk.h>

#include "gtkinternals.h"
#include "ui/ui.h"

static gchar *current_folder;

static void add_filter_defaults( GtkWidget *file_chooser );
static void add_filter_auxiliary_files( GtkFileFilter *filter );
static void add_filter_disk_files( GtkFileFilter *filter );
static void add_filter_dock_files( GtkFileFilter *filter );
static void add_filter_harddisk_files( GtkFileFilter *filter );
static void add_filter_microdrive_files( GtkFileFilter *filter );
static void add_filter_movie_files( GtkFileFilter *filter );
static void add_filter_recording_files( GtkFileFilter *filter );
static void add_filter_snapshot_files( GtkFileFilter *filter );
static void add_filter_tape_files( GtkFileFilter *filter );

#if defined LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION || \
    defined LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
static void add_filter_compressed_files( GtkFileFilter *filter );
#endif

static char*
run_dialog( const char *title, GtkFileChooserAction action )
{
  GtkWidget *dialog;
  char *filename = NULL;
  const char *button;

  if( action == GTK_FILE_CHOOSER_ACTION_SAVE ) {
    button = "_Save";
  } else {
    button = "_Open";
  }

  dialog =
    gtk_file_chooser_dialog_new( title, GTK_WINDOW( gtkui_window ),
                                 action, "_Cancel", GTK_RESPONSE_CANCEL,
                                 button, GTK_RESPONSE_ACCEPT,
                                 NULL );

  gtk_dialog_set_default_response( GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT );

  /* TODO: select filter based on UI operation (snapshot, recording, screenshot) */
  /* TODO: custom filter based file action (open, save) */
  add_filter_defaults( dialog );

  if( current_folder )
    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), current_folder );

  if( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
    gchar *new_folder = gtk_file_chooser_get_current_folder( GTK_FILE_CHOOSER( dialog ) );
    if( new_folder ) {
      g_free( current_folder );
      current_folder = new_folder;
    }
    filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );
  }

  gtk_widget_destroy( dialog );

  return filename;
}


char*
ui_get_open_filename( const char *title )
{
  return run_dialog( title, GTK_FILE_CHOOSER_ACTION_OPEN );
}

char*
ui_get_save_filename( const char *title )
{
  return run_dialog( title, GTK_FILE_CHOOSER_ACTION_SAVE );
}

static void
add_filter_defaults( GtkWidget *file_chooser )
{
  GtkFileFilter *filter;

  /* TODO: poll libspectrum for supported file extensions and avoid duplication
     between UIs */
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Supported Files" );
  add_filter_auxiliary_files( filter );
#if defined LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION || \
    defined LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  add_filter_compressed_files( filter );
#endif
  add_filter_disk_files( filter );
  add_filter_dock_files( filter );
  add_filter_harddisk_files( filter );
  add_filter_microdrive_files( filter );
  add_filter_movie_files( filter );
  add_filter_recording_files( filter );
  add_filter_snapshot_files( filter );
  add_filter_tape_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "All Files" );
  gtk_file_filter_add_pattern( filter, "*" );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Auxiliary Files" );
  add_filter_auxiliary_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Cartridge Files" );
  add_filter_dock_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

#if defined LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION || \
    defined LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Compressed Files" );
  add_filter_compressed_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );
#endif

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Disk Files" );
  add_filter_disk_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Harddisk Files" );
  add_filter_harddisk_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Microdrive Files" );
  add_filter_microdrive_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Movie Files" );
  add_filter_movie_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Recording Files" );
  add_filter_recording_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Snapshot Files" );
  add_filter_snapshot_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name( filter, "Tape Files" );
  add_filter_tape_files( filter );
  gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( file_chooser ), filter );

}

static void
add_filter_auxiliary_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.bin" );
  gtk_file_filter_add_pattern( filter, "*.log" );
  gtk_file_filter_add_pattern( filter, "*.pok" );
  gtk_file_filter_add_pattern( filter, "*.scr" );
  gtk_file_filter_add_pattern( filter, "*.mlt" );

  gtk_file_filter_add_pattern( filter, "*.BIN" );
  gtk_file_filter_add_pattern( filter, "*.LOG" );
  gtk_file_filter_add_pattern( filter, "*.POK" );
  gtk_file_filter_add_pattern( filter, "*.SCR" );
  gtk_file_filter_add_pattern( filter, "*.MLT" );

#ifdef USE_LIBPNG
  gtk_file_filter_add_pattern( filter, "*.png" );
  gtk_file_filter_add_pattern( filter, "*.PNG" );
#endif

#ifdef HAVE_LIB_XML2
  gtk_file_filter_add_pattern( filter, "*.svg" );
  gtk_file_filter_add_pattern( filter, "*.SVG" );
#endif
}


#if defined LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION || \
    defined LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION

static void
add_filter_compressed_files( GtkFileFilter *filter )
{
#ifdef LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION
  gtk_file_filter_add_pattern( filter, "*.gz" );
  gtk_file_filter_add_pattern( filter, "*.GZ" );
  gtk_file_filter_add_pattern( filter, "*.zip" );
  gtk_file_filter_add_pattern( filter, "*.ZIP" );
#endif

#ifdef LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION
  gtk_file_filter_add_pattern( filter, "*.bz2" );
  gtk_file_filter_add_pattern( filter, "*.BZ2" );
#endif
}

#endif

static void
add_filter_disk_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.d40" );
  gtk_file_filter_add_pattern( filter, "*.d80" );
  gtk_file_filter_add_pattern( filter, "*.dsk" );
  gtk_file_filter_add_pattern( filter, "*.fdi" );
  gtk_file_filter_add_pattern( filter, "*.img" );
  gtk_file_filter_add_pattern( filter, "*.mgt" );
  gtk_file_filter_add_pattern( filter, "*.opd" );
  gtk_file_filter_add_pattern( filter, "*.opu" );
  gtk_file_filter_add_pattern( filter, "*.sad" );
  gtk_file_filter_add_pattern( filter, "*.scl" );
  gtk_file_filter_add_pattern( filter, "*.td0" );
  gtk_file_filter_add_pattern( filter, "*.trd" );
  gtk_file_filter_add_pattern( filter, "*.udi" );

  gtk_file_filter_add_pattern( filter, "*.D40" );
  gtk_file_filter_add_pattern( filter, "*.D80" );
  gtk_file_filter_add_pattern( filter, "*.DSK" );
  gtk_file_filter_add_pattern( filter, "*.FDI" );
  gtk_file_filter_add_pattern( filter, "*.IMG" );
  gtk_file_filter_add_pattern( filter, "*.MGT" );
  gtk_file_filter_add_pattern( filter, "*.OPD" );
  gtk_file_filter_add_pattern( filter, "*.OPU" );
  gtk_file_filter_add_pattern( filter, "*.SAD" );
  gtk_file_filter_add_pattern( filter, "*.SCL" );
  gtk_file_filter_add_pattern( filter, "*.TD0" );
  gtk_file_filter_add_pattern( filter, "*.TRD" );
  gtk_file_filter_add_pattern( filter, "*.UDI" );
}

static void
add_filter_dock_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.dck" );
  gtk_file_filter_add_pattern( filter, "*.rom" );

  gtk_file_filter_add_pattern( filter, "*.DCK" );
  gtk_file_filter_add_pattern( filter, "*.ROM" );
}

static void
add_filter_harddisk_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.hdf" );

  gtk_file_filter_add_pattern( filter, "*.HDF" );
}

static void
add_filter_microdrive_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.mdr" );

  gtk_file_filter_add_pattern( filter, "*.MDR" );
}

static void
add_filter_movie_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.fmf" );

  gtk_file_filter_add_pattern( filter, "*.FMF" );
}

static void
add_filter_recording_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.rzx" );

  gtk_file_filter_add_pattern( filter, "*.RZX" );
}

static void
add_filter_snapshot_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.mgtsnp" );
  gtk_file_filter_add_pattern( filter, "*.slt" );
  gtk_file_filter_add_pattern( filter, "*.sna" );
  gtk_file_filter_add_pattern( filter, "*.snapshot" );
  gtk_file_filter_add_pattern( filter, "*.snp" );
  gtk_file_filter_add_pattern( filter, "*.sp" );
  gtk_file_filter_add_pattern( filter, "*.szx" );
  gtk_file_filter_add_pattern( filter, "*.z80" );
  gtk_file_filter_add_pattern( filter, "*.zx-state" );  

  gtk_file_filter_add_pattern( filter, "*.MGTSNP" );
  gtk_file_filter_add_pattern( filter, "*.SLT" );
  gtk_file_filter_add_pattern( filter, "*.SNA" );
  gtk_file_filter_add_pattern( filter, "*.SNAPSHOT" );
  gtk_file_filter_add_pattern( filter, "*.SNP" );
  gtk_file_filter_add_pattern( filter, "*.SP" );
  gtk_file_filter_add_pattern( filter, "*.SZX" );
  gtk_file_filter_add_pattern( filter, "*.Z80" );
  gtk_file_filter_add_pattern( filter, "*.ZX-STATE" );  
}

static void
add_filter_tape_files( GtkFileFilter *filter )
{
  gtk_file_filter_add_pattern( filter, "*.csw" );
  gtk_file_filter_add_pattern( filter, "*.ltp" );
  gtk_file_filter_add_pattern( filter, "*.pzx" );
  gtk_file_filter_add_pattern( filter, "*.raw" );
  gtk_file_filter_add_pattern( filter, "*.spc" );
  gtk_file_filter_add_pattern( filter, "*.sta" );
  gtk_file_filter_add_pattern( filter, "*.tzx" );
  gtk_file_filter_add_pattern( filter, "*.tap" );
  gtk_file_filter_add_pattern( filter, "*.wav" );

  gtk_file_filter_add_pattern( filter, "*.CSW" );
  gtk_file_filter_add_pattern( filter, "*.LTP" );
  gtk_file_filter_add_pattern( filter, "*.PZX" );
  gtk_file_filter_add_pattern( filter, "*.RAW" );
  gtk_file_filter_add_pattern( filter, "*.SPC" );
  gtk_file_filter_add_pattern( filter, "*.STA" );
  gtk_file_filter_add_pattern( filter, "*.TZX" );
  gtk_file_filter_add_pattern( filter, "*.TAP" );
  gtk_file_filter_add_pattern( filter, "*.WAV" );
}
