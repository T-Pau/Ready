/* filesel.c: File selection dialog box
   Copyright (c) 2001-2015 Matan Ziv-Av, Philip Kendall, Russell Marks,
			   Marek Januszewski
   Copyright (c) 2015 Sergio Baldoví

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

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_STRCASECMP
#include <strings.h>
#endif      /* #ifdef HAVE_STRINGS_STRCASECMP */
#include <sys/stat.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#include <ctype.h>
#endif				/* #ifdef WIN32 */

#include "fuse.h"
#include "ui/ui.h"
#include "utils.h"
#include "widget_internals.h"

#if defined AMIGA || defined __MORPHOS__
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>

struct Library *AslBase;

#ifndef __MORPHOS__
struct AslIFace *IAsl;
struct Library *DOSBase;
#endif				/* #ifndef __MORPHOS__ */

#ifndef __MORPHOS__
struct DOSIFace *IDOS;
struct Library *ExecBase;
#endif				/* #ifndef __MORPHOS__ */


int err = 0;

char *amiga_asl( char *title, BOOL is_saving );

#endif /* ifdef AMIGA */

struct widget_dirent **widget_filenames; /* Filenames in the current
					    directory */
size_t widget_numfiles;	  /* The number of files in the current
			     directory */

static const char *title;
static int is_saving;

#ifdef WIN32
static int is_drivesel = 0;
static int is_rootdir;
#endif				/* #ifdef WIN32 */

#define ENTRIES_PER_SCREEN (is_saving ? 32 : 36)

/* The number of the filename in the top-left corner of the current
   display, that of the filename which the `cursor' is on, and that
   which it will be on after this keypress */
static size_t top_left_file, current_file, new_current_file;

static char *widget_get_filename( const char *title, int saving );

static int widget_add_filename( int *allocated, int *number,
                                struct widget_dirent ***namelist,
                                const char *name );
static void widget_scan( char *dir );
static int widget_select_file( const char *name );
static int widget_scan_compare( const widget_dirent **a,
				const widget_dirent **b );

#if !defined AMIGA && !defined __MORPHOS__
static char* widget_getcwd( void );
#endif /* ifndef AMIGA */
static int widget_print_all_filenames( struct widget_dirent **filenames, int n,
				       int top_left, int current,
				       const char *dir );
static int widget_print_filename( struct widget_dirent *filename, int position,
				  int inverted );
#ifdef WIN32
static void widget_filesel_chdrv( void );
static void widget_filesel_drvlist( void );
#endif				/* #ifdef WIN32 */
static int widget_filesel_chdir( void );

/* The filename to return */
char* widget_filesel_name;

/* Should we exit all widgets when we're done with this selector? */
static int exit_all_widgets;

static char *
widget_get_filename( const char *title, int saving )
{
  char *filename = NULL;

  widget_filesel_data data;

  data.exit_all_widgets = 1;
  data.title = title;

  if( saving ) {
    widget_do_fileselector_save( &data );
  } else {
    widget_do_fileselector( &data );
  }
  if( widget_filesel_name )
    filename = utils_safe_strdup( widget_filesel_name );

  return filename;
  
}

char *
ui_get_open_filename( const char *title )
{
#if !defined AMIGA && !defined __MORPHOS__
  return widget_get_filename( title, 0 );
#else
  return amiga_asl( title, FALSE );
#endif
}

char *
ui_get_save_filename( const char *title )
{
#if !defined AMIGA && !defined __MORPHOS__
  return widget_get_filename( title, 1 );
#else
  return amiga_asl( title, TRUE );
#endif
}

