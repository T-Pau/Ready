/* printer.c: Printer support
   Copyright (c) 2001-2016 Ian Collier, Russell Marks, Philip Kendall
   Copyright (c) 2015 Stuart Brady
   Copyright (c) 2015 Fredrick Meunier
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

/* The ZX Printer support is based on Ian Collier's emulation in xz80.
 * Well, `based' is an understatement, it's almost exactly the same. :-)
 */
   
#include <config.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fuse.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "memory_pages.h"
#include "module.h"
#include "printer.h"
#include "settings.h"
#include "ui/ui.h"

static int printer_graphics_enabled=0;
static int printer_text_enabled=0;
static FILE *printer_graphics_file=NULL;
static FILE *printer_text_file=NULL;

/* for the ZX Printer */
static int zxpframes,zxpspeed,zxpnewspeed;
static libspectrum_dword zxpcycles;
static int zxpheight,zxppixel,zxpstylus;
static unsigned char zxpline[256];
static unsigned int frames=0;
static unsigned int zxplineofchar=0;

/* last 8 pixel lines output, as bitmap */
static unsigned char zxplast8[32*8];


/* for parallel */
static unsigned char parallel_data=0;

/* see printer_parallel_strobe_write() comment; must be less than one
 * frame's worth.
 */
#define PARALLEL_STROBE_MAX_CYCLES	10000

static void printer_zxp_reset(int hard_reset);
static libspectrum_byte printer_zxp_read( libspectrum_word port, libspectrum_byte *attached );
static void printer_zxp_write( libspectrum_word port, libspectrum_byte b );
static libspectrum_byte printer_parallel_read(libspectrum_word port GCC_UNUSED,
				              libspectrum_byte *attached);

static void zx_printer_snapshot_enabled( libspectrum_snap *snap );
static void zx_printer_to_snapshot( libspectrum_snap *snap );

static module_info_t printer_zxp_module_info = {

  /* .reset = */ printer_zxp_reset,
  /* .romcs = */ NULL,
  /* .snapshot_enabled = */ zx_printer_snapshot_enabled,
  /* .snapshot_from = */ NULL,
  /* .snapshot_to = */ zx_printer_to_snapshot,

};

static const periph_port_t printer_zxp_ports[] = {
  { 0x0004, 0x0000, printer_zxp_read, printer_zxp_write },
  { 0, 0, NULL, NULL }
};

