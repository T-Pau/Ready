#ifndef TEST_TEST_H
#define TEST_TEST_H

#include "libspectrum.h"

extern const char *progname;

typedef enum test_return_t {
  TEST_PASS,
  TEST_FAIL,
  TEST_INCOMPLETE,
} test_return_t;

typedef struct test_edge_sequence_t {

  libspectrum_dword length;
  size_t count;
  int flags;

} test_edge_sequence_t;

#define STATIC_TEST_PATH(x) SRCDIR "/test/" x
#define DYNAMIC_TEST_PATH(x) "test/" x

int read_file( libspectrum_byte **buffer, size_t *length,
	       const char *filename );

test_return_t check_edges( const char *filename, test_edge_sequence_t *edges,
			   int flags_mask );

test_return_t test_15( void );
test_return_t test_28( void );
test_return_t test_29( void );

/* SZX write tests */
test_return_t test_31( void );
test_return_t test_32( void );
test_return_t test_33( void );
test_return_t test_34( void );
test_return_t test_35( void );
test_return_t test_36( void );
test_return_t test_37( void );
test_return_t test_38( void );
test_return_t test_39( void );
test_return_t test_40( void );
test_return_t test_41( void );
test_return_t test_42( void );
test_return_t test_43( void );
test_return_t test_57( void );
test_return_t test_59( void );
test_return_t test_61( void );
test_return_t test_62( void );
test_return_t test_65( void );
test_return_t test_66( void );
test_return_t test_67( void );

/* SZX read tests */
test_return_t test_44( void );
test_return_t test_45( void );
test_return_t test_46( void );
test_return_t test_47( void );
test_return_t test_48( void );
test_return_t test_49( void );
test_return_t test_50( void );
test_return_t test_51( void );
test_return_t test_52( void );
test_return_t test_53( void );
test_return_t test_54( void );
test_return_t test_55( void );
test_return_t test_56( void );
test_return_t test_58( void );
test_return_t test_60( void );
test_return_t test_63( void );
test_return_t test_64( void );
test_return_t test_68( void );
test_return_t test_69( void );
test_return_t test_70( void );

#endif
