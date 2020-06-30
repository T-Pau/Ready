/* make-perl.c: Generate a perl script to create the libspectrum_* typedefs
   Copyright (c) 2002-2003,2015 Philip Kendall, Darren Salt

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

#include <stdio.h>

int main(void)
{
  /*
   * Define the integer types
   */

  printf( "if( /LIBSPECTRUM_DEFINE_TYPES/ ) {\n\n  $_ = << \"CODE\";\n" );

#ifdef HAVE_STDINT_H

  printf( "#include <stdint.h>\n\n" );

  printf( "typedef  uint8_t libspectrum_byte;\n" );
  printf( "typedef   int8_t libspectrum_signed_byte;\n" );

  printf( "typedef uint16_t libspectrum_word;\n" );
  printf( "typedef  int16_t libspectrum_signed_word;\n" );

  printf( "typedef uint32_t libspectrum_dword;\n" );
  printf( "typedef  int32_t libspectrum_signed_dword;\n" );

  printf( "typedef uint64_t libspectrum_qword;\n" );
  printf( "typedef  int64_t libspectrum_signed_qword;\n" );

#else				/* #ifdef HAVE_STDINT_H */

  if( sizeof( char ) == 1 ) {
    printf( "typedef unsigned char libspectrum_byte;\n" );
    printf( "typedef   signed char libspectrum_signed_byte;\n" );
  } else if( sizeof( short ) == 1 ) {
    printf( "typedef unsigned short libspectrum_byte;\n" );
    printf( "typedef   signed short libspectrum_signed_byte;\n" );
  } else {
    fprintf( stderr, "No plausible 8 bit types found\n" );
    return 1;
  }

  if( sizeof( short ) == 2 ) {
    printf( "typedef unsigned short libspectrum_word;\n" );
    printf( "typedef   signed short libspectrum_signed_word;\n" );
  } else if( sizeof( int ) == 2 ) {
    printf( "typedef unsigned int libspectrum_word;\n" );
    printf( "typedef   signed int libspectrum_signed_word;\n" );
  } else {
    fprintf( stderr, "No plausible 16 bit types found\n" );
    return 1;
  }

  if( sizeof( int ) == 4 ) {
    printf( "typedef unsigned int libspectrum_dword;\n" );
    printf( "typedef   signed int libspectrum_signed_dword;\n" );
  } else if( sizeof( long ) == 4 ) {
    printf( "typedef unsigned long libspectrum_dword;\n" );
    printf( "typedef   signed long libspectrum_signed_dword;\n" );
  } else {
    fprintf( stderr, "No plausible 32 bit types found\n" );
    return 1;
  }

  if( sizeof( long ) == 8 ) {
    printf( "typedef unsigned long libspectrum_qword;\n" );
    printf( "typedef   signed long libspectrum_signed_qword;\n" );
#if defined(_MSC_VER) && _MSC_VER <= 1200
  } else {
    printf( "typedef unsigned __int64 libspectrum_qword;\n" );
    printf( "typedef   signed __int64 libspectrum_signed_qword;\n" );
#else
  } else if( sizeof( long long ) == 8 ) {
    printf( "typedef unsigned long long libspectrum_qword;\n" );
    printf( "typedef   signed long long libspectrum_signed_qword;\n" );
  } else {
    fprintf( stderr, "No plausible 64 bit types found\n" );
    return 1;
#endif
  }

#endif				/* #ifdef HAVE_STDINT_H */

  printf( "CODE\n}\n\n" );

  /*
   * Get glib or our replacement for it
   */
   
  printf( "if( /LIBSPECTRUM_GLIB_REPLACEMENT/ ) {\n\n" );

#ifdef HAVE_LIB_GLIB		/* #ifdef HAVE_LIB_GLIB */

  printf( "  $_ = \"#define LIBSPECTRUM_HAS_GLIB_REPLACEMENT 0\\n\"\n" );