static const periph_t printer_zxp_periph = {
  /* .option = */ &settings_current.zxprinter,
  /* .ports = */ printer_zxp_ports,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static const periph_port_t printer_zxp_ports_full_decode[] = {
  { 0x00ff, 0x00fb, printer_zxp_read, printer_zxp_write },
  { 0, 0, NULL, NULL }
};

static const periph_t printer_zxp_periph_full_decode = {
  /* .option = */ &settings_current.zxprinter,
  /* .ports = */ printer_zxp_ports_full_decode,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static const periph_port_t printer_parallel_ports[] = {
  { 0xf002, 0x0000, printer_parallel_read, printer_parallel_write },
  { 0, 0, NULL, NULL }
};

static const periph_t printer_parallel_periph = {
  /* .option = */ &settings_current.printer,
  /* .ports = */ printer_parallel_ports,
  /* .hard_reset = */ 0,
  /* .activate = */ NULL,
};

static void printer_zxp_init(void)
{
zxpstylus=zxpspeed=zxpheight=zxpnewspeed=zxplineofchar=0;
zxppixel=-1;
module_register(&printer_zxp_module_info);
periph_register(PERIPH_TYPE_ZXPRINTER,&printer_zxp_periph);
periph_register(PERIPH_TYPE_ZXPRINTER_FULL_DECODE,&printer_zxp_periph_full_decode);
periph_register(PERIPH_TYPE_PARALLEL_PRINTER,&printer_parallel_periph);
}


static void printer_text_init(void)
{
/* nothing yet, just a placeholder */
}


static int printer_zxp_open_file(void)
{
static const char * const pbmstart="P4\n256 ";
FILE *tmpf;
int overwrite=1;

if(!printer_graphics_enabled || !settings_current.printer_graphics_filename)
  return(0);

/* first, see if there's an existing file we can add to. */
if((tmpf=fopen(settings_current.printer_graphics_filename,"rb"))!=NULL)
  {
  char buf[7+10+1];		/* 7 being length of pbmstart */

  /* check it has a header in our slightly odd format. */
  if(fread(buf,1,sizeof(buf),tmpf)==sizeof(buf) &&
     memcmp(buf,pbmstart,strlen(pbmstart))==0 &&
     buf[sizeof(buf)-1]=='\n')
    {
    char *ptr=buf+strlen(pbmstart);
    int f,want_space=1;
    
    /* looks good so far, but now we need to check that
     * the height field looks ok. It should be all-spaces
     * until we get a digit, then all-digits.
     */
    for(f=0;f<10;f++,ptr++)
      {
      /* make sure it's a space or digit */
      if(!strchr(" 0123456789",*ptr))
        break;
      
      if(want_space)
        {
        if(*ptr!=' ')
          want_space=0;
        }
      else
        if(!isdigit( (int)*ptr ))
          break;
      }

    /* if it got through that, reuse the file */
    if(f==10)
      {
      overwrite=0;
      zxpheight=atoi(buf+strlen(pbmstart));
      }
    }

  fclose(tmpf);
  }

if((printer_graphics_file=fopen(settings_current.printer_graphics_filename,
                                overwrite?"wb":"r+b"))==NULL)
  {
  ui_error(UI_ERROR_ERROR,"Couldn't open '%s', graphics printout disabled",
	   settings_current.printer_graphics_filename);
  printer_graphics_enabled=0;
  return(0);
  }

if(overwrite)
  {
  /* we reserve 10 chars for height */
  fputs(pbmstart,printer_graphics_file);
  fprintf(printer_graphics_file,"%10d\n",0);
  }
else
  {
  /* if appending, seek to the correct place */
  if(fseek(printer_graphics_file,
           strlen(pbmstart)+10+1+(256/8)*zxpheight,
           SEEK_SET)!=0)
    {
    ui_error(UI_ERROR_ERROR,
	     "Couldn't seek on file, graphics printout disabled");
    fclose(printer_graphics_file);
    printer_graphics_file=NULL;
    printer_graphics_enabled=0;
    }
  }

return(1);
}


static int printer_text_open_file(void)
{
if(!printer_text_enabled || !settings_current.printer_text_filename)
  return(0);

/* append to any existing file... */
if((printer_text_file=fopen(settings_current.printer_text_filename,"a"))==NULL)
  {
  ui_error(UI_ERROR_ERROR,"Couldn't open '%s', text printout disabled",
	   settings_current.printer_text_filename);
  printer_text_enabled=0;
  return(0);
  }

/* ensure users have immediate access to text file contents */
setbuf( printer_text_file, NULL );

return(1);
}


/* this is the output routine for the `proper' printers
 * (those connected via serial/parallel ports), and for
 * the text-parsing ZX Printer mode.
 */
static void printer_text_output_char(int c)
{
if(!printer_text_enabled)
  return;

if(!printer_text_file && !printer_text_open_file())
  return;

fputc(c,printer_text_file);
}


static void printer_zxp_update_header(void)
{
long pos;

if(!printer_graphics_enabled || !zxpheight) return;

if(!printer_graphics_file && !printer_zxp_open_file())
  return;

pos=ftell(printer_graphics_file);

/* seek back to write the image height */
if(fseek(printer_graphics_file,strlen("P4\n256 "),SEEK_SET)!=0)
  ui_error(UI_ERROR_ERROR,
	   "Couldn't seek to write graphics printout image height");
else
  {
  /* I originally had spaces after the image height, but that actually
   * breaks the format as defined in pbm(5) (not to mention breaking
   * when read by zgv :-)). So they're now before the height.
   */
  fprintf(printer_graphics_file,"%10d",zxpheight);
  }

if(fseek(printer_graphics_file,pos,SEEK_SET)!=0)
  {
  ui_error(UI_ERROR_ERROR,
	   "Couldn't re-seek on file, graphics printout disabled");
  fclose(printer_graphics_file);
  printer_graphics_file=NULL;
  printer_graphics_enabled=0;
  }
}


static void printer_zxp_end(void)
{
/* stop the printer */
printer_zxp_write(0xfb,4);

/* if not enabled or not opened, can't have written anything */
if(!printer_graphics_enabled || !printer_graphics_file || zxpheight==0)
  return;

/* write header */
printer_zxp_update_header();

fclose(printer_graphics_file);
printer_graphics_file=NULL;
printer_graphics_enabled=0;
}


static void printer_text_end(void)
{
if(!printer_text_enabled)
  return;

if(printer_text_file)
  {
  fclose(printer_text_file);
  printer_text_file=NULL;
  }
}


/* output the last line printed as text. */
static void printer_zxp_output_as_text(void)
{
static unsigned char charset[256*8];
static unsigned char outbuf[32];
unsigned char *ptr;
int x,y,f,c,chars;

#define SYSV_CHARS	0x5c36

chars=readbyte_internal(SYSV_CHARS);
chars+=256*readbyte_internal(SYSV_CHARS+1);

memset(charset,0,sizeof(charset));
ptr=charset+32*8;
for(f=32*8;f<128*8;f++)
  *ptr++=readbyte_internal(chars+f);

for(x=0;x<32;x++)
  {
  c=-1;
  
  /* try each char */
  for(f=32;f<128 && c==-1;f++)
    {
    ptr=zxplast8+x;
    c=f;
    for(y=0;y<8;y++,ptr+=32)
      if(*ptr!=charset[f*8+y])
        {
        c=-1;
        break;
        }
    }

  /* can't do UDGs, too unreliable */

  if(c==-1) c=32;
  
  outbuf[x]=c;
  }

for(f=31;f>=0 && outbuf[f]==32;f--)
  outbuf[f]=0;

for(f=0;f<32 && outbuf[f];f++)
  printer_text_output_char(outbuf[f]);
printer_text_output_char('\n');
}


/* output a pixel line */
static void printer_zxp_output_line(void)
{
unsigned char *ptr;
int i,j,d;

if(!printer_graphics_enabled) return;

if(!printer_graphics_file && !printer_zxp_open_file())
  return;

zxpheight++;
zxplineofchar++;

/* scroll up record of last char-line */
memmove(zxplast8,zxplast8+32,sizeof(zxplast8)-32);

ptr=zxplast8+sizeof(zxplast8)-32;
for(i=0;i<32;i++)
  {
  for(d=j=0;j<8;j++)
    {
    d<<=1;
    d|=(zxpline[i*8+j]?1:0);
    }
  
  *ptr++=d;
  
  fputc(d,printer_graphics_file);
  }

if(zxplineofchar>=8)
  {
  printer_zxp_output_as_text();
  zxplineofchar=0;
  }
}


/* currently just incrs a frame counter we need for ZX Printer.
 * can't fail, hence no return value.
 */
void printer_frame(void)
{
frames++;
}



/* ZX Printer support, transliterated from IMC's xz80 by a bear of
 * very little brain. :-) Or at least, I don't grok it that well.
 * It works wonderfully though.
 */
static libspectrum_byte printer_zxp_read(libspectrum_word port GCC_UNUSED,
				         libspectrum_byte *attached)
{
if(!settings_current.printer)
  return(0xff);
if(!printer_graphics_enabled)
  return(0xff);
if(plusd_available)
  return(0xff);

*attached = 0xff; /* TODO: check this */

if(!zxpspeed)
  return 0x3e;
else
  {
  int frame=frames-zxpframes;
  int cycles=tstates-zxpcycles;
  int pix=zxppixel;
  int sp=zxpnewspeed;
  int x,ans;
  int cpp=440/zxpspeed;
      
  if(frame>400)
    frame=400;
  cycles+=frame*machine_current->timings.tstates_per_frame;
  x=cycles/cpp-64;        /* x-coordinate reached */
      
  while(x>320)
    {           /* if we are on another line, */
    pix=-1;              /* find out where we are */
    x-=384;
    if(sp)
      {
      x=(x+64)*cpp;
      cpp=440/sp;
      x=x/cpp-64;
      sp=0;
      }
    }
  if((x>-10 && x<0) | zxpstylus)
    ans=0xbe;
  else
    ans=0x3e;
  if(x>pix)
    ans|=1;
  return ans;
  }
}


static void printer_zxp_write(libspectrum_word port GCC_UNUSED,
                              libspectrum_byte b)
{
if(!settings_current.printer)
  return;
if(plusd_available)
  return;
if(!zxpspeed)
  {
  if(!(b&4))
    {
    zxpspeed=(b&2)?1:2;
    zxpframes=frames;
    zxpcycles=tstates;
    zxpstylus=b&128;
    zxppixel=-1;
    zxplineofchar=0;
    }
  }
else
  {
  int frame=frames-zxpframes;
  int cycles=tstates-zxpcycles;
  int i,x;
  int cpp=440/zxpspeed;
      
  if(frame>400)
    frame=400; /* limit height of blank paper */
  cycles+=frame*machine_current->timings.tstates_per_frame;
  x=cycles/cpp-64;        /* x-coordinate reached */
  for(i=zxppixel;i<x && i<256;i++)
    if(i>=0)		/* should be, but just in case */
      zxpline[i]=zxpstylus;
  if(x>=256 && zxppixel<256)
    printer_zxp_output_line();
      
  while(x>=320)
    {          /* move to next line */
    zxpcycles+=cpp*384;
    if(zxpcycles>=machine_current->timings.tstates_per_frame)
      zxpcycles-=machine_current->timings.tstates_per_frame,zxpframes++;
    x-=384;
    if(zxpnewspeed)
      {
      zxpspeed=zxpnewspeed;
      zxpnewspeed=0;
      x=(x+64)*cpp;
      cpp=440/zxpspeed;
      x=x/cpp-64;
      }
    for(i=0;i<x && i<256;i++)
      zxpline[i]=zxpstylus;
    if(x>=256)
      printer_zxp_output_line();
    }
  if(x<0)
    x=-1;
  if(b&4)
    {
    if(x>=0 && x<256)
      {
      for(i=x;i<256;i++)
        zxpline[i]=zxpstylus;
      printer_zxp_output_line();
      }
    zxpspeed=zxpstylus=0;

    /* this marks the end of a char line or COPY */
    zxplineofchar=0;
    
    /* this is pretty frequent (on a per-char-line basis!),
     * but it's the only time we can really do it automagically.
     */
    printer_zxp_update_header();
    }
  else
    {
    zxppixel=x;
    zxpstylus=b&128;
    if(x<0)
      zxpspeed=(b&2)?1:2;
    else
      {
      zxpnewspeed=(b&2)?1:2;
      if(zxpnewspeed==zxpspeed)
        zxpnewspeed=0;
      }
    }
  } 
}


void printer_zxp_reset(int hard_reset GCC_UNUSED)
{
/* stop printer - XXX not sure if the real one does this on reset */
printer_zxp_write(0xfb,4);

zxplineofchar=0;
}


/* called when writing to the AY port which deals with RS232 output.
 * Assuming that the port gets written for every bit sent (which seems
 * likely; certainly the ROMs do it) means we can use a
 * bps-independent approach.
 */
void printer_serial_write(libspectrum_byte b)
{
static int reading=0,bits_to_get=0,ser_byte=0;
int high=(b&8);
if(!settings_current.printer)
  return;
if(!reading)
  {
  if(!high)
    {
    bits_to_get=9;
    reading=1;
    }
  }
else /* reading */
  {
  if(bits_to_get)
    {
    ser_byte>>=1;
    ser_byte|=(high?0x100:0);
    bits_to_get--;
    if(!bits_to_get)
      {
      if(ser_byte&0x100)	/* check stop bit is valid */
        printer_text_output_char(ser_byte&0xff);
      reading=0;
      }
    }
  }
}


/* The +2A/+3 ROM writes the strobe the wrong way, which doesn't work on
 * some printers. But some programs (notably Ian Collier's 48K Disk BASIC)
 * write it the *right* way. This means we can't treat either edge *alone*
 * as a signal to print, otherwise we'll print a junk byte when 48KDB
 * starts up. Realistically, we have to look for two close together.
 *
 * The Centronics spec seems to require that the strobe `goes low' for
 * a minimum of 1us to give the printer time to read the data. But that's
 * not even a NOP's worth on the Speccy, so we need to allow for longer;
 * 10000 cycles seems reasonable, being thousands of times longer. :-)
 * (And the longest delay I've seen was just over 5000 cycles.)
 */
void printer_parallel_strobe_write(int on)
{
static int old_on=0;
static int second_edge=0;
static unsigned int last_frames=0;
static libspectrum_dword last_tstates=0;
static unsigned char last_data=0;
libspectrum_dword diff;

if(!settings_current.printer)
  return;
if((old_on && !on) || (!old_on && on))
  {
  /* got an edge */

  if(!second_edge)
    {
    /* the ROM also seems to assume the printer is reading at the first
     * edge, breaking the spec (sigh). So we need to save current
     * data or we lose CRs in LPRINT/LLIST.
     */
    last_data=parallel_data;
    second_edge=1;
    }
  else
    {
    second_edge=0;
    diff=tstates;
    if(frames!=last_frames)
      diff+=machine_current->timings.tstates_per_frame;
    diff-=last_tstates;

    if(diff<=PARALLEL_STROBE_MAX_CYCLES)
      printer_text_output_char(last_data);
    else
      {
      /* too long ago, treat it as first edge */
      last_data=parallel_data;
      second_edge=1;
      }
    }

  last_frames=frames;
  last_tstates=tstates;
  }

old_on=on;
}


static libspectrum_byte printer_parallel_read(libspectrum_word port GCC_UNUSED,
				              libspectrum_byte *attached)
{
if(!settings_current.printer)
  return(0xff);

*attached = 0xff; /* TODO: check this */

/* bit 0 = busy. other bits high? */

return(0xfe);	/* never busy */
}


void printer_parallel_write(libspectrum_word port GCC_UNUSED,
			    libspectrum_byte b)
{
if(!settings_current.printer)
  return;
parallel_data=b;
}

static int
printer_init( void *context )
{
  printer_graphics_enabled=printer_text_enabled = 1;
  printer_graphics_file=printer_text_file = NULL;

  printer_zxp_init();
  printer_text_init();

  return 0;
}

static void
printer_end( void )
{
  printer_text_end();
  printer_zxp_end();
}

void
printer_register_startup( void )
{
  startup_manager_module dependencies[] = {
    STARTUP_MANAGER_MODULE_MACHINE,
    STARTUP_MANAGER_MODULE_SETUID,
  };
  startup_manager_register( STARTUP_MANAGER_MODULE_PRINTER, dependencies,
                            ARRAY_SIZE( dependencies ), printer_init, NULL,
                            printer_end );
}

static void zx_printer_snapshot_enabled( libspectrum_snap *snap )
{
  if( libspectrum_snap_zx_printer_active( snap ) )
    settings_current.zxprinter = 1;
}

static void zx_printer_to_snapshot( libspectrum_snap *snap )
{
  libspectrum_snap_set_zx_printer_active( snap, settings_current.zxprinter );
}
