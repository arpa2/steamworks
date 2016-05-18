/* bitset.c -- Handle bitsets of a predefined size and define mathops on them.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include "bitset.h"
#include "parser.h"


bitset_t *bitset_new (type_t *elt_type) {
	bitset_t *retval = malloc (sizeof (bitset_t));
	if (retval == NULL) {
		fatal_error ("Out of memory allocating bitset");
	}
	retval->maxbit = 0;
	retval->bits = NULL;
	retval->type = elt_type;
	return retval;
}

void bitset_destroy (bitset_t *bts) {
	if ((bts != NULL) && (bts->bits != NULL)) {
		free (bts->bits);
	}
	free (bts);
}

type_t *bitset_type (bitset_t *bts) {
	return bts->type;
}

void bitset_print (bitset_t *bts, char *opt_pfix, FILE *stream) {
	bitset_iter_t bi;
	char *comma = "";
	if (bts == NULL) {
		fprintf (stream, "bitset_NULL");
		return;
	}
	if (opt_pfix == NULL) {
		opt_pfix = "";
	}
	fprintf (stream, "%s{ ", bts->type->name);
	bitset_iterator_init (&bi, bts);
	while (bitset_iterator_next_one (&bi, NULL)) {
		fprintf (stream, "%s%s%d", comma, opt_pfix, bitset_iterator_bitnum (&bi));
		comma = ", ";
	}
	fprintf (stream, " }");
}

void bitset_copy (bitset_t *dest, bitset_t *orig) {
	anynum_t i, m1, m2;
	bitset_require_maxbit (dest, orig->maxbit);
	m1 = (orig->maxbit + 1) >> 5;
	m2 = (dest->maxbit + 1) >> 5;
	for (i=0; i<m1; i++) {
		dest->bits [i] = orig->bits [i];
	}
	for (   ; i<m2; i++) {
		dest->bits [i] = 0;
	}
}

bitset_t *bitset_clone (bitset_t *bts) {
	bitset_t *new;
	anynum_t i, m;
	new = bitset_new (bts->type);
	bitset_require_maxbit (new, bts->maxbit);
	m = (bts->maxbit + 1) >> 5;
	for (i=0; i<m; i++) {
		new->bits [i] = bts->bits [i];
	}
	return new;
}

void bitset_empty (bitset_t *bts) {
	anynum_t i, m;
	m = (bts->maxbit + 1) >> 5;
	for (i=0; i<m; i++) {
		bts->bits [i] = 0U;
	}
}

void bitset_require_maxbit (bitset_t *bts, anynum_t reqmaxbit) {
	reqmaxbit = reqmaxbit | 31;
	if ((bts->bits == NULL) || (bts->maxbit < reqmaxbit)) {
		int i;
		uint32_t *newbits = realloc (bts->bits, (reqmaxbit + 1) >> 3);
		if (newbits == NULL) {
			fatal_error ("Out of memory allocating bitset data");
		}
		for (i = ((bts->maxbit + 1) >> 5); i < ((reqmaxbit + 1) >> 5); i++) {
			newbits [i] = 0;
		}
		bts->bits = newbits;
		bts->maxbit = reqmaxbit;
	}
}

void bitset_set (bitset_t *bts, anynum_t bit) {
	bitset_require_maxbit (bts, bit);
	bts->bits [bit >> 5] |= (((uint32_t) 1) << (bit & 31));
}

void bitset_clear (bitset_t *bts, anynum_t bit) {
	bitset_require_maxbit (bts, bit);
	bts->bits [bit >> 5] &= ~(((uint32_t) 1) << (bit & 31));
}

void bitset_toggle (bitset_t *bts, anynum_t bit) {
	bitset_require_maxbit (bts, bit);
	bts->bits [bit >> 5] ^= (((uint32_t) 1) << (bit & 31));
}

bool bitset_test (bitset_t *bts, anynum_t bit) {
	if (bit > bts->maxbit) {
		return false;
	}
	if (bts->bits [bit >> 5] & (((uint32_t) 1) << (bit & 31))) {
		return true;
	} else {
		return false;
	}
}

void *bitset_element (bitset_t *bts, anynum_t idx) {
	return bts->type->index (bts->type->data, idx);
}

anynum_t bitset_min (bitset_t *bts) {
	anynum_t i, j, m;
	m = (bts->maxbit + 1) >> 5;
	i = 0;
	while (bts->bits [i] == 0) {
		if (i < m) {
			i++;
		} else {
			return BITNUM_BAD;
		}
	}
	j = 0;
	while ((bts->bits [i] & (((uint32_t) 1) << j)) == 0) {
		j++;
	}
	return (i<<5) | j;
}

anynum_t bitset_max (bitset_t *bts) {
	anynum_t i, j;
	i = (bts->maxbit >> 5) - 1;
	while (bts->bits [i] == 0) {
		if (i > 0) {
			i--;
		} else {
			return BITNUM_BAD;
		}
	}
	j = 31;
	while ((bts->bits [i] & (((uint32_t) 1) << j)) == 0) {
		j--;
	}
	return (i<<5) | j;
}

/* return |bts| */
anynum_t bitset_count (bitset_t *bts) {
	anynum_t i, j, m, count;
	m = (bts->maxbit + 1) >> 5;
	count = 0;
	for (i=0; i<m; i++) {
		for (j=0; j<32; j++) {
			if (bts->bits [i] & (((int32_t) 1) << j)) {
				count++;
			}
		}
	}
	return count;
}

/* return (|bts| == 0) */
bool bitset_isempty (bitset_t *bts) {
	anynum_t i, m;
	m = (bts->maxbit + 1) >> 5;
	for (i=0; i<m; i++) {
		if (bts->bits [i] != 0) {
			return false;
		}
	}
	return true;
}

