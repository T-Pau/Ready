/* crypto.c: crytography-related functions
   Copyright (c) 2002-2015 Philip Kendall

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

#ifdef HAVE_GCRYPT_H

#include <gcrypt.h>

#include "internals.h"

static const char * const private_key_format =
  "(key-data (private-key (dsa (p %m) (q %m) (g %m) (y %m) (x %m))))";
static const char * const public_key_format =
  "(key-data (public-key (dsa (p %m) (q %m) (g %m) (y %m))))";
static const char * const hash_format = "(data (flags raw) (value %m))";
static const char * const signature_format = "(sig-val (dsa (r %m) (s %m)))";

#define HASH_ALGORITHM GCRY_MD_SHA1
#define MPI_COUNT 5

static libspectrum_error
get_signature( gcry_mpi_t *r, gcry_mpi_t *s, libspectrum_byte *data,
	       size_t data_length, libspectrum_rzx_dsa_key *key );
static libspectrum_error
get_hash( gcry_sexp_t *hash, const libspectrum_byte *data,
	  size_t data_length );
static libspectrum_error
create_key( gcry_sexp_t *s_key, libspectrum_rzx_dsa_key *key, int secret_key);
static void free_mpis( gcry_mpi_t *mpis, size_t n );
static libspectrum_error get_mpi( gcry_mpi_t *mpi, gcry_sexp_t signature,
				  const char *token );
static libspectrum_error
serialise_mpis( libspectrum_byte **signature, size_t *signature_length,
		gcry_mpi_t r, gcry_mpi_t s );

libspectrum_error
libspectrum_sign_data( libspectrum_byte **signature, size_t *signature_length,
		       libspectrum_byte *data, size_t data_length,
		       libspectrum_rzx_dsa_key *key )
{
  int error;
  gcry_mpi_t r, s;

  error = get_signature( &r, &s, data, data_length, key );
  if( error ) return error;

  error = serialise_mpis( signature, signature_length, r, s );
  if( error ) { gcry_mpi_release( r ); gcry_mpi_release( s ); return error; }

  gcry_mpi_release( r ); gcry_mpi_release( s );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_signature( gcry_mpi_t *r, gcry_mpi_t *s, libspectrum_byte *data,
	       size_t data_length, libspectrum_rzx_dsa_key *key )
{
  libspectrum_error error;
  gcry_error_t gcrypt_error;
  gcry_sexp_t hash, s_key, s_signature;

  error = get_hash( &hash, data, data_length ); if( error ) return error;

  error = create_key( &s_key, key, 1 );
  if( error ) { gcry_sexp_release( hash ); return error; }

  gcrypt_error = gcry_pk_sign( &s_signature, hash, s_key );
  if( gcrypt_error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_signature: error signing data: %s",
			     gcry_strerror( gcrypt_error ) );
    gcry_sexp_release( s_key ); gcry_sexp_release( hash );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  gcry_sexp_release( s_key ); gcry_sexp_release( hash );

  error = get_mpi( r, s_signature, "r" );
  if( error ) { gcry_sexp_release( s_signature ); return error; }
  error = get_mpi( s, s_signature, "s" );
  if( error ) {
    gcry_sexp_release( s_signature ); gcry_mpi_release( *r );
    return error;
  }

  gcry_sexp_release( s_signature );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
get_hash( gcry_sexp_t *hash, const libspectrum_byte *data, size_t data_length )
{
  gcry_error_t error;
  unsigned char *digest; size_t digest_length;
  gcry_mpi_t hash_mpi;
  
  digest_length = gcry_md_get_algo_dlen( HASH_ALGORITHM );
  digest = libspectrum_new( unsigned char, digest_length );

  gcry_md_hash_buffer( HASH_ALGORITHM, digest, data, data_length );

  error = gcry_mpi_scan( &hash_mpi, GCRYMPI_FMT_USG, digest, digest_length,
			 NULL );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_hash: error creating hash MPI: %s",
			     gcry_strerror( error )
    );
    libspectrum_free( digest );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  libspectrum_free( digest );

  error = gcry_sexp_build( hash, NULL, hash_format, hash_mpi );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_hash: error creating hash sexp: %s",
			     gcry_strerror( error )
    );
    gcry_mpi_release( hash_mpi );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  gcry_mpi_release( hash_mpi );

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
create_key( gcry_sexp_t *s_key, libspectrum_rzx_dsa_key *key,
	    int secret_key )
{
  gcry_error_t error;
  size_t i;
  gcry_mpi_t mpis[MPI_COUNT];
  const char *format;

  for( i=0; i<MPI_COUNT; i++ ) mpis[i] = NULL;

    error = gcry_mpi_scan( &mpis[0], GCRYMPI_FMT_HEX, (unsigned char*)key->p,
			   0, NULL );
  if( !error ) 
    error = gcry_mpi_scan( &mpis[1], GCRYMPI_FMT_HEX, (unsigned char*)key->q,
			   0, NULL );
  if( !error )
    error = gcry_mpi_scan( &mpis[2], GCRYMPI_FMT_HEX, (unsigned char*)key->g,
			   0, NULL );
  if( !error )
    error = gcry_mpi_scan( &mpis[3], GCRYMPI_FMT_HEX, (unsigned char*)key->y,
			   0, NULL );
  if( !error && secret_key )
    error = gcry_mpi_scan( &mpis[4], GCRYMPI_FMT_HEX, (unsigned char*)key->x,
			   0, NULL );

  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "create_key: error creating MPIs: %s",
			     gcry_strerror( error ) );
    free_mpis( mpis, MPI_COUNT );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  format = secret_key ? private_key_format : public_key_format;
  
  error = gcry_sexp_build( s_key, NULL, format,
			   mpis[0], mpis[1], mpis[2], mpis[3], mpis[4] );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "create_key: error creating key: %s",
			     gcry_strerror( error ) );
    free_mpis( mpis, MPI_COUNT );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  free_mpis( mpis, MPI_COUNT );

  /* FIXME: Test public keys as well once gcry_pk_testkey acquires this
     functionality */
  if( secret_key ) {
    error = gcry_pk_testkey( *s_key );
    if( error ) {
      libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			       "create_key: key is not sane: %s",
			       gcry_strerror( error ) );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static void
free_mpis( gcry_mpi_t *mpis, size_t n )
{
  size_t i;
  for( i=0; i<n; i++ ) if( mpis[i] ) gcry_mpi_release( mpis[i] );
}

static libspectrum_error
get_mpi( gcry_mpi_t *mpi, gcry_sexp_t sexp, const char *token )
{
  gcry_sexp_t pair;

  pair = gcry_sexp_find_token( sexp, token, strlen( token ) );
  if( !pair ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_mpis: couldn't find '%s'", token );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  *mpi = gcry_sexp_nth_mpi( pair, 1, GCRYMPI_FMT_STD );
  if( !(*mpi) ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "get_mpis: couldn't create MPI '%s'", token );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

static libspectrum_error
serialise_mpis( libspectrum_byte **signature, size_t *signature_length,
		gcry_mpi_t r, gcry_mpi_t s )
{
  gcry_error_t error;
  size_t length, length_s;
  unsigned char *ptr;

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, NULL, 0, &length, r );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: length of r: %s",
			     gcry_strerror( error ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, NULL, 0, &length_s, s );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: length of s: %s",
			     gcry_strerror( error ) );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  length += length_s; *signature_length = length;

  *signature = libspectrum_new( libspectrum_byte, length );

  error = gcry_mpi_print( GCRYMPI_FMT_PGP, *signature, length, &length, r );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: printing r: %s",
			     gcry_strerror( error ) );
    libspectrum_free( *signature );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  ptr = *signature + length; length = *signature_length - length;
  error = gcry_mpi_print( GCRYMPI_FMT_PGP, ptr, length, NULL, s );
  if( error ) {
    libspectrum_print_error( LIBSPECTRUM_ERROR_LOGIC,
			     "serialise_mpis: printing s: %s",
			     gcry_strerror( error ) );
    libspectrum_free( *signature );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  return LIBSPECTRUM_ERROR_NONE;
}

#include <stdio.h>

libspectrum_error
libspectrum_verify_signature( libspectrum_signature *signature,
			      libspectrum_rzx_dsa_key *key )
{
  libspectrum_error error;
  gcry_error_t gcrypt_error;
  gcry_sexp_t hash, key_sexp, signature_sexp;

  error = get_hash( &hash, signature->start, signature->length );
  if( error ) return error;

  error = create_key( &key_sexp, key, 0 );
  if( error ) { gcry_sexp_release( hash ); return error; }

  error = gcry_sexp_build( &signature_sexp, NULL, signature_format,
			   signature->r, signature->s );

  if( error ) {
    libspectrum_print_error(
      LIBSPECTRUM_ERROR_LOGIC,
      "create_signature: error building signature sexp: %s",
      gcry_strerror( error )
    );
    gcry_sexp_release( key_sexp ); gcry_sexp_release( hash );
    return LIBSPECTRUM_ERROR_LOGIC;
  }

  gcrypt_error = gcry_pk_verify( signature_sexp, hash, key_sexp );

  gcry_sexp_release( signature_sexp );
  gcry_sexp_release( key_sexp ); gcry_sexp_release( hash );

  if( gcrypt_error ) {
    if( gcry_err_code( gcrypt_error ) == GPG_ERR_BAD_SIGNATURE ) {
      return LIBSPECTRUM_ERROR_SIGNATURE;
    } else {
      libspectrum_print_error(
        LIBSPECTRUM_ERROR_LOGIC,
	"libgcrypt error verifying signature: %s",
	gcry_strerror( gcrypt_error )
      );
      return LIBSPECTRUM_ERROR_LOGIC;
    }
  }

  return LIBSPECTRUM_ERROR_NONE;
}

libspectrum_error
libspectrum_signature_free( libspectrum_signature *signature )
{
  gcry_mpi_release( signature->r ); gcry_mpi_release( signature->s );

  return LIBSPECTRUM_ERROR_NONE;
}

#endif				/* #ifdef HAVE_GCRYPT_H */
