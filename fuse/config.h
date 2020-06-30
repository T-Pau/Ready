/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Defined if we can increase Blip Buffer accuracy */
/* #undef BLIP_BUFFER_ACCURACY */

/* Define to 1 if SpeccyBoot is supported. */
/* #undef BUILD_SPECCYBOOT */

/* Defined if we support spectranet */
/* #undef BUILD_SPECTRANET */

/* DirectX 7 or higher is required */
/* #undef DIRECTSOUND_VERSION */

/* Define copyright of Fuse */
#define FUSE_COPYRIGHT "(c) 1999-2018 Philip Kendall and others"

/* Define version information for win32 executables */
#define FUSE_RC_VERSION 1,5,7,0

/* Define to 1 if you have the `dirname' function. */
#define HAVE_DIRNAME 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Defined if we've got enough memory to compile z80_ops.c */
#define HAVE_ENOUGH_MEMORY 1

/* Define to 1 if you have the `fsync' function. */
#define HAVE_FSYNC 1

/* Define to 1 if you have the `geteuid' function. */
#define HAVE_GETEUID 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Defined if gpm in use */
/* #undef HAVE_GPM_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <jsw.h> header file. */
/* #undef HAVE_JSW_H */

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Defined if we've got GLib */
/* #undef HAVE_LIB_GLIB */

/* Defined if we've got libxml2 */
#define HAVE_LIB_XML2 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Have PTHREAD_PRIO_INHERIT. */
/* #undef HAVE_PTHREAD_PRIO_INHERIT */

/* Define to 1 if you have the <siginfo.h> header file. */
/* #undef HAVE_SIGINFO_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if your system has strcasecmp() in string.h */
#define HAVE_STRING_STRCASECMP 1

/* Define to 1 if you have the <sys/audioio.h> header file. */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define to 1 if you have the <sys/audio.h> header file. */
/* #undef HAVE_SYS_AUDIO_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <X11/extensions/XShm.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XSHM_H */

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Defined if no sound code is present */
/* #undef NO_SOUND */

/* Name of package */
#define PACKAGE "fuse"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://sourceforge.net/p/fuse-emulator/bugs/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "fuse"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "fuse 1.5.7"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "fuse"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://fuse-emulator.sourceforge.net/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.5.7"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Location of the ROM images */
/* #undef ROMSDIR */

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 0

/* Defined if the sound code uses a fifo */
#define SOUND_FIFO 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Defined if framebuffer UI in use */
/* #undef UI_FB */

/* Defined if GTK+ UI is in use */
/* #undef UI_GTK */

/* Defined if null UI is in use */
/* #undef UI_NULL */

/* Defined if the SDL UI in use */
/* #undef UI_SDL */

/* Defined if svgalib UI in use */
/* #undef UI_SVGA */

/* Defined if Wii UI in use */
/* #undef UI_WII */

/* Defined if Win32 UI in use */
/* #undef UI_WIN32 */

/* Defined if Xlib UI in use */
#define UI_X 1

/* Defined if we're using hardware joysticks */
/* #undef USE_JOYSTICK */

/* Defined if we're going to be using the installed libpng */
/* #undef USE_LIBPNG */

/* Defined if we're using a widget-based UI */
/* #undef USE_WIDGET */

/* Version number of package */
#define VERSION "1.5.7"

/* Exclude rarely used stuff from Windows headers <windows.h> */
/* #undef WIN32_LEAN_AND_MEAN */

/* Minimal supported version of Windows is 2000 */
/* #undef WINVER */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Internet Explorer 5.01 or higher is required */
/* #undef _WIN32_IE */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
