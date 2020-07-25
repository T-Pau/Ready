/* svg.c: Routines to capture ROM graphics statements to Scalable Vector
          Graphics files
   Copyright (c) 2014 Stefano Bodrato
   Portions taken from svgwrite.c, (c) J.J. Green 2005

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

   E-mail: stefano@bodrato.it

*/

#include <config.h>

#ifdef HAVE_LIB_XML2
#include <errno.h>

#include <string.h>
#include <time.h>
#include <math.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#define ENCODING "utf-8"
#endif

#include "machine.h"
#include "memory_pages.h"
#include "svg.h"
#include "ui/ui.h"
#include "z80/z80.h"

#define SVG_NAME_LEN 128
#define BUFSZ 128

int svg_capture_active = 0;

#ifdef HAVE_LIB_XML2

static svg_capture_type svg_capture_mode = SVG_CAPTURE_LINES;

static char *svg_fname;
static char *svg_fnameroot;

static int svg_filecount;

static int svg_flag = 0;
static int svg_y_size = 176;

xmlBuffer* buffer;
xmlTextWriter* writer;


static const
libspectrum_byte palette[16][3] = {
                                    /*  R    G    B */
                                    {   0,   0,   0 },
                                    {   0,   0, 192 },
                                    { 192,   0,   0 },
                                    { 192,   0, 192 },
                                    {   0, 192,   0 },
                                    {   0, 192, 192 },
                                    { 200, 200,   0 },
                                    { 220, 220, 220 },
                                    {   0,   0,   0 },
                                    {   0,   0, 255 },
                                    { 255,   0,   0 },
                                    { 255,   0, 255 },
                                    {   0, 255,   0 },
                                    {   0, 255, 255 },
                                    { 255, 255,   0 },
                                    { 255, 255, 255 } };


/* handle error messages setting attributes */

static int
svg_attribute( const char *name, const char *value, const char *element)
{
  if( xmlTextWriterWriteAttribute( writer, BAD_CAST name, BAD_CAST value ) < 0 )
    return 1;

  return 0;
}

static const char*
timestring( void )
{
  time_t tm;
  char *tmstr;
  static char ts[25]; 

  time( &tm );
  tmstr = ctime( &tm );

  snprintf( ts, 25, "%.24s", tmstr );

  return ts;
}

static int
svg_write_metadata( void )
{
  if( xmlTextWriterStartElement( writer, BAD_CAST "metadata" ) < 0 )
    return 1;

  if( xmlTextWriterStartElement( writer, BAD_CAST "creator" ) < 0 )
    return 1;

  if( svg_attribute( "name", "fuse", "creator" ) != 0 )
    return 1;

  if( svg_attribute( "version", VERSION, "creator" ) != 0 )
    return 1;

  if( xmlTextWriterEndElement( writer ) < 0 )
    return 1;

  if( xmlTextWriterStartElement( writer, BAD_CAST "created" ) < 0 )
    return 1;

  if( svg_attribute( "date", timestring(), "created" ) != 0 )
    return 1;

  if( xmlTextWriterEndElement( writer ) < 0 )
    return 1;

  if( xmlTextWriterEndElement( writer ) < 0 )
    return 1;

  return 0;
}