/* accu := accu | operand */
void bitset_union (bitset_t *accu, bitset_t *operand) {
	anynum_t i, m;
	if (accu->maxbit < operand->maxbit) {
		bitset_require_maxbit (accu, operand->maxbit);
	} else {
		bitset_require_maxbit (operand, accu->maxbit);
	}
	m = (1 + accu->maxbit) >> 5;
	for (i=0; i<m; i++) {
		accu->bits [i] |= operand->bits [i];
	}
}

/* accu := accu & operand */
void bitset_disjunction (bitset_t *accu, bitset_t *operand) {
	anynum_t i, m1, m2;
	if (accu->maxbit < operand->maxbit) {
		bitset_require_maxbit (accu, operand->maxbit);
	}
	m2 = (1 + accu   ->maxbit) >> 5;
	m1 = (1 + operand->maxbit) >> 5;
	if (m1 > m2) {
		m1 = m2;
	}
	for (i=0; i<m1; i++) {
		accu->bits [i] &= operand->bits [i];
	}
	for (   ; i<m2; i++) {
		accu->bits [i] = 0U;
	}
}

/* return (|accu & operand| == 0) */
bool bitset_disjoint (bitset_t *left, bitset_t *rigt) {
	anynum_t i, m1, m2;
	m1 = (1 + left->maxbit) >> 5;
	m2 = (1 + rigt->maxbit) >> 5;
	if (m1 > m2) {
		m1 = m2;
	}
	for (i=0; i<m1; i++) {
		if ((left->bits [i] & rigt->bits [i]) != 0U) {
			return false;
		}
	}
	return true;
}

/* accu := accu - operand */
void bitset_subtract (bitset_t *accu, bitset_t *operand) {
	int i, m1, m2;
	m1 = (1 + operand->maxbit) >> 5;
	m2 = (1 + accu   ->maxbit) >> 5;
	if (m1 > m2) {
		m1 = m2;
	}
	for (i=0; i<m1; i++) {
		accu->bits [i] &= ~operand->bits [i];
	}
	for (   ; i<m2; i++) {
		accu->bits [i] = 0U;
	}
}

/* accu := operand - accu */
void bitset_bustract (bitset_t *accu, bitset_t *operand) {
	int i, m1, m2;
	m1 = (1 + operand->maxbit) >> 5;
	m2 = (1 + accu   ->maxbit) >> 5;
	if (m1 > m2) {
		m1 = m2;
	}
	for (i=0; i<m1; i++) {
		accu->bits [i] = operand->bits [i] & ~accu->bits [i];
	}
	for (   ; i<m2; i++) {
		accu->bits [i] = 0U;
	}
}

/* accu := accu ^ operand */
void bitset_exor (bitset_t *accu, bitset_t *operand) {
	anynum_t i, m;
	if (accu->maxbit < operand->maxbit) {
		bitset_require_maxbit (accu, operand->maxbit);
	} else {
		bitset_require_maxbit (operand, accu->maxbit);
	}
	m = (1 + accu->maxbit) >> 5;
	for (i=0; i<m; i++) {
		accu->bits [i] ^= operand->bits [i];
	}
}

#ifdef DEPRECATED_BITSET_ITERATION_USING_SEPARATE_FUNCTIONS
void *bitset_iterate (bitset_t *bts, bitset_iterator_f *it, void *data) {
	anynum_t i;
	for (i=0; i<bts->maxbit; i++) {
		if (bts->bits [i >> 5] & (((uint32_t) 1) << (i & 31))) {
			data = (*it) (i, data);
		}
	}
	return data;
}
#endif

void bitset_iterator_add (bitset_iter_t *bitit, bitset_t *bits) {
	if (bitit->maxbit < bits->maxbit) {
		bitit->maxbit = bits->maxbit;
	}
}

void bitset_iterator_init (bitset_iter_t *bitit, bitset_t *opt_bits) {
	bitit->maxbit = 0;
	bitit->curbit = -1;
	bitit->curint = -1;
	bitit->curmsk = 0;
	bitit->curset = opt_bits;
	if (opt_bits != NULL) {
		bitset_iterator_add (bitit, opt_bits);
	}
}

bool bitset_iterator_next (bitset_iter_t *bitit) {
	bitit->curbit++;
	bitit->curmsk <<= 1;
	if (bitit->curmsk == 0) {
		bitit->curint++;
		bitit->curmsk = 1;
	}
	return (bitit->curbit <= bitit->maxbit);
}

bool bitset_iterator_next_one (bitset_iter_t *bitit, bitset_t *bits) {
	if ((bits != NULL) && (bitit->maxbit < bits->maxbit)) {
		bitit->maxbit = bits->maxbit;
	}
	while (bitset_iterator_next (bitit)) {
		if (bitset_iterator_test (bitit, bits)) {
			return true;
		}
	}
	return false;
}

anynum_t bitset_iterator_bitnum (bitset_iter_t *bitit) {
	return bitit->curbit;
}

bool bitset_iterator_test (bitset_iter_t *bitit, bitset_t *bits) {
	if (bits == NULL) {
		bits = bitit->curset;
	}
	if (bits->bits == NULL) {
		return false;
	}
	if (bits->bits [bitit->curint] & bitit->curmsk) {
		return true;
	} else {
		return false;
	}
}

void *bitset_iterator_element (bitset_iter_t *bitit, bitset_t *bits) {
	if (bits == NULL) {
		bits = bitit->curset;
	}
	return bitset_element (bits, bitit->curint);
}