#else				/* #ifdef HAVE_LIB_GLIB */

  printf( "  $_ = << \"CODE\";\n"
"#define LIBSPECTRUM_HAS_GLIB_REPLACEMENT 1\n"
"\n"
"#ifndef	FALSE\n"
"#define	FALSE	(0)\n"
"#endif\n"
"\n"
"#ifndef	TRUE\n"
"#define	TRUE	(!FALSE)\n"
"#endif\n"
"\n"
"typedef char gchar;\n"
"typedef int gint;\n"
"typedef long glong;\n"
"typedef gint gboolean;\n"
"typedef unsigned int guint;\n"
"typedef unsigned long gulong;\n"
"typedef const void * gconstpointer;\n"
"typedef void * gpointer;\n"
"\n"
"typedef struct _GSList GSList;\n"
"\n"
"struct _GSList {\n"
"    gpointer data;\n"
"    GSList *next;\n"
"};\n"
"\n"
"typedef void		(*GFunc)		(gpointer	data,\n"
"						 gpointer	user_data);\n"
"\n"
"typedef gint		(*GCompareFunc)		(gconstpointer	a,\n"
"						 gconstpointer	b);\n"
"\n"
"typedef void           (*GDestroyNotify)       (gpointer       data);\n"
"\n"
"typedef void           (*GFreeFunc)            (gpointer       data);\n"
"\n"
"\n"
"WIN32_DLL GSList *g_slist_insert_sorted	(GSList		*list,\n"
"						 gpointer	 data,\n"
"						 GCompareFunc	 func);\n"
"\n"
"WIN32_DLL GSList *g_slist_insert		(GSList		*list,\n"
"			 			 gpointer	 data,\n"
"			 			 gint		 position);\n"
"\n"
"WIN32_DLL GSList *g_slist_append		(GSList		*list,\n"
"						 gpointer	 data);\n"
"\n"
"WIN32_DLL GSList *g_slist_prepend		(GSList		*list,\n"
"						 gpointer	 data);\n"
"\n"
"WIN32_DLL GSList *g_slist_remove		(GSList		*list,\n"
"						 gconstpointer	 data);\n"
"\n"
"WIN32_DLL GSList *g_slist_last			(GSList		*list);\n"
"\n"
"WIN32_DLL GSList *g_slist_reverse		(GSList		*list);\n"
"\n"
"WIN32_DLL GSList *g_slist_delete_link		(GSList		*list,\n"
"				 		 GSList		*link);\n"
"\n"
"WIN32_DLL guint	g_slist_length		(GSList *list);\n"
"\n"
"WIN32_DLL void	g_slist_foreach			(GSList		*list,\n"
"						 GFunc		 func,\n"
"						 gpointer	 user_data);\n"
"\n"
"WIN32_DLL void	g_slist_free			(GSList		*list);\n"
"\n"
"WIN32_DLL GSList *g_slist_nth		(GSList		*list,\n"
"					 guint		n);\n"
"\n"
"WIN32_DLL GSList *g_slist_find_custom	(GSList		*list,\n"
"					 gconstpointer	data,\n"
"					 GCompareFunc	func );\n"
"\n"
"WIN32_DLL gint	g_slist_position	(GSList		*list,\n"
"					 GSList		*llink);\n"
"\n"
"typedef struct _GHashTable	GHashTable;\n"
"\n"
"typedef guint		(*GHashFunc)		(gconstpointer	key);\n"
"\n"
"typedef void	(*GHFunc)		(gpointer	key,\n"
"						 gpointer	value,\n"
"						 gpointer	user_data);\n"
"\n"
"typedef gboolean	(*GHRFunc)		(gpointer	key,\n"
"						 gpointer	value,\n"
"						 gpointer	user_data);\n"
"\n"
"WIN32_DLL gint	g_int_equal (gconstpointer   v,\n"
"			       gconstpointer   v2);\n"
"WIN32_DLL guint	g_int_hash  (gconstpointer   v);\n"
"\n"
"WIN32_DLL gint	g_str_equal (gconstpointer   v,\n"
"			       gconstpointer   v2);\n"
"WIN32_DLL guint	g_str_hash  (gconstpointer   v);\n"
"\n"
"WIN32_DLL GHashTable *g_hash_table_new	(GHashFunc	 hash_func,\n"
"					 GCompareFunc	 key_compare_func);\n"
"\n"
"WIN32_DLL GHashTable *g_hash_table_new_full (GHashFunc       hash_func,\n"
"                                             GCompareFunc    key_equal_func,\n"
"                                             GDestroyNotify  key_destroy_func,\n"
"                                             GDestroyNotify  value_destroy_func);\n"
"\n"
"WIN32_DLL void	g_hash_table_destroy	(GHashTable	*hash_table);\n"
"\n"
"WIN32_DLL void	g_hash_table_insert	(GHashTable	*hash_table,\n"
"					 gpointer	 key,\n"
"					 gpointer	 value);\n"
"\n"
"WIN32_DLL gpointer g_hash_table_lookup	(GHashTable	*hash_table,\n"
"					 gconstpointer	 key);\n"
"\n"
"WIN32_DLL void	g_hash_table_foreach (GHashTable	*hash_table,\n"
"						 GHFunc    func,\n"
"						 gpointer  user_data);\n"
"\n"
"WIN32_DLL guint	g_hash_table_foreach_remove	(GHashTable	*hash_table,\n"
"						 GHRFunc	 func,\n"
"						 gpointer	 user_data);\n"
"\n"
"WIN32_DLL guint	g_hash_table_size (GHashTable	*hash_table);\n"
"\n"
"typedef struct _GArray GArray;\n"
"\n"
"struct _GArray {\n"
"  /* Public */\n"
"  gchar *data;\n"
"  size_t len;\n"
"\n"
"  /* Private */\n"
"  guint element_size;\n"
"  size_t allocated;\n"
"};\n"
"\n"
"WIN32_DLL GArray* g_array_new( gboolean zero_terminated, gboolean clear,\n"
"		      guint element_size );\n"
"WIN32_DLL GArray* g_array_sized_new( gboolean zero_terminated, gboolean clear,\n"
"                   guint element_size, guint reserved_size );\n"
"#define g_array_append_val(a,v) g_array_append_vals( a, &(v), 1 );\n"
"WIN32_DLL GArray* g_array_append_vals( GArray *array, gconstpointer data, guint len );\n"
"#define g_array_index(a,t,i) (*(((t*)a->data)+i))\n"
"WIN32_DLL GArray* g_array_set_size( GArray *array, guint length );\n"
"WIN32_DLL GArray* g_array_remove_index_fast( GArray *array, guint index );\n"
"WIN32_DLL gchar* g_array_free( GArray *array, gboolean free_segment );\n"
"\n" );
  if( sizeof( void* ) == sizeof( int ) ) {
    printf( "#define GINT_TO_POINTER(i)      ((gpointer)  (i))\n" );
    printf( "#define GPOINTER_TO_INT(p)      ((gint)   (p))\n" );
    printf( "#define GPOINTER_TO_UINT(p)     ((guint)  (p))\n" );
  } else if( sizeof( void* ) == sizeof( long ) ) {
    printf( "#define GINT_TO_POINTER(i)      ((gpointer)  (glong)(i))\n" );
    printf( "#define GPOINTER_TO_INT(p)      ((gint)   (glong)(p))\n" );
    printf( "#define GPOINTER_TO_UINT(p)     ((guint)  (gulong)(p))\n" );
  } else {
    fprintf( stderr, "No plausible int to pointer cast found\n" );
    return 1;
  }
  printf( "CODE\n" );