void
svg_openfile( void )
{
  int err;

  if( ( buffer = xmlBufferCreate() ) == NULL ) {
    ui_error( UI_ERROR_ERROR, "error creating the XML buffer" );
    return;
  }

  if( ( writer = xmlNewTextWriterMemory( buffer, 0 ) ) == NULL ) {
    ui_error( UI_ERROR_ERROR, "error allocating the XML writer" );
    return;
  }

  /*
  we free the writer at the start of each of
  these branches since this must be done before
  using the buffer
  */
    
  /* start xml */

  if( xmlTextWriterStartDocument( writer, NULL, ENCODING, NULL ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error on SVG document start" );
    return;
  }

  /* svg */

  if( xmlTextWriterStartElement( writer, BAD_CAST "svg" ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error on SVG open" );
    return;
  }

  err = 0;
  err += ( svg_attribute( "version", "1.1", "svg" ) != 0 );
  err += ( svg_attribute( "xmlns", "http://www.w3.org/2000/svg", "svg" ) != 0 );

  if( err ) {
    ui_error( UI_ERROR_ERROR, "error setting the SVG attributes" );
    return;
  }
}

void
svg_rect( int xpos, int ypos, const char *xsize, const char *ysize,
          const char *opacity, const char *color )
{
  char path_element[ BUFSZ ];
  int err;

  if( xmlTextWriterStartElement( writer, BAD_CAST "rect" ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error creating a box in the SVG file" );
    return;
  }

  err = 0;
  snprintf( path_element, BUFSZ, "%d", xpos );
  err += ( svg_attribute( "x", path_element, "rect" ) != 0 );
  snprintf( path_element, BUFSZ, "%d", ypos );
  err += ( svg_attribute( "y", path_element, "rect" ) != 0 );
  err += ( svg_attribute( "width", xsize, "rect" ) != 0 );
  err += ( svg_attribute( "height", ysize, "rect" ) != 0 );
  err += ( svg_attribute( "opacity", opacity, "rect" ) != 0 );
  err += ( svg_attribute( "stroke", "none", "rect" ) != 0 );
  err += ( svg_attribute( "fill", color, "rect" ) != 0 );

  if( err ) {
    ui_error( UI_ERROR_ERROR, "error setting box coordinates in the SVG file" );
    return;
  }

  if( xmlTextWriterEndElement( writer ) < 0 ) {
    ui_error( UI_ERROR_ERROR,
              "error finishing a box creation in the SVG file" );
    return;
  }
}

void
svg_openpath( void )
{
  int err;
  char svgcolor[ BUFSZ ];

  static int svg_paper;
  static int svg_ink;

  /* Background */
  svg_paper = ( ( readbyte_internal( 0x5c8d ) >> 3 ) & 15 ); /* ATTR_P */
  snprintf( svgcolor, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_paper ][0],
                                              palette[ svg_paper ][1],
                                              palette[ svg_paper ][2] );

  svg_rect( 0, 0, "512", "368", "1", svgcolor );

  if( xmlTextWriterStartElement( writer, BAD_CAST "g" ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error opening the SVG element" );
    return;
  }

  svg_ink = ( readbyte_internal( 0x5c8d ) & 7 ) +
            ( ( readbyte_internal( 0x5c8d ) >> 3 ) & 8 );  /* ATTR_P */

  err = 0;
  snprintf( svgcolor, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_ink ][0],
                                              palette[ svg_ink ][1],
                                              palette[ svg_ink ][2] );
  err += ( svg_attribute( "stroke", svgcolor, "g" ) != 0 );
  err += ( svg_attribute( "stroke-width", "1.7", "g" ) != 0 );

  if( err ) {
    ui_error( UI_ERROR_ERROR, "error setting the SVG element attributes" );
    return;
  }
}

void
svg_closepath( void )
{
  if( xmlTextWriterEndElement( writer ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error on SVG element close" );
    return;
  }
}

void
svg_closefile( void )
{
  FILE *fp;

  if( svg_write_metadata() != 0 ) {
    ui_error( UI_ERROR_ERROR, "error writing the SVG metadata" );
    return;
  }

  if( xmlTextWriterEndElement( writer ) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error on SVG close" );
    return;
  }

  if( xmlTextWriterEndDocument( writer) < 0 ) {
    ui_error( UI_ERROR_ERROR, "error on SVG document end" );
    return;
  }

  xmlFreeTextWriter( writer );

  svg_fname = libspectrum_new( char, strlen( svg_fnameroot ) + BUFSIZ );
  snprintf( svg_fname, strlen( svg_fnameroot ) + BUFSIZ, "%s%d.svg", 
            svg_fnameroot, svg_filecount++ );

  if( ( fp = fopen( svg_fname, "w" ) ) != NULL ) {
    fprintf( fp, "%s", buffer->content );

    if( fclose( fp ) != 0 ) {
      ui_error( UI_ERROR_ERROR, "error closing SVG file '%s': %s", svg_fname,
                strerror( errno ) );
    }
  }

  libspectrum_free( svg_fname );
}



/* some init, open file (name)*/
void
svg_startcapture( const char *name, svg_capture_type mode )
{
  if( !svg_capture_active ) {

    svg_capture_mode = mode;

    if( name == NULL || *name == '\0' )
      name = "fuse";

    svg_fnameroot = libspectrum_new( char, strlen ( name ) );
    strcpy( svg_fnameroot, name );
    svg_filecount = 0;

    svg_capture_active = 1;
    svg_flag = 0;
    svg_y_size = 176;

    svg_openfile();
    svg_openpath();
  }
}

void
svg_stopcapture( void )
{
  if( svg_capture_active ) {
    svg_closepath();
    svg_closefile();

    libspectrum_free( svg_fnameroot );

    svg_capture_active = 0;
  }
}

void
svg_capture_draw( void )
{
  char path_element[ BUFSZ ];
  char svgcolor[ BUFSZ ];
  int err;
  int dx,dy;
  int svg_ink;

  dx = z80.bc.b.l;
  dy = z80.bc.b.h;
  if( z80.de.b.l == 255 )
    dx = -dx;
  if( z80.de.b.h == 255 )
    dy = -dy;

  svg_ink = ( readbyte_internal( 0x5c8f ) & 7 ) +
            ( ( readbyte_internal( 0x5c8f ) >> 3 ) & 8 );  /* ATTR_T */
  snprintf( svgcolor, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_ink ][0],
                                              palette[ svg_ink ][1],
                                              palette[ svg_ink ][2] );

  if( ( svg_capture_mode == SVG_CAPTURE_LINES ) &&
      ( ( z80.pc.w == 0x24ba ) || ( z80.pc.w == 0x2813 ) ) ) {
    /* DRAW */
    if( xmlTextWriterStartElement( writer, BAD_CAST "line" ) < 0 ) {
      ui_error( UI_ERROR_ERROR, "error creating the SVG line" );
      return;
    }
    err = 0;
    snprintf( path_element, BUFSZ, "%d", readbyte_internal( 0x5c7d ) * 2 );
    err += ( svg_attribute( "x1", path_element, "line" ) != 0 );
    snprintf( path_element, BUFSZ, "%d",
              ( svg_y_size - 1 - readbyte_internal( 0x5c7e ) ) * 2 );
    err += ( svg_attribute( "y1", path_element, "line" ) != 0 );

    snprintf( path_element, BUFSZ, "%d",
              ( readbyte_internal( 0x5c7d ) + dx ) * 2 );
    err += ( svg_attribute( "x2", path_element, "line" ) != 0 );
    snprintf( path_element, BUFSZ, "%d",
              ( ( svg_y_size - 1 - readbyte_internal( 0x5c7e ) - dy ) * 2 ) );
    err += ( svg_attribute( "y2", path_element, "line" ) != 0 );
    err += ( svg_attribute( "stroke", svgcolor, "line" ) != 0 );

  } else {
    /* PLOT */
    if( xmlTextWriterStartElement( writer, BAD_CAST "line" ) < 0 ) {
      ui_error( UI_ERROR_ERROR, "error creating the SVG DOT" );
      return;
    }
    err = 0;
    snprintf( path_element, BUFSZ, "%d", dx * 2 );
    err += ( svg_attribute( "x1", path_element, "line" ) != 0 );
    err += ( svg_attribute( "x2", path_element, "line" ) != 0 );
    snprintf( path_element, BUFSZ, "%d", ( ( svg_y_size - dy ) * 2 ) - 1 );
    err += ( svg_attribute( "y1", path_element, "line" ) != 0 );
    snprintf( path_element, BUFSZ, "%d", ( ( svg_y_size - dy ) * 2 ) - 2 );
    err += ( svg_attribute( "y2", path_element, "line" ) != 0 );
    err += ( svg_attribute( "stroke", svgcolor, "line" ) != 0 );
  }

  if( err ) {
    ui_error( UI_ERROR_ERROR, "error setting the SVG element coordinates" );
    return;
  }

  if( xmlTextWriterEndElement( writer ) < 0 ) {
    ui_error( UI_ERROR_ERROR,
              "error finishing the creation of the SVG element" );
    return;
  }

  svg_flag = 1;

  return;
}

void
svg_byte( int xpos, int ypos, int udg_byte, char *color )
{
  int i;
  char path_element[ BUFSZ ];
  int err = 0;
  
  for( i = 7; i >= 0; i-- ) {
    if( udg_byte & 1 ) {
      if( xmlTextWriterStartElement( writer, BAD_CAST "circle" ) < 0 ) {
        ui_error( UI_ERROR_ERROR, "error creating an UDG dot in the SVG file" );
        return;
      }
      snprintf( path_element,BUFSZ, "%d", xpos + ( i * 2 ) );
      err += ( svg_attribute( "cx", path_element, "circle" ) != 0 );
      snprintf( path_element, BUFSZ, "%d", ypos + 1 );
      err += ( svg_attribute( "cy", path_element, "circle" ) != 0 );
      err += ( svg_attribute( "r", "0.17", "circle" ) != 0 );
      err += ( svg_attribute( "stroke", color, "circle" ) != 0 );
      err += ( svg_attribute( "fill", color, "circle" ) != 0 );

      if( err ) {
        ui_error( UI_ERROR_ERROR, "error setting the SVG UDG dot coordinates" );
        return;
      }

      if( xmlTextWriterEndElement( writer ) < 0 ) {
        ui_error( UI_ERROR_ERROR,
                  "error finishing the creation of a dot in the UDG element" );
        return;
      }
    }
    udg_byte /= 2;
  }
}

void
svg_capture_char( void )
{
  char path_element[ BUFSZ ];
  char svgcolor_ink[ BUFSZ ];
  char svgcolor_paper[ BUFSZ ];
  int i, err;
  int svg_ink;
  int svg_paper;
  int x_pos, y_pos;
  int svg_char, udg_ptr;

  if( ( z80.de.b.l == 188 ) ||
      ( ( z80.de.b.l == 70 ) &&
        ( ( machine_current->machine == LIBSPECTRUM_MACHINE_TS2068 ) ||
          ( machine_current->machine == LIBSPECTRUM_MACHINE_TC2068 ) ) ) ) {
    x_pos = 33 - readbyte_internal( 0x5c88 );
    y_pos = ( svg_y_size / 8 ) + 3 - readbyte_internal( 0x5c89 );
    if( x_pos > 31 ) {
      x_pos = 0; y_pos++;
    }

    svg_ink = ( readbyte_internal( 0x5c8f ) & 7 ) +
              ( ( readbyte_internal( 0x5c8f ) >> 3 ) & 8 );  /* ATTR_T */
    svg_paper = ( ( readbyte_internal( 0x5c8f ) >> 3 ) & 15 ); /* ATTR_T */
    if( readbyte_internal( 0x5c91 ) & 4 ) {  /* INVERSE */
      svg_char = svg_paper;
      svg_paper = svg_ink;
      svg_ink = svg_char;
    }

    snprintf( svgcolor_paper, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_paper ][0],
                                                      palette[ svg_paper ][1],
                                                      palette[ svg_paper ][2] );
    snprintf( svgcolor_ink, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_ink ][0],
                                                    palette[ svg_ink ][1],
                                                    palette[ svg_ink ][2] );
    svg_char = z80.af.b.h;


    /* -- PAPER */
    if( svg_char < 165 )
      svg_rect( x_pos * 16, ( y_pos - 1 ) * 16, "16", "16", "0.4",
                svgcolor_paper );

    if( svg_char > 127 ) {

      if( svg_char < 144 ) {
        /* GRAPHICS BLOCKS */
        if( svg_char & 1 )
          svg_rect( 8 + x_pos * 16, ( y_pos - 1 ) * 16, "8", "8", "1",
                    svgcolor_ink );

        if( svg_char & 2 )
          svg_rect( x_pos * 16, ( y_pos - 1 ) * 16, "8", "8", "1",
                    svgcolor_ink );
        
        if( svg_char & 4 )
          svg_rect( 8 + x_pos * 16, 8 + ( y_pos - 1 ) * 16, "8", "8", "1",
                    svgcolor_ink );
        
        if( svg_char & 8 )
          svg_rect( x_pos * 16, 8 + ( y_pos - 1 ) * 16, "8", "8", "1",
                    svgcolor_ink );
      }
      else if( svg_char < 165 ) {
        /* 144-164 -> UDG "A"-"U" (pointed by $5C7B/$5C7C) */
        for( i = 0; i < 8; i++ ) {
          udg_ptr = ( readbyte_internal( 0x5c7c ) << 8 ) +
                      readbyte_internal( 0x5c7b );
          svg_byte( x_pos * 16, ( i * 2 ) + ( y_pos - 1 ) * 16,
                    readbyte_internal( udg_ptr + i + 8 * ( svg_char - 144 ) ),
                    svgcolor_ink );
        }
      }

    } else {


      /* -- INK and char */  

      if( xmlTextWriterStartElement( writer, BAD_CAST "text" ) < 0 ) {
        ui_error( UI_ERROR_ERROR, "error creating the SVG text element" );
        return;
      }

      err = 0;
      snprintf( path_element, BUFSZ, "%d", x_pos * 16 + 2 );
      err += ( svg_attribute( "x", path_element, "text" ) != 0 );
      snprintf( path_element, BUFSZ, "%d", y_pos * 16 - 3 );
      err += ( svg_attribute( "y", path_element, "text" ) != 0 );
      err += ( svg_attribute( "font-family", "monospace", "text" ) != 0 );
      err += ( svg_attribute( "stroke", svgcolor_ink, "text" ) != 0 );

      /* FLASH attribute */
      if( ( readbyte_internal( 0x5c8f ) & 128 ) != 0 ) {
        err += ( svg_attribute( "font-weight", "bold", "text" ) != 0 );
        err += ( svg_attribute( "stroke-width", "0.9", "text" ) != 0 );
        err += ( svg_attribute( "font-size", "21", "text" ) != 0 );
        err += ( svg_attribute( "fill", svgcolor_paper, "text" ) != 0 );
      } else {
        err += ( svg_attribute( "stroke-width", "0.7", "text" ) != 0 );
        err += ( svg_attribute( "font-size", "19", "text" ) != 0 );
        err += ( svg_attribute( "fill", svgcolor_ink, "text" ) != 0 );
      }

      if( err ) {
        ui_error( UI_ERROR_ERROR,
                  "error setting the SVG text element coordinates" );
        return;
      }

      if( svg_char == 127 ) /* (C) symbol */
          snprintf( path_element, BUFSZ, "%c%c", 0xc2, 0xa9 );
      else        /* Normal Character */
          snprintf( path_element, BUFSZ, "%c", svg_char );

      xmlTextWriterWriteString( writer, BAD_CAST path_element );

      if( xmlTextWriterEndElement( writer ) < 0 ) {
        ui_error( UI_ERROR_ERROR, "error closing the SVG text element" );
        return;
      }
    }

    svg_flag = 1;
  }

  return;
}

