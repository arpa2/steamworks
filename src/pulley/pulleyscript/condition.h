
#ifndef PULLEYSCRIPT_CONDITION_H
#define PULLEYSCRIPT_CONDITION_H

#include <stdio.h>

#include "bitset.h"
#include "types.h"
#include "lexhash.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct condition {
	float weight;
	bitset_t *vars_needed;
	hash_t linehash;
	int *calc;
	size_t calclen;
} condition_t;

struct cndtab;

/* Conditions codes for use with the condition interface, <0 to differ from varnum_t */
#define CND_SEQ_START	(-'(')
#define CND_NOT		(-'!')
#define CND_AND		(-'&')
#define CND_OR		(-'|')
#define CND_TRUE	(-'1')
#define CND_FALSE	(-'0')
#define CND_EQ		(-'=')
#define CND_NE		(-'/')
#define CND_LT		(-'<')
#define CND_GT		(-'>')
#define CND_LE		(-'[')
#define CND_GE		(-']')

struct cndtab *cndtab_new (void);
void cndtab_destroy (struct cndtab *tab);
void cndtab_set_variable_type (struct cndtab *tab, type_t *vartp);
type_t *cndtab_share_type (struct cndtab *tab);

struct cndtab *cndtab_from_type (type_t *cndtype);

/* Hash handling */
void cnd_set_hash (struct cndtab *tab, cndnum_t cndnum, hash_t cndhash);
hash_t cnd_get_hash (struct cndtab *tab, cndnum_t cndnum);

/* Insert condition elements; note that sequence are specially inserted, namely as
 *    LOG_TRUE,  ..., LOG_AND
 *    LOG_FALSE, ..., LOG_OR
 */
void cnd_pushvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum);
void cnd_pushop  (struct cndtab *tab, cndnum_t cndnum, int token);

/* Parser support for external reproduction of conditions */
void cnd_share_expression (struct cndtab *tab, cndnum_t cndnum, int **exp, size_t *explen);
void cnd_parse_operation (int *exp, size_t explen, int *operator_, int *operands, int **subexp, size_t *subexplen);
void cnd_parse_operand (int **exp, size_t *explen, int **subexp, size_t *subexplen);
varnum_t cnd_parse_variable (int **exp, size_t *explen);

void cndtab_print (struct cndtab *tab, char *head, FILE *stream, int indent);
#ifdef DEBUG
#  define cndtab_debug(tab,head,stream,indent) cndtab_print(tab,head,stream,indent)
#else
#  define cndtab_debug(tab,head,stream,indent)
#endif

void cndtab_drive_partitions (struct cndtab *tab);

cndnum_t cnd_new (struct cndtab *tab);

void cnd_set_weight (struct cndtab *tab, cndnum_t cndnum, float weight);
float cnd_get_weight (struct cndtab *tab, cndnum_t cndnum);

void cnd_add_guardvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum);

void cnd_needvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum);

/* Check that all conditions refer to at least one variable; it is silly to
 * have a constant-based (or context-free) expression to filter information
 * that passes through Pulley.
 * Return NULL if all are bound; otherwise return the variables
 * that are not bound.
 */
bitset_t *cndtab_invariant_conditions (struct cndtab *tab);

bool cnd_estimate_total_cost (struct cndtab *tab, cndnum_t cndnum,
				bitset_t *varsneeded,
				float *soln_mincost,
				struct gentab *gentab,
				unsigned int *soln_generator_count,
				gennum_t soln_generators []);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* CONDITION_H */
