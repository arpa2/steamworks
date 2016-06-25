/* generator.h -- Pulley script administration for generators.
 *
 * Generators are the source of variable forking; the receive LDAP updates
 * and match those to form sets of variable values that can be deliverd as
 * added values/forks or as removals of previously created values/forks.
 *
 * Generators might have their own LDAP listening SyncRepl thread, but they
 * may also be addressed by a shared thread that informs them all in turn.
 *
 * When a generator processes an update, it must pass through the "path of
 * least resistence" to generate the driver output that might come about in
 * combination with other generators -- and conditions of course.  So, when
 * new variable values are added (or old ones removed) then a list of actions
 * takes place to produce all combinations of variables that need to be
 * driven out.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef GENERATOR_H
#define GENERATOR_H

#include "types.h"
#include "bitset.h"
#include "lexhash.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gentab;

struct generator {
	float weight;
	varnum_t source;
	bitset_t *variables;
	bitset_t *driverout;
	struct path *path_of_least_resistence;
	hash_t linehash;
	bool cogenerate;
};


/* Management functions at the generator table level.  After setting up the
 * various tables, the types of the variable table and driverout table must
 * be setup in the generator table, for its internal typed bitsets.
 */
struct gentab *gentab_new (void);
void gentab_destroy (struct gentab *tab);
type_t *gentab_share_type (struct gentab *tab);
void gentab_set_variable_type (struct gentab *tab, type_t *vartp);
void gentab_set_driverout_type (struct gentab *tab, type_t *drvtp);
type_t *gentab_share_variable_type (struct gentab *tab);
type_t *gentab_share_driverout_type (struct gentab *tab);

unsigned int gentab_count (struct gentab *tab);


/* Hash handling */
void gen_set_hash (struct gentab *tab, gennum_t gennum, hash_t genhash);
hash_t gen_get_hash (struct gentab *tab, gennum_t gennum);


/* Printing functions */
void gen_print (struct gentab *tab, gennum_t gennum, FILE *stream, int indent);
void gentab_print (struct gentab *tab, char *head, FILE *stream, int indent);
#ifdef DEBUG
#  define gen_debug(tab,gennum,stream,indent) gen_print(tab,gennum,stream,indent)
#  define gentab_debug(tab,head,stream,indent) gen_print(tab,head,stream,indent)
#else
#  define gen_debug(tab,gennum,stream,indent)
#  define gentab_debug(tab,head,stream,indent)
#endif


/* Management of single generator instances within a generator table.
 * Note that (struct generator *) values may change when calling gen_new(),
 * and that the right way of addressing generators until then is to index
 * them by their integer number.
 */
gennum_t gen_new (struct gentab *tab, varnum_t source);

/* Retrieve the source DN variable that was supplied to this generator.
 */
varnum_t gen_get_source (struct gentab *tab, gennum_t gennum);

/* Manage whether this generator is used as a cogenerator or, which is the same,
 * has cogenerators.  For some engines, this may be used to determine whether
 * all generated values must be stored for iteration when it is called upon to
 * act as a cogenerator.
 */
void gen_set_cogeneration (struct gentab *tab, gennum_t gennunm);
bool gen_get_cogeneration (struct gentab *tab, gennum_t gennunm);

void gen_set_weight (struct gentab *tab, gennum_t gennum, float weight);
float gen_get_weight (struct gentab *tab, gennum_t gennum);
int gen_cmp_weight (void *gentab, const void *left, const void *right);
void gen_add_guardvar (struct gentab *tab, gennum_t gennum, varnum_t varnum);

/* Edit variable or driverout sets for the given generator */
void gen_add_variable  (struct gentab *tab, gennum_t gennum, varnum_t varnum);
void gen_add_driverout (struct gentab *tab, gennum_t gennum, drvnum_t drvnum);
bool gen_ask_variable  (struct gentab *tab, gennum_t gennum, varnum_t varnum);
bool gen_ask_driverout (struct gentab *tab, gennum_t gennum, drvnum_t drvnum);

bitset_t *gen_share_variables (struct gentab *tab, gennum_t gennum);
bitset_t *gen_share_driverout (struct gentab *tab, gennum_t gennum);

void gen_add_path_of_least_resistence (struct gentab *tab, gennum_t gennum, path_t *path);
path_t *gen_share_path_of_least_resistence (struct gentab *tab, gennum_t gennum);

#ifdef __cplusplus
}
#endif

#endif /* GENERATOR_H */
