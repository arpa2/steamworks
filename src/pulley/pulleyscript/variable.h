/* variable.h -- Symbol table for Pulley scripts.
 *
 * This is the one-level symbol table for Pulley scripts.  The implementation
 * is simple, reflecting the relative simplicity anticipated for individual
 * Pulley scripts.  Specifically, symbols are stored in a sequential array.
 *
 * Variables are assigned a unique number, from a climbing range starting
 * at 1; the value 0 is reserved for the special variable named "_" and
 * failure results from functions are returned as -1.
 *
 * Variables enter the script in their very own partition, numbered the same
 * as their variable number.  They may later be merged with other partitions,
 * and then one partition takes over the number from the other partition.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef PULLEYSCRIPT_VARIABLE_H
#define PULLEYSCRIPT_VARIABLE_H


#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "types.h"
#include "bitset.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structures for variable values, including the type of the element.
 * Note that constants are also entered into the variable table, since
 * they are treated like variables with respect to passing them around.
 */

typedef enum varkind {
	VARKIND_VARIABLE,
	VARKIND_PARAMETER,
	VARKIND_CONSTANT,
	VARKIND_ATTRTYPE,
	VARKIND_DRIVERNAME,
	VARKIND_BINDING,
} varkind_t;

enum vartype {
	VARTP_UNDEFINED,
	VARTP_INTEGER,
	VARTP_FLOAT,
	VARTP_STRING,
	VARTP_BLOB,
	VARTP_ATTROPTS,
};

struct var_value {
	enum vartype type;
	union {
		int union_integer;
		float union_float;
		char *union_string;
		struct {
			uint8_t *str;
			uint32_t len;
		} union_blob;
		struct list_attropt *union_attropts;
	} typed_union;
};

#define typed_integer	typed_union.union_integer
#define typed_float	typed_union.union_float
#define typed_string	typed_union.union_string
#define typed_blob	typed_union.union_blob
#define typed_attropts	typed_union.union_attropts

/* The special value for an option identifier is a list of attribute options,
 * which are strings that may modify behaviour.
 */
struct list_attropt {
	struct list_attropt *next;
	char option [1];
};


/* Variables are handled through their number, not through pointers to their
 * data structures, because those addresses may change over time.
 */


/* The variable table holding all variables (in an array, so direct access
 * is ill-advised).
 */


/* Management functions for variable tables.  After creating tables for
 * variables, generators, conditions and driverouts they have to know
 * about each others' types to be able to create their typed bitsets.
 */
struct vartab *vartab_new (void);
void vartab_destroy (struct vartab *tab);
void vartab_set_generator_type (struct vartab *tab, type_t *gentp);
void vartab_set_condition_type (struct vartab *tab, type_t *condtp);
void vartab_set_driverout_type (struct vartab *tab, type_t *drvtp);
type_t *vartab_share_type (struct vartab *tab);
type_t *vartab_share_driverout_type (struct vartab *tab);
struct vartab *vartab_from_type (type_t *vartype);

/* Printing functions */
void vartab_print (struct vartab *tab, char *opt_header, FILE *stream, int indent);
void vartab_print_names (struct vartab *tab, bitset_t *varset, FILE *stream);
void var_print (struct vartab *tab, varnum_t varnum, FILE *stream, int indent);
#ifdef DEBUG
#  define vartab_debug(tab,opt_header,stream,indent) vartab_print(tab,opt_header,stream,indent)
#  define var_debug(tab,varnum,stream,indent) vartab_print(tab,varnum,stream,indent)
#else
#  define vartab_debug(tab,opt_header,stream,indent)
#  define var_debug(tab,varnum,stream,indent)
#endif

/* Functions to handle variables; adding one, finding one and being sure to
 * have it available (meaning, adding it when it cannot be found yet).
 * Note that var_find() returns VARNUM_BAD if the variable cannot be found;
 * whereas the var_have*() functions then create a new variable.
 */
varnum_t var_add  (struct vartab *tab, char *name, varkind_t kind);
varnum_t var_find (struct vartab *tab, char *name, varkind_t kind);
varnum_t var_have (struct vartab *tab, char *name, varkind_t kind);
#if TODO_OLDER_INFORMATION_IDEA
varnum_t var_have_for_generator (struct vartab *tab, char *name, gennum_t gennum);
varnum_t var_have_for_condition (struct vartab *tab, char *name, cndnum_t cndnum);
varnum_t var_have_for_driverout (struct vartab *tab, char *name, drvnum_t drvnum);
#endif
void var_used_in_generator (struct vartab *tab, varnum_t varnum, gennum_t gennum);
void var_used_in_condition (struct vartab *tab, varnum_t varnum, cndnum_t cndnum);
void var_used_in_driverout (struct vartab *tab, varnum_t varnum, drvnum_t drvnum);

/* Fetch a variable's name, or current partition; overwrite an old partition
 * number with a new partition number throughout the variable table.
 */
char *var_get_name (struct vartab *tab, varnum_t varnum);

/* Fetch a variable's kind.
 */
varkind_t var_get_kind (struct vartab *tab, varnum_t varnum);

void vartab_merge_partitions (struct vartab *tab,
			unsigned int newpart, unsigned int oldpart);
