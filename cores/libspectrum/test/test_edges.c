#include "test.h"

static test_edge_sequence_t
complete_edges_list[] = 
{
  /* Standard speed data block */
  { 2168, 3223, 0 },	/* Pilot */
  {  667,    1, 0 },	/* Sync 1 */
  {  735,    1, 0 },	/* Sync 2 */

  { 1710,    2, 0 },	/* Bit 1 */
  {  855,    2, 0 },	/* Bit 2 */
  { 1710,    2, 0 },	/* Bit 3 */
  {  855,    2, 0 },	/* Bit 4 */
  { 1710,    2, 0 },	/* Bit 5 */
  {  855,    2, 0 },	/* Bit 6 */
  { 1710,    2, 0 },	/* Bit 7 */
  {  855,    2, 0 },	/* Bit 8 */

  { 8207500, 1, 0 },	/* Pause */

  /* Turbo speed data block */
  { 1000,    5, 0 },	/* Pilot */
  {  123,    1, 0 },	/* Sync 1 */
  {  456,    1, 0 },	/* Sync 2 */

  {  789,   16, 0 },	/* Byte 1, bits 1-8 */
  {  400,   16, 0 },	/* Byte 2, bits 1-8 */

  {  789,    2, 0 },	/* Byte 3, bit 1 */
  {  400,    2, 0 },	/* Byte 3, bit 2 */
  {  789,    2, 0 },	/* Byte 3, bit 3 */
  {  400,    2, 0 },	/* Byte 3, bit 4 */
  {  789,    2, 0 },	/* Byte 3, bit 5 */
  {  400,    2, 0 },	/* Byte 3, bit 6 */
  {  789,    2, 0 },	/* Byte 3, bit 7 */
  {  400,    2, 0 },	/* Byte 3, bit 8 */

  {  400,    2, 0 },	/* Byte 4, bit 1 */
  {  789,    2, 0 },	/* Byte 4, bit 2 */
  {  400,    2, 0 },	/* Byte 4, bit 3 */
  {  789,    2, 0 },	/* Byte 4, bit 4 */

  { 3454500, 1, 0 },	/* Pause */

  /* Pure tone block */
  {  535,  666, 0 },

  /* List of pulses */
  {  772,    1, 0 },
  {  297,    1, 0 },
  {  692,    1, 0 },

  /* Pure data block */
  { 1639,   16, 0 },	/* Byte 1, bits 1-8 */
  {  552,   16, 0 },	/* Byte 2, bits 1-8 */
  { 1639,   12, 0 },	/* Byte 3, bits 1-6 */
  { 1939000, 1, 0 },	/* Pause */

  /* Pause block */
  { 2163000, 1, 0 },

  /* Group start block */
  {    0,    1, 0 },

  /* Group end block */
  {    0,    1, 0 },

  /* Jump block */
  {    0,    1, 0 },

  /* Pure tone block skipped */

  /* Loop start block */
  {    0,    1, 0 },

    /* Iteration 1 */
    {  837,  185, 0 },	/* Pure tone block */
    {    0,    1, 0 },	/* Loop end block */

    /* Iteration 2 */
    {  837,  185, 0 },	/* Pure tone block */
    {    0,    1, 0 },	/* Loop end block */

    /* Iteration 3 */
    {  837,  185, 0 },	/* Pure tone block */
    {    0,    1, 0 },	/* Loop end block */

  /* Stop tape if in 48K mode block */
  {    0,    1, 4 },

  /* Text description block */
  {    0,    1, 0 },

  /* Message block */
  {    0,    1, 0 },

  /* Archive info block */
  {    0,    1, 0 },

  /* Hardware info block */
  {    0,    1, 0 },

  /* Custom info block */
  {    0,    1, 0 },

  /* Pure tone block */
  {  820,  940, 0 },
  {  820,    1, 2 },

  { -1, 0, 0 }		/* End marker */

};

test_return_t
test_15( void )
{
  return check_edges( DYNAMIC_TEST_PATH( "complete-tzx.tzx" ), complete_edges_list,
		      LIBSPECTRUM_TAPE_FLAGS_STOP | LIBSPECTRUM_TAPE_FLAGS_STOP48 );
}

static test_edge_sequence_t
zero_tail_edges_list[] = 
{
  /* Data block with 0 tail */
  {  855,   1,  96 },	/* Data short pulse 1 high */
  {  855,   1,  80 },	/* Data short pulse 2 low */
  {    0,   1,   9 },	/* No edge for tail, end of block */

  /* Pulse block */
  {  200,   1,  32 },	/* Following pulse 1 high */
  {  200,   1, 275 },	/* Following pulse 2 low, end of block, end of tape,
                           stop the tape */

  { -1, 0, 0 }		/* End marker */

};

test_return_t
test_28( void )
{
  return check_edges( DYNAMIC_TEST_PATH( "zero-tail.pzx" ),
                      zero_tail_edges_list, 0x1ff );
}

static test_edge_sequence_t
no_pilot_gdb_list[] = 
{
  /* Set signal level block */
  {    0,   1,  17 },	/* Set signal level low, end of block */

  /* GDB with 0 tail */
  {  771,   1,   0 },	/* Byte 1, bit 1, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 1, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 2, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 2, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 3, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 3, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 4, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 4, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 5, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 5, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 6, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 6, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 7, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 7, pulse 2 */
  {  771,   1,   0 },	/* Byte 1, bit 8, pulse 1 */
  { 1542,   1,   0 },	/* Byte 1, bit 8, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 1, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 1, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 2, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 2, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 3, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 3, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 4, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 4, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 5, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 5, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 6, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 6, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 7, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 7, pulse 2 */
  {  771,   1,   0 },	/* Byte 2, bit 8, pulse 1 */
  { 1542,   1,   0 },	/* Byte 2, bit 8, pulse 2 */
  {    0,   1, 259 },	/* End of block, end of tape, stop the tape (normally no
                           edge but not at end of tape) */

  { -1, 0, 0 }		/* End marker */

};

test_return_t
test_29( void )
{
  return check_edges( DYNAMIC_TEST_PATH( "no-pilot-gdb.tzx" ),
                      no_pilot_gdb_list, 0x1ff );
}