#endif				/* #ifdef HAVE_LIB_GLIB */

  printf( "}\n\n" );

  /*
   * If we have libgcrypt, include the header file and store the signature
   * parameters
   */

  printf( "if( /LIBSPECTRUM_INCLUDE_GCRYPT/ ) {\n\n" );

#ifdef HAVE_GCRYPT_H

  printf( "  $_ = \"#include <gcrypt.h>\\n\";\n\n" );

#else

  printf( "  $_ = '';\n\n" );

#endif

  printf( "}\n\n" );

  printf( "if( /LIBSPECTRUM_SIGNATURE_PARAMETERS/ ) {\n\n" );

#ifdef HAVE_GCRYPT_H

  printf( "  $_ = \"  /* The DSA signature parameters 'r' and 's' */\\n  gcry_mpi_t r, s;\\n\";\n\n" );

#else

  printf( "  $_ = \"/* Signature parameters not stored as libgcrypt is not present */\n\";\n\n" );

#endif

  printf( "}\n\n" );

  printf( "if( /LIBSPECTRUM_CAPABILITIES/ ) {\n\n  $_ = << \"CODE\";\n" );

#ifdef HAVE_ZLIB_H

  printf( "\n/* we support snapshots etc. requiring zlib (e.g. compressed szx) */\n" );
  printf( "#define	LIBSPECTRUM_SUPPORTS_ZLIB_COMPRESSION	(1)\n\n" );
  printf( "/* zlib (de)compression routines */\n\n" );
  printf( "WIN32_DLL libspectrum_error\n" );
  printf( "libspectrum_zlib_inflate( const libspectrum_byte *gzptr, size_t gzlength,\n" );
  printf( "			  libspectrum_byte **outptr, size_t *outlength );\n\n" );
  printf( "WIN32_DLL libspectrum_error\n" );
  printf( "libspectrum_zlib_compress( const libspectrum_byte *data, size_t length,\n" );
  printf( "			   libspectrum_byte **gzptr, size_t *gzlength );\n\n" );

#endif				/* #ifdef HAVE_ZLIB_H */

#ifdef HAVE_LIBBZ2
  printf( "\n/* we support files compressed with bz2 */\n" );
  printf( "#define	LIBSPECTRUM_SUPPORTS_BZ2_COMPRESSION	(1)\n\n" );
#endif				/* #ifdef HAVE_LIBBZ2 */

#ifdef HAVE_LIB_AUDIOFILE
  printf( "\n/* we support wav files */\n" );
  printf( "#define	LIBSPECTRUM_SUPPORTS_AUDIOFILE	(1)\n\n" );
#endif				/* #ifdef HAVE_LIB_AUDIOFILE */

  printf( "CODE\n}\n\n" );

  return 0;
}