void vartab_collect_varpartitions (struct vartab *tab);
bitset_t *var_share_varpartition (struct vartab *tab, varnum_t varnum);
bool var_test_is_generated (struct vartab *tab, varnum_t varnum);
bool var_test_is_generated_once (struct vartab *tab, varnum_t varnum);

/* Functions to share the bitsets holding a variable's bitsets with
 * generators, conditions, drivers.  The bitsets can be operated on without
 * restraint; effects will modify the variable's information base.  There
 * is no need to free these structures, they are truly shared.
 */
bitset_t *var_share_generators (struct vartab *tab, varnum_t varnum);
bitset_t *var_share_conditions (struct vartab *tab, varnum_t varnum);
bitset_t *var_share_driversout (struct vartab *tab, varnum_t varnum);

/* Set a variable to a given value.  This may replace a previously defined
 * value.  The value is copied, rather than shared.  Referenced memory will
 * not be cloned; it will be freed upon release of the variable value; this
 * is the case for VARTP_STRING, _BLOB and _ATTROPTS.
 */
void var_set_value (struct vartab *tab, varnum_t varnum, struct var_value *val);

/* Share the value of a variable.  The structures are shared with the current
 * variable use.  Note that values may change, so this is only safe in a
 * single-threaded environment.  Returns NULL for an undefined variable.
 */
struct var_value *var_share_value (struct vartab *tab, varnum_t varnum);

/* An iterator to use in the functions below.  Note how a (void *) carries
 * data in and out of the iterator function; this mechanism permits the
 * collection of information from the various variables, as each are
 * considered in their order of appearance.
 */
typedef void *var_iterator_f (struct vartab *tab, varnum_t var, void *data);

/* Iterators over the variable set, selecting entries with a matching
 * partition, generator, condition or driver.  The (void *) data is
 * carried through all iterator invocations on selected variables,
 * end the end result returned from the iterator call.
 */
void *vartab_iterate_partition (struct vartab *tab, unsigned int part,
			var_iterator_f *it, void *data);
void *vartab_iterate_generator (struct vartab *tab, unsigned int generator,
			var_iterator_f *it, void *data);
void *vartab_iterate_condition (struct vartab *tab, unsigned int condition,
			var_iterator_f *it, void *data);
void *vartab_iterate_driverout (struct vartab *tab, unsigned int driverout,
			var_iterator_f *it, void *data);

/* After assigning all generators to all variables, it is possible to run
 * the vartab_analyse_cheapest_generators() to investigate which of the
 * available generators for each variable are the cheapest.  This routine
 * can be used once as part of static analysis; the data is not dependent
 * on knowledge about variable sets known or unknown.
 *
 * Generators may produce multiple variables at once, so this is at best
 * useful to _estimate_ the cost of generating sets of variables, but at
 * least it is a basis for that.
 *
 * This function returns a boolean indicating which is nonzero if one or
 * more variables had no at all generator.
 *
 * The output of this analys phase can be requested with
 * var_get_cheapest_generator() which can store the cheapest generator and/or
 * its cost at pointed-at locations, and it returns a boolean indicating if a
 * generator exists at all for the given variable.
 */
int vartab_analyse_cheapest_generators (struct vartab *tab);

/* Retrieve information for the given variable from prior low-cost generator
 * analysis in * vartab_analyse_cheapest_generators() namely:
 *  - return whether a generator exists at all; false indicates a script error
 *  - return the cheapest generator in out_gen if non-NULL
 *  - return the cost of the cheapest generator in out_gencost if non-NULL
 */
struct generator;
bool var_get_cheapest_generator (struct vartab *tab, varnum_t varnum,
			gennum_t *out_gen,
			float *out_gencost);


/* Given a set of variables, transform it into a set of partition numbers.
 * Note that partition numbers are still variable numbers, however each
 * partition is represented by a single member from the partition.
 */
void var_vars2partitions (struct vartab *tab, bitset_t *varset);

/* Given a set of partition numbers, which are the numbers of the
 * representative variables for partitons, turn those into a set of
 * variables from all these partitions combined.
 */
void var_partitions2vars (struct vartab *tab, bitset_t *varset);

/* Make a closure under varpartitions of a variable set.  This means that
 * the set is expanded to include the original variables as well as all
 * others that are part of a varpartition.
 */
void var_partition_closure (struct vartab *tab, bitset_t *varset);

/* Check if the variable is the identifying one for the varpartition.  Everey
 * varpartition always contains exactly one variable whose number identifies
 * the partition.  Use this to avoid repeating work on variable sets that are
 * closed under partitioning.
 */
bool var_partition_identifiedby (struct vartab *tab, varnum_t varnum);

/* Check that all variables (VARKIND_VARIABLE) are bound by at least one
 * generator.  Return NULL if all are bound; otherwise return the variables
 * that are not bound.
 */
bitset_t *vartab_unbound_variables (struct vartab *tab);

/* Check that all variables (VARKIND_VARIABLE) are bound at most once by a
 * generator.  Return NULL if all are bound at most once; otherwise return
 * the variables that are bound by multiple generators.
 */
bitset_t *vartab_multibound_variables (struct vartab *tab);

#ifdef __cplusplus
}
#endif

#endif /* VARIABLE_H */