void
svg_capture_cls( void )
{
  if( svg_flag ) {
    svg_closepath();
    svg_closefile();
    svg_openfile();
    svg_openpath();
  } else {
    /* This trick could load empty fake paths into the SVG file when CLS
       happens */
    svg_closepath();
    svg_openpath();
  }

  svg_flag = 0;
  svg_y_size = 176;

  return;
}

void
svg_capture_scroll( void )
{
  char svgcolor[ BUFSZ ];
  static int svg_paper;

  /* Background */
  svg_paper = ( ( readbyte_internal( 0x5c8d ) >> 3 ) & 15 ); /* ATTR_P */
  snprintf( svgcolor, BUFSZ, "rgb(%u,%u,%u)", palette[ svg_paper ][0],
                                              palette[ svg_paper ][1], 
                                              palette[ svg_paper ][2] );

  svg_y_size += 8;
  svg_rect( 0, svg_y_size * 2, "512", "16", "1", svgcolor );
    return;
}

void
svg_capture( void )
{
  if( trap_check_rom( CHECK_48K_ROM ) ) {

    if( ( machine_current->machine == LIBSPECTRUM_MACHINE_TS2068 ) ||
        ( machine_current->machine == LIBSPECTRUM_MACHINE_TC2068 ) ) {
      if( ( svg_capture_mode == SVG_CAPTURE_DOTS ) && ( z80.pc.w == 0x263e ) )
        svg_capture_draw();

      if( ( svg_capture_mode == SVG_CAPTURE_LINES ) &&
          ( ( z80.pc.w == 0x2813 || z80.pc.w == 0x263e ) ) )
        svg_capture_draw();

      if( z80.pc.w == 0x63b )
        svg_capture_char();

      if( z80.pc.w == 0x08ea )
        svg_capture_cls();

      if( z80.pc.w == 0x0939 )
        svg_capture_scroll();
    } else {
        if( ( svg_capture_mode == SVG_CAPTURE_DOTS ) && ( z80.pc.w == 0x22e5 ) )
          svg_capture_draw();

        if( ( svg_capture_mode == SVG_CAPTURE_LINES ) &&
            ( ( z80.pc.w == 0x24ba || z80.pc.w == 0x22df ) ) )
          svg_capture_draw();

        if( z80.pc.w == 0x0b24 )
          svg_capture_char();

        if( z80.pc.w == 0x0daf )
          svg_capture_cls();

        if( z80.pc.w == 0x0e00 )
          svg_capture_scroll();
      }
    }
}

void
svg_capture_end( void )
{
  svg_stopcapture();
  return;
}

#else  /* !HAVE_LIB_XML2 */

void
svg_capture( void )
{
  return;
}

void
svg_capture_end( void )
{
  return;
}

#endif  /* HAVE_LIB_XML2 */