static int widget_add_filename( int *allocated, int *number,
                                struct widget_dirent ***namelist,
                                const char *name ) {
  int i; size_t length;

  if( ++*number > *allocated ) {
    struct widget_dirent **oldptr = *namelist;

    *namelist = realloc( (*namelist), 2 * *allocated * sizeof(**namelist) );
    if( *namelist == NULL ) {
      for( i=0; i<*number-1; i++ ) {
	free( oldptr[i]->name );
	free( oldptr[i] );
      }
      free( oldptr );
      return -1;
    }
    *allocated *= 2;
  }

  (*namelist)[*number-1] = malloc( sizeof(***namelist) );
  if( !(*namelist)[*number-1] ) {
    for( i=0; i<*number-1; i++ ) {
      free( (*namelist)[i]->name );
      free( (*namelist)[i] );
    }
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  length = strlen( name ) + 1;
  if( length < 16 ) length = 16;

  (*namelist)[*number-1]->name = malloc( length );
  if( !(*namelist)[*number-1]->name ) {
    free( (*namelist)[*number-1] );
    for( i=0; i<*number-1; i++ ) {
      free( (*namelist)[i]->name );
      free( (*namelist)[i] );
    }
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  strncpy( (*namelist)[*number-1]->name, name, length );
  (*namelist)[*number-1]->name[ length - 1 ] = 0;

  return 0;
}

#if defined AMIGA || defined __MORPHOS__
char *
amiga_asl( char *title, BOOL is_saving ) {
  char *filename;
  struct FileRequester *filereq;

#ifndef __MORPHOS__
  if( AslBase = IExec->OpenLibrary( "asl.library", 52 ) ) {
    if( IAsl = ( struct AslIFace * ) IExec->GetInterface( AslBase,"main",1,NULL ) ) {
      filereq = IAsl->AllocAslRequestTags( ASL_FileRequest,
                                           ASLFR_RejectIcons,TRUE,
                                           ASLFR_TitleText,title,
                                           ASLFR_DoSaveMode,is_saving,
                                           ASLFR_InitialPattern,"#?.(sna|z80|szx|sp|snp|zxs|tap|tzx|csw|rzx|dsk|trd|scl|mdr|dck|hdf|rom|psg|scr|mlt|png|gz|bz2)",
                                           ASLFR_DoPatterns,TRUE,
                                           TAG_DONE );
      if( err = IAsl->AslRequest( filereq, NULL ) ) {
        filename = ( STRPTR ) IExec->AllocVec( 1024, MEMF_CLEAR );
#else				/* #ifndef __MORPHOS__ */
  if( AslBase = OpenLibrary( "asl.library", 0 ) ) {
      filereq = AllocAslRequestTags( ASL_FileRequest,
                                     ASLFR_RejectIcons,TRUE,
                                     ASLFR_TitleText,title,
                                     ASLFR_DoSaveMode,is_saving,
                                     ASLFR_InitialPattern,"#?.(sna|z80|szx|sp|snp|zxs|tap|tzx|csw|rzx|dsk|trd|scl|mdr|dck|hdf|rom|psg|scr|mlt|png|gz|bz2)",
                                     ASLFR_DoPatterns,TRUE,
                                     TAG_DONE );
      if( err = AslRequest( filereq, NULL ) ) {
        filename = ( STRPTR ) AllocVec( 1024, MEMF_CLEAR );
#endif				/* #ifndef __MORPHOS__ */

        strcpy( filename,filereq->fr_Drawer );	
#ifndef __MORPHOS__
        IDOS->AddPart( filename, filereq->fr_File, 1024 );
#else				/* #ifndef __MORPHOS__ */
        AddPart( filename, filereq->fr_File, 1024 );
#endif				/* #ifndef __MORPHOS__ */
        widget_filesel_name = utils_safe_strdup( filename );
#ifndef __MORPHOS__
        IExec->FreeVec( filename );
#else				/* #ifndef __MORPHOS__ */
        FreeVec( filename );
#endif				/* #ifndef __MORPHOS__ */
        err = WIDGET_FINISHED_OK;
      } else {
        err = WIDGET_FINISHED_CANCEL;
      }
#ifndef __MORPHOS__
      IExec->DropInterface( ( struct Interface * )IAsl );
    }
    IExec->CloseLibrary( AslBase );
#else				/* #ifndef __MORPHOS__ */
    CloseLibrary( AslBase );
#endif				/* #ifndef __MORPHOS__ */
  }
  return widget_filesel_name;
}
#else /* ifdef AMIGA */

static int widget_scandir( const char *dir, struct widget_dirent ***namelist,
			   int (*select_fn)(const char*) )
{
  compat_dir directory;

  int allocated, number;
  int i;
  int done = 0;

  *namelist = malloc( 32 * sizeof(**namelist) );
  if( !*namelist ) return -1;

  allocated = 32; number = 0;

  directory = compat_opendir( dir );
  if( !directory ) {
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

#ifdef WIN32
  /* Assume this is the root directory, unless we find an entry named ".." */
  is_rootdir = 1;
#endif				/* #ifdef WIN32 */

  while( !done ) {
    char name[ PATH_MAX ];

    compat_dir_result_t result =
      compat_readdir( directory, name, sizeof( name ) );

    switch( result )
    {
    case COMPAT_DIR_RESULT_OK:
      if( select_fn( name ) ) {
#ifdef WIN32
        if( is_rootdir && !strcmp( name, ".." ) ) {
          is_rootdir = 0;
        }
#endif				/* #ifdef WIN32 */
        if( widget_add_filename( &allocated, &number, namelist, name ) ) {
          compat_closedir( directory );
          return -1;
        }
      }
      break;

    case COMPAT_DIR_RESULT_END:
      done = 1;
      break;

    case COMPAT_DIR_RESULT_ERROR:
      for( i=0; i<number; i++ ) {
        free( (*namelist)[i]->name );
        free( (*namelist)[i] );
      }
      free( *namelist );
      *namelist = NULL;
      compat_closedir( directory );
      return -1;
    }

  }

  if( compat_closedir( directory ) ) {
    for( i=0; i<number; i++ ) {
      free( (*namelist)[i]->name );
      free( (*namelist)[i] );
    }
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

#ifdef WIN32
  if( is_rootdir ) {
    /* Add a fake ".." entry for drive selection */
    if( widget_add_filename( &allocated, &number, namelist, ".." ) ) {
      return -1;
    }
  }
#endif				/* #ifdef WIN32 */

  return number;
}

#ifdef WIN32
static int widget_scandrives( struct widget_dirent ***namelist )
{
  int allocated, number;
  unsigned long drivemask;
  int i;
  char drive[3];
  const char *driveletters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  drive[1] = ':';
  drive[2] = '\0';

  *namelist = malloc( 32 * sizeof(**namelist) );
  if( !*namelist ) return -1;

  allocated = 32; number = 0;

  drivemask = _getdrives();
  if( !drivemask ) {
    free( *namelist );
    *namelist = NULL;
    return -1;
  }

  for( i = 0; i < 26; i++ ) {
    if( drivemask & 1) {
      drive[0] = driveletters[i];
      if( widget_add_filename( &allocated, &number, namelist, drive ) ) {
        return -1;
      }
    }
    drivemask >>= 1;
  }

  return number;
}
#endif

static void widget_scan( char *dir )
{
  struct stat file_info;

  size_t i; int error;
  
  /* Free the memory belonging to the files in the previous directory */
  for( i=0; i<widget_numfiles; i++ ) {
    free( widget_filenames[i]->name );
    free( widget_filenames[i] );
  }

#ifdef WIN32
  if( dir ) {
    widget_numfiles = widget_scandir( dir, &widget_filenames,
				      widget_select_file );
  } else {
    widget_numfiles = widget_scandrives( &widget_filenames );
  }
#else				/* #ifdef WIN32 */
  widget_numfiles = widget_scandir( dir, &widget_filenames,
				    widget_select_file );
#endif				/* #ifdef WIN32 */

  if( widget_numfiles == (size_t)-1 ) return;

  for( i=0; i<widget_numfiles; i++ ) {
    error = stat( widget_filenames[i]->name, &file_info );
    widget_filenames[i]->mode = error ? 0 : file_info.st_mode;
  }

  qsort( widget_filenames, widget_numfiles, sizeof(struct widget_dirent*),
	 (int(*)(const void*,const void*))widget_scan_compare );

}

static int
widget_select_file( const char *name )
{
  if( !name ) return 0;

  /* Skip current directory */
  if( !strcmp( name, "." ) ) return 0;

#ifndef WIN32
  /* Skip hidden files/directories */
  if( strlen( name ) > 1 && name[0] == '.' && name[1] != '.' ) return 0;
#endif				/* #ifdef WIN32 */

  return 1;
}

static int widget_scan_compare( const struct widget_dirent **a,
				const struct widget_dirent **b )
{
  int isdir1 = S_ISDIR( (*a)->mode ),
      isdir2 = S_ISDIR( (*b)->mode );

  if( isdir1 && !isdir2 ) {
    return -1;
  } else if( isdir2 && !isdir1 ) {
    return 1;
  } else {
    return strcmp( (*a)->name, (*b)->name );
  }

}
#endif /* ifdef AMIGA */

/* File selection widget */

static int
widget_filesel_draw( void *data )
{
  widget_filesel_data *filesel_data = data;
  char *directory;
  int error;

  exit_all_widgets = filesel_data->exit_all_widgets;
  title = filesel_data->title;

#if !defined AMIGA && !defined __MORPHOS__
#ifdef WIN32
  if( !is_drivesel ) {
    directory = widget_getcwd();
    if( directory == NULL ) return 1;
  } else {
    directory = NULL;
  }
#else				/* #ifdef WIN32 */
  directory = widget_getcwd();
  if( directory == NULL ) return 1;
#endif				/* #ifdef WIN32 */

  widget_scan( directory );
  new_current_file = current_file = 0;
  top_left_file = 0;

  /* Create the dialog box */
  error = widget_dialog_with_border( 1, 2, 30, 22 );
  if( error ) {
    free( directory );
    return error; 
  }

#ifdef WIN32
  if( directory == NULL ) {
    directory = utils_safe_strdup( "Drive selection" );
  }
#endif				/* #ifdef WIN32 */

  /* Show all the filenames */
  widget_print_all_filenames( widget_filenames, widget_numfiles,
			      top_left_file, current_file, directory );

  free( directory );

#endif /* ifndef AMIGA */

  return 0;
}

int widget_filesel_finish( widget_finish_state finished ) {

  /* Return with null if we didn't finish cleanly */
  if( finished != WIDGET_FINISHED_OK ) {
    if( widget_filesel_name ) free( widget_filesel_name );
    widget_filesel_name = NULL;
  }

  return 0;
}

int
widget_filesel_load_draw( void *data )
{
  is_saving = 0;
  return widget_filesel_draw( data );
}

int
widget_filesel_save_draw( void *data )
{
  is_saving = 1;
  return widget_filesel_draw( data );
}

#if !defined AMIGA && !defined __MORPHOS__
static char* widget_getcwd( void )
{
  char *directory; size_t directory_length;
  char *ptr;

  directory_length = 64;
  directory = malloc( directory_length * sizeof( char ) );
  if( directory == NULL ) {
    return NULL;
  }

  do {
    ptr = getcwd( directory, directory_length );
    if( ptr ) break;
    if( errno == ERANGE ) {
      ptr = directory;
      directory_length *= 2;
      directory =
	(char*)realloc( directory, directory_length * sizeof( char ) );
      if( directory == NULL ) {
	free( ptr );
	return NULL;
      }
    } else {
      free( directory );
      return NULL;
    }
  } while(1);

#ifdef WIN32
  if( directory[0] && directory[1] == ':' ) {
    directory[0] = toupper( directory[0] );
  }
#endif

  return directory;
}

static int widget_print_all_filenames( struct widget_dirent **filenames, int n,
				       int top_left, int current,
				       const char *dir )
{
  int i;
  int error;

  /* Give us a clean box to start with */
  error = widget_dialog_with_border( 1, 2, 30, 22 );
  if( error ) return error;

  widget_printstring( 10, 16, WIDGET_COLOUR_TITLE, title );
  if( widget_stringwidth( dir ) > 223 ) {
    char buffer[128];
    int prefix = widget_stringwidth( "..." ) + 1;
    while( widget_stringwidth( dir ) > 223 - prefix ) dir++;
    snprintf( buffer, sizeof( buffer ), "...%s", dir );
    widget_print_title( 24, WIDGET_COLOUR_FOREGROUND, buffer );  
  } else {
    widget_print_title( 24, WIDGET_COLOUR_FOREGROUND, dir );
  }

  if( top_left ) widget_up_arrow( 1, 5, WIDGET_COLOUR_FOREGROUND );

  /* Print the filenames, mostly normally, but with the currently
     selected file inverted */
  for( i = top_left; i < n && i < top_left + ENTRIES_PER_SCREEN; i++ ) {
    if( i == current ) {
      widget_print_filename( filenames[i], i-top_left, 1 );
    } else {
      widget_print_filename( filenames[i], i-top_left, 0 );
    }
  }

  if( is_saving )
  {
    widget_printstring( 12, 22 * 8, WIDGET_COLOUR_FOREGROUND,
				     "\012RETURN\001 = select" );
    widget_printstring_right( 244, 22 * 8, WIDGET_COLOUR_FOREGROUND,
					   "\012TAB\001 = enter name" );
  }

  if( i < n )
    widget_down_arrow( 1, is_saving ? 20 : 22, WIDGET_COLOUR_FOREGROUND );

  /* Display that lot */
  widget_display_lines( 2, 22 );

  return 0;
}

/* Print a filename onto the dialog box */
static int widget_print_filename( struct widget_dirent *filename, int position,
				  int inverted )
{
  char buffer[64], suffix[64], *dot = 0;
  int width, suffix_width = 0;
  int dir = S_ISDIR( filename->mode );
  int truncated = 0, suffix_truncated = 0;

#define FILENAME_WIDTH 112
#define MAX_SUFFIX_WIDTH 56

  int x = (position & 1) ? 132 : 16,
      y = 40 + (position >> 1) * 8;

  int foreground = WIDGET_COLOUR_FOREGROUND,

      background = inverted ? WIDGET_COLOUR_HIGHLIGHT
                            : WIDGET_COLOUR_BACKGROUND;

  widget_rectangle( x, y, FILENAME_WIDTH, 8, background );

  strncpy( buffer, filename->name, sizeof( buffer ) - dir - 1);
  buffer[sizeof( buffer ) - dir - 1] = '\0';

  if (dir)
    dir = widget_charwidth( FUSE_DIR_SEP_CHR );
  else {
    /* get filename extension */
    dot = strrchr( filename->name, '.' );

    /* if .gz or .bz2, we want the previous component too */
    if( dot &&( !strcasecmp( dot, ".gz" ) || !strcasecmp( dot, ".bz2" ) ) ) {
      char *olddot = dot;
      *olddot = '\0';
      dot = strrchr( filename->name, '.' );
      *olddot = '.';
      if (!dot)
	dot = olddot;
    }

    /* if the dot is at the start of the name, ignore it */
    if( dot == filename->name )
      dot = 0;
  }

  if( dot ) {
    /* split filename at extension separator */
    if( dot - filename->name < sizeof( buffer ) )
      buffer[dot - filename->name] = '\0';

    /* get extension width (for display purposes) */
    snprintf( suffix, sizeof( suffix ), "%s", dot );
    while( ( suffix_width = ( dot && !dir )
	     ? widget_stringwidth( suffix ) : 0 ) > 110 ) {
      suffix_truncated = 1;
      suffix[strlen( suffix ) - 1] = '\0';
    }
  }

  while( ( width = widget_stringwidth( buffer ) ) >=
	 FILENAME_WIDTH - dir - ( dot ? truncated + suffix_width : 0 ) ) {
    truncated = 2;
    if( suffix_width >= MAX_SUFFIX_WIDTH ) {
      suffix_truncated = 2;
      suffix[strlen (suffix) - 1] = '\0';
      suffix_width = widget_stringwidth (suffix);
    }
    else
      buffer[strlen (buffer) - 1] = '\0';
  }
  if( dir )
    strcat (buffer, FUSE_DIR_SEP_STR );

  widget_printstring( x + 1, y, foreground, buffer );
  if( truncated )
    widget_rectangle( x + width + 2, y, 1, 8, 4 );
  if( dot )
    widget_printstring( x + width + 2 + truncated, y,
			foreground ^ 2, suffix );
  if( suffix_truncated )
    widget_rectangle( x + FILENAME_WIDTH, y, 1, 8, 4 );

  return 0;
}
#endif /* ifndef AMIGA */

#ifdef WIN32
static void
widget_filesel_chdrv( void )
{
  char *fn;

  if( chdir( widget_filenames[ current_file ]->name ) ) {
    ui_error( UI_ERROR_ERROR, "Could not change directory" );
    return;
  }

  is_drivesel = 0;
  fn = widget_getcwd();
  widget_scan( fn ); free( fn );
  new_current_file = 0;
  /* Force a redisplay of all filenames */
  current_file = 1; top_left_file = 1;
}

static void
widget_filesel_drvlist( void )
{
  is_drivesel = 1;
  widget_scan( NULL );
  new_current_file = 0;
  /* Force a redisplay of all filenames */
  current_file = 1; top_left_file = 1;
}
#endif				/* #ifdef WIN32 */

static int
widget_filesel_chdir( void )
{
  char *fn, *ptr;

  /* Get the new directory name */
  fn = widget_getcwd();
  if( fn == NULL ) {
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return 1;
  }
  ptr = fn;
  fn = realloc( fn,
     ( strlen( fn ) + 1 + strlen( widget_filenames[ current_file ]->name ) +
       1 ) * sizeof(char)
  );
  if( fn == NULL ) {
    free( ptr );
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    return 1;
  }
#ifndef GEKKO
  /* Wii getcwd() already has the slash on the end */
  strcat( fn, FUSE_DIR_SEP_STR );
#endif				/* #ifndef GEKKO */
  strcat( fn, widget_filenames[ current_file ]->name );

/*
in Win32 errno resulting from chdir on file is EINVAL which may mean many things
this will not be fixed in mingw - must use native function instead
http://thread.gmane.org/gmane.comp.gnu.mingw.user/9197
*/ 

  if( chdir( fn ) == -1 ) {
#ifndef WIN32
    if( errno == ENOTDIR ) {
#else   /* #ifndef WIN32 */
    if( GetFileAttributes( fn ) != FILE_ATTRIBUTE_DIRECTORY ) {
#endif  /* #ifndef WIN32 */
      widget_filesel_name = fn; fn = NULL;
      if( exit_all_widgets ) {
	widget_end_all( WIDGET_FINISHED_OK );
      } else {
	widget_end_widget( WIDGET_FINISHED_OK );
      }
    }
  } else {
    widget_scan( fn );
    new_current_file = 0;
    /* Force a redisplay of all filenames */
    current_file = 1; top_left_file = 1;
  }

  free( fn );

  return 0;
}

void
widget_filesel_keyhandler( input_key key )
{
#if !defined AMIGA && !defined __MORPHOS__
  char *fn, *ptr;
  char *dirtitle;
#endif

  /* If there are no files (possible on the Wii), can't really do anything */
  if( widget_numfiles == 0 ) {
    if( key == INPUT_KEY_Escape ) widget_end_widget( WIDGET_FINISHED_CANCEL );
    return;
  }
  
#if defined AMIGA || defined __MORPHOS__
  if( exit_all_widgets ) {
    widget_end_all( err );
  } else {
    widget_end_widget( err );
  }
#else  /* ifndef AMIGA */

  new_current_file = current_file;

  switch(key) {

#if 0
  case INPUT_KEY_Resize:	/* Fake keypress used on window resize */
    widget_dialog_with_border( 1, 2, 30, 20 );
    widget_print_all_filenames( widget_filenames, widget_numfiles,
				top_left_file, current_file        );
    break;
#endif
    
  case INPUT_KEY_Escape:
  case INPUT_JOYSTICK_FIRE_2:
    widget_end_widget( WIDGET_FINISHED_CANCEL );
    break;
  
  case INPUT_KEY_Left:
  case INPUT_KEY_5:
  case INPUT_KEY_h:
  case INPUT_JOYSTICK_LEFT:
    if( current_file > 0                 ) new_current_file--;
    break;

  case INPUT_KEY_Down:
  case INPUT_KEY_6:
  case INPUT_KEY_j:
  case INPUT_JOYSTICK_DOWN:
    if( current_file+2 < widget_numfiles ) new_current_file += 2;
    break;

  case INPUT_KEY_Up:
  case INPUT_KEY_7:		/* Up */
  case INPUT_KEY_k:
  case INPUT_JOYSTICK_UP:
    if( current_file > 1                 ) new_current_file -= 2;
    break;

  case INPUT_KEY_Right:
  case INPUT_KEY_8:
  case INPUT_KEY_l:
  case INPUT_JOYSTICK_RIGHT:
    if( current_file < widget_numfiles-1 ) new_current_file++;
    break;

  case INPUT_KEY_Page_Up:
    new_current_file = ( current_file > ENTRIES_PER_SCREEN ) ?
                       current_file - ENTRIES_PER_SCREEN     :
                       0;
    break;

  case INPUT_KEY_Page_Down:
    new_current_file = current_file + ENTRIES_PER_SCREEN;
    if( new_current_file >= widget_numfiles )
      new_current_file = widget_numfiles - 1;
    break;

  case INPUT_KEY_Home:
    new_current_file = 0;
    break;

  case INPUT_KEY_End:
    new_current_file = widget_numfiles - 1;
    break;

  case INPUT_KEY_Tab:
    if( is_saving ) {
      widget_text_t text_data;
      text_data.title = title;
      text_data.allow = WIDGET_INPUT_ASCII;
      text_data.max_length = 30;
      text_data.text[0] = 0;
      if( widget_do_text( &text_data ) ||
	  !widget_text_text || !*widget_text_text      )
	break;
      if( !compat_is_absolute_path( widget_text_text ) ) {
							/* relative name */
        /* Get current dir name and allocate space for the leafname */
        fn = widget_getcwd();
        ptr = fn;
        if( fn )
    	  fn = realloc( fn, strlen( fn ) + strlen( widget_text_text ) + 2 );
        if( !fn ) {
          free( ptr );
	  widget_end_widget( WIDGET_FINISHED_CANCEL );
	  return;
        }
        /* Append the leafname and return it */
        strcat( fn, FUSE_DIR_SEP_STR );
        strcat( fn, widget_text_text );
      } else {						/* absolute name */
	fn = utils_safe_strdup( widget_text_text );
      }
      widget_filesel_name = fn;
      if( exit_all_widgets ) {
	widget_end_all( WIDGET_FINISHED_OK );
      } else {
	widget_end_widget( WIDGET_FINISHED_OK );
      }
    }
    break;

  case INPUT_KEY_Return:
  case INPUT_KEY_KP_Enter:
  case INPUT_JOYSTICK_FIRE_1:
#ifdef WIN32
    if( is_drivesel ) {
      widget_filesel_chdrv();
    } else if( is_rootdir &&
	       !strcmp( widget_filenames[ current_file ]->name, ".." ) ) {
      widget_filesel_drvlist();
    } else {
#endif				/* #ifdef WIN32 */
      if( widget_filesel_chdir() ) return;
#ifdef WIN32
    }
#endif				/* #ifdef WIN32 */
    break;

  default:	/* Keep gcc happy */
    break;

  }

#ifdef WIN32
  if( is_drivesel ) {
    dirtitle = utils_safe_strdup( "Drive selection" );
  } else {
#endif				/* #ifdef WIN32 */
    dirtitle = widget_getcwd();
#ifdef WIN32
  }
#endif				/* #ifdef WIN32 */

  /* If we moved the cursor */
  if( new_current_file != current_file ) {

    /* If we've got off the top or bottom of the currently displayed
       file list, then reset the top-left corner and display the whole
       thing */
    if( new_current_file < top_left_file ) {

      top_left_file = new_current_file & ~1;
      widget_print_all_filenames( widget_filenames, widget_numfiles,
				  top_left_file, new_current_file, dirtitle );

    } else if( new_current_file >= top_left_file+ENTRIES_PER_SCREEN ) {

      top_left_file = new_current_file & ~1;
      top_left_file -= ENTRIES_PER_SCREEN - 2;
      widget_print_all_filenames( widget_filenames, widget_numfiles,
				  top_left_file, new_current_file, dirtitle );

    } else {

      /* Otherwise, print the current file uninverted and the
	 new current file inverted */

      widget_print_filename( widget_filenames[ current_file ],
			     current_file - top_left_file, 0 );
	  
      widget_print_filename( widget_filenames[ new_current_file ],
			     new_current_file - top_left_file, 1 );
        
      widget_display_lines( 2, 21 );
    }

    /* Reset the current file marker */
    current_file = new_current_file;

  }

  free( dirtitle );
#endif /* ifdef AMIGA */
}
