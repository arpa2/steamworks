/* bitset.h -- Handle bitsets of a predefined size and define mathops on them.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef BITSET_H
#define BITSET_H


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#include "types.h"


typedef struct bitset {
	anynum_t maxbit;
	uint32_t *bits;
	type_t *type;
} bitset_t;


typedef struct bitset_iter {
	anynum_t maxbit;
	anynum_t curbit;
	anynum_t curint;
	uint32_t     curmsk;
	bitset_t    *curset;
} bitset_iter_t;


/* Management operations, creating a new bitset, destroying a bitset,
 * optimisations by setting a minimum size, emptying a bitset, and
 * copying and cloning a bitset.
 *
 * The elt_type provided to bitset_new is useful for extraction of
 * values from bit numbers, so add it to be able to get to the data
 * described in the bitsets.
 */
bitset_t *bitset_new (type_t *elt_type);
void bitset_destroy (bitset_t *bts);
type_t *bitset_type (bitset_t *bts);
void bitset_require_maxbit (bitset_t *bts, anynum_t reqsz);
void bitset_empty (bitset_t *bts);
void bitset_copy (bitset_t *dest, bitset_t *orig);
bitset_t *bitset_clone (bitset_t *bts);

/* Output routines, in this case printing bit numbers within a line */
void bitset_print (bitset_t *bts, char *opt_pfix, FILE *stream);
#ifdef DEBUG
#  define bitset_debug(bts,opt_pfix,stream) bitset_print(bts,opt_pfix,stream)
#else
#  define bitset_debug(bts,opt_pfix,stream)
#endif

/* Bitwise operations: setting, clearing, inverting and testing bits.
 */
void bitset_set (bitset_t *bts, anynum_t bit);
void bitset_clear (bitset_t *bts, anynum_t bit);
void bitset_toggle (bitset_t *bts, anynum_t bit);
bool bitset_test (bitset_t *bts, anynum_t bit);
void *bitset_element (bitset_t *bts, anynum_t idx);

/* Overview calculations: minimum and maximum bit number that is set.
 * Returns BITNUM_BAD when none exists.
 */
anynum_t bitset_min (bitset_t *bts);
anynum_t bitset_min (bitset_t *bts);
anynum_t bitset_count (bitset_t *bts);
bool bitset_isempty (bitset_t *bts);
bool bitset_disjoint (bitset_t *left, bitset_t *rigt);

/* Bulk operations: union, disjunction, subtraction, exor.
 */
void bitset_union (bitset_t *accu, bitset_t *operand);
void bitset_disjunction (bitset_t *accu, bitset_t *operand);
void bitset_subtract (bitset_t *accu, bitset_t *operand);
void bitset_bustract (bitset_t *accu, bitset_t *operand); // opposite subtract
void bitset_exor (bitset_t *accu, bitset_t *operand);


/* Iterator function prototype, see bitset_iterate() below.
 */
typedef void *bitset_iterator_f (anynum_t bitnr, void *data);

/* Iterate over a bitset, running a function for each bit number that
 * is set.  Pass a data structure between the calls, which are made in
 * the order of rising bit numbers, and return the final result.
 */
void *bitset_iterate (bitset_t *bts, bitset_iterator_f *it, void *data);

/* Iterator functions that can be run within a routine, without providing
 * a function to iterate over structures.  By adding more bitsets after
 * initialisation, it is possible to perform tests over more than one
 * bitset, and use the iterator just to advance a bit counter and related
 * cached structures to speed up indexing and masking.  Note the first
 * iterated item is not preset, but can be found after bitset_iterator_next()
 * or bitset_iterator_next_one().  The former function iterates over bit
 * numbers regardless of their state, the latter loops until it finds a
 * set bit in the provided bitset.  Most functions can handle a NULL
 * bitset, in which case they fall back to the opt_bits bitset provided to
 * bitset_iterator_init() -- which must in that case have been provided.
 */
void bitset_iterator_init (bitset_iter_t *bitit, bitset_t *opt_bits);
void bitset_iterator_add (bitset_iter_t *bitit, bitset_t *bits);
bool bitset_iterator_next (bitset_iter_t *bitit);
bool bitset_iterator_next_one (bitset_iter_t *bitit, bitset_t *bits);
anynum_t bitset_iterator_bitnum (bitset_iter_t *bitit);
bool bitset_iterator_test (bitset_iter_t *bitit, bitset_t *bits);
void *bitset_iterator_element (bitset_iter_t *bitit, bitset_t *bits);


#endif /* BITSET_H */
