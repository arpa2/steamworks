/* variable.c -- Symbol table for Pulley scripts.
 *
 * This is the one-level symbol table for Pulley scripts.  The implementation
 * is simple, reflecting the relative simplicity anticipated for individual
 * Pulley scripts.  Specifically, symbols are stored in a sequential array.
 *
 * Variables are assigned a unique number, from a climbing range starting
 * at 1; the value 0 is reserved for the special variable named "_" and
 * failure results from functions are returned as VARNUM_BAD.
 *
 * Variables enter the script in their very own partition, numbered the same
 * as their variable number.  They may later be merged with other partitions,
 * and then one partition takes over the number from the other partition.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "generator.h"
#include "parser.h"

#include "variable_int.h"


static void *vartab_index (void *data, unsigned int index);

struct vartab *vartab_new (void) {
	struct vartab *retval = malloc (sizeof (struct vartab));
	if (retval == NULL) {
		fatal_error ("Out of memory allocating variable table");
	}
	retval->vartype.data = (void *) retval;
	retval->vartype.index = vartab_index;
	retval->vartype.name = "Variable";
	retval->vars = NULL;
	retval->count_vars = 0;
	retval->allocated_vars = 0;
	return retval;
}

static void var_cleanup (struct vartab *tab, varnum_t varnum);

void vartab_destroy (struct vartab *tab) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		var_cleanup (tab, i);
	}
	free(tab->vars);
	tab->vars = NULL;
	free (tab);
}

static void var_cleanup (struct vartab *tab, varnum_t varnum) {
	/* This function is only used in the vartab_destroy() context.
	 * Its utility is to centralise symbol cleanup operations.
	 */
	struct variable *var = &tab->vars [varnum];
	free (var->name);
	bitset_destroy (var->conditions);
	bitset_destroy (var->driversout);
	if (var->partition == varnum) {
		/* This bitset should be destroyed precisely once, so do it
		 * where the partition is numbered by the variable.
		 */
		if (var->shared_varpartition != NULL) {
			bitset_destroy (var->shared_varpartition);
		}
	}
}

void vartab_set_generator_type (struct vartab *tab, type_t *gentp) {
	tab->gentype = gentp;
}

void vartab_set_condition_type (struct vartab *tab, type_t *condtp) {
	tab->condtype = condtp;
}

void vartab_set_driverout_type (struct vartab *tab, type_t *drvtp) {
	tab->drvtype = drvtp;
}

struct vartab *vartab_from_type (type_t *vartype) {
	return (struct vartab *) vartype->data;
}

static void *vartab_index (void *data, unsigned int index) {
	return (void *) (&(((struct vartab *) data)->vars [index]));
}

type_t *vartab_share_type (struct vartab *tab) {
	return &tab->vartype;
}

type_t *vartab_share_driverout_type (struct vartab *tab) {
	return tab->drvtype;
}

void vartab_print (struct vartab *tab, char *opt_header, FILE *stream, int indent) {
	varnum_t i;
	if (opt_header != NULL) {
		fprintf (stream, "Variable table %s\n", opt_header);
	}
	for (i=0; i<tab->count_vars; i++) {
		struct variable *var = &tab->vars [i];
		char *kindstr;
		switch (var->kind) {
		case VARKIND_VARIABLE:   kindstr="var  "; break;
		case VARKIND_CONSTANT:   kindstr="const"; break;
		case VARKIND_ATTRTYPE:   kindstr="attr "; break;
		case VARKIND_PARAMETER:  kindstr="param"; break;
		case VARKIND_DRIVERNAME: kindstr="drvnm"; break;
		case VARKIND_BINDING:    kindstr="bind "; break;
		default:                 kindstr="undef"; break;
		}
		fprintf (stream, "V%d kind=%s partition=P%d, cheapest_generator=G%d, name=\"%s\"\n", i, kindstr, var->partition, var->cheapest_generator, var->name);
	}
}

void vartab_print_names (struct vartab *tab, bitset_t *varset, FILE *stream) {
	int i;
	char *comma = "";
	for (i=0; i<tab->count_vars; i++) {
		if (bitset_test (varset, i)) {
			fprintf (stream, "%s%s", comma, tab->vars [i].name);
			comma = ",";
		}
	}
}

/* After variable partitions have stabilised, collect them into a reverse relation
 * that permits treating the variables contained within a partition as a set.  The
 * set is stored in the variable with the same number as the partition; this is the
 * one whose initial "variable_number == partition_number" has survived partition
 * merges.  The set is made available to all variables in a partition, but it is a
 * shared data structure that is considered to "originally" belong to the variable
 * whose number matches the partition number; this is vital during cleanup.
 */
void vartab_collect_varpartitions (struct vartab *tab) {
	varnum_t i;
	/* Create precisely one shared varpartition bitset, and
	 * do that where the partition matches the variable number.
	 */
	for (i=0; i<tab->count_vars; i++) {
		if (tab->vars [i].partition == i) {
			tab->vars [i].shared_varpartition = bitset_new (&tab->vartype);
		}
	}
	/* Now copy the shared varpartition to all variables in the partition,
	 * even to itself, and set the corresponding bit in varpartitions.
	 */
	for (i=0; i<tab->count_vars; i++) {
		struct variable *var = &tab->vars [i];
		var->shared_varpartition = tab->vars [var->partition].shared_varpartition;
		bitset_set (var->shared_varpartition, i);
	}
}

/* Given a set of variables, transform it into a set of partition numbers.
 * Note that partition numbers are still variable numbers, however each
 * partition is represented by a single member from the partition.
 */
void var_vars2partitions (struct vartab *tab, bitset_t *varset) {
	bitset_iter_t vi;
	bitset_iterator_init (&vi, varset);
	while (bitset_iterator_next_one (&vi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&vi);
		// Replace member bit by partition representative bit:
		bitset_clear (varset, varnum                      );
		bitset_set   (varset, tab->vars [varnum].partition);
	}
}

/* Given a set of partition numbers, which are the numbers of the
 * representative variables for partitons, turn those into a set of
 * variables from all these partitions combined.
 */
void var_partitions2vars (struct vartab *tab, bitset_t *varset) {
	bitset_iter_t vi;
	bitset_iterator_init (&vi, varset);
	while (bitset_iterator_next_one (&vi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&vi);
		// Do not repeat when running into non-representative members:
		if (varnum == tab->vars [varnum].partition) {
			bitset_union (varset, tab->vars [varnum].shared_varpartition);
		}
	}
}

/* Make a closure under varpartitions of a variable set.  This means that
 * the set is expanded to include the original variables as well as all
 * others that are part of a varpartition.
 */
void var_partition_closure (struct vartab *tab, bitset_t *varset) {
	var_vars2partitions (tab, varset);
	var_partitions2vars (tab, varset);
}

/* Check if the variable is the identifying one for the varpartition.  Everey
 * varpartition always contains exactly one variable whose number identifies
 * the partition.  Use this to avoid repeating work on variable sets that are
 * closed under partitioning.
 */
bool var_partition_identifiedby (struct vartab *tab, varnum_t varnum) {
	return (varnum == tab->vars [varnum].partition);
}

/* Print an individual variable with more detail than in the vartab */
void var_print (struct vartab *tab, varnum_t varnum, FILE *stream, int indent) {
	struct variable *var = &tab->vars [varnum];
	// Overview line
	fprintf (stream, "V%d partition=P%d, cheapest_generator=G%d, name=\"%s\"", varnum, var->partition, var->cheapest_generator, var->name);
	// Partition
	fprintf (stream, "\nvarpartition=");
	bitset_print (var->shared_varpartition, "V", stream);
	// Generators
	fprintf (stream, "\ngenerators=");
	bitset_print (var->generators, "G", stream);
	// Conditions
	fprintf (stream, "\nconditions=");
	bitset_print (var->conditions, "C", stream);
	// Driverouts
	fprintf (stream, "\ndriversout=");
	bitset_print (var->driversout, "D", stream);
	// Terminating newline
	fprintf (stream, "\n");
}

/* Print a list of variables by their names */
void varset_print (struct vartab *tab, bitset_t varset) {
}

varnum_t var_add (struct vartab *tab, char *name, varkind_t kind) {
	struct variable *newvar;
	if (tab->count_vars >= tab->allocated_vars) {
		int alloc = tab->allocated_vars + 100;
		struct variable *newvars;
		newvars = realloc (tab->vars, alloc * sizeof (struct variable));
		if (newvars == NULL) {
			fatal_error ("Out of memory allocating variable");
		}
		tab->vars = newvars;
		tab->allocated_vars = alloc;
	}
	newvar = &tab->vars [tab->count_vars];	/* incremented when returned */
	newvar->name = strdup (name);
	if (newvar->name == NULL) {
		fatal_error ("Out of memory allocating variable name");
	}
	newvar->partition = tab->count_vars;
	newvar->cheapest_generator = GENNUM_BAD;
	newvar->generators = bitset_new (tab->gentype);
	newvar->conditions = bitset_new (tab->condtype);
	newvar->driversout = bitset_new (tab->drvtype);
	newvar->shared_varpartition = NULL;
	newvar->kind = kind;
	newvar->value.type = VARTP_UNDEFINED;
	return tab->count_vars++;
}

varnum_t var_find (struct vartab *tab, const char *name, varkind_t kind) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		if ((tab->vars [i].kind == kind) && (strcmp (tab->vars [i].name, name) == 0)) {
			return i;
		}
	}
	return VARNUM_BAD;
}

varnum_t var_have (struct vartab *tab, char *name, varkind_t kind) {
	varnum_t thevar;
	thevar = var_find (tab, name, kind);
	if (thevar == VARNUM_BAD) {
		thevar = var_add (tab, name, kind);
	}
	return thevar;
}

#if TODO_OLDER_INFORMATION_IDEA
varnum_t var_have_for_generator (struct vartab *tab, char *name, gennum_t gennum) {
	varnum_t thevar = var_have (tab, name, VARKIND_VARIABLE);
	var_used_in_generator (tab, thevar, gennum);
	return thevar;
}
#endif

void var_used_in_generator (struct vartab *tab, varnum_t varnum, gennum_t gennum) {
	bitset_set (tab->vars [varnum].generators, gennum);
}

#if TODO_OLDER_INFORMATION_IDEA
varnum_t var_have_for_condition (struct vartab *tab, char *name, cndnum_t cndnum) {
	varnum_t thevar = var_have (tab, name, VARKIND_VARIABLE);
	bitset_set (tab->vars [thevar].conditions, cndnum);
	return thevar;
}
#endif

void var_used_in_condition (struct vartab *tab, varnum_t varnum, cndnum_t cndnum) {
	bitset_set (tab->vars [varnum].conditions, cndnum);
}

#if TODO_OLDER_INFORMATION_IDEA
varnum_t var_have_for_driverout (struct vartab *tab, char *name, drvnum_t drvnum) {
	varnum_t thevar = var_have (tab, name, VARKIND_VARIABLE);
	bitset_set (tab->vars [thevar].driversout, drvnum);
	return thevar;
}
#endif

void var_used_in_driverout (struct vartab *tab, varnum_t varnum, drvnum_t drvnum) {
	bitset_set (tab->vars [varnum].driversout, drvnum);
}

/* Iterate over the variable table and replace the partition oldpart by the newpart.
 * This is used to unite two partitions that may hitherto have been separate when
 * they should overlap.  Initially, every variable has its own partition assigned,
 * but advances in knowledge of the program may require partitions to merge.  This
 * procedure can be repeated with various partitions, and will then result in a
 * transitively closed merge of partitions.
 */
void vartab_merge_partitions (struct vartab *tab,
			unsigned int newpart, unsigned int oldpart) {
	int i;
	struct variable *here;
	if (tab->vars [oldpart].kind != VARKIND_VARIABLE) {
		return;
	}
	if (tab->vars [newpart].kind != VARKIND_VARIABLE) {
		return;
	}
	for (i=0; i<tab->count_vars; i++) {
		if (tab->vars [i].partition == oldpart) {
			tab->vars [i].partition = newpart;
		}
	}
}

char *var_get_name (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].name;
}

varkind_t var_get_kind (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].kind;
}

/* Set a variable to a given value.  This may replace a previously defined
 * value.  The value is copied, rather than shared.  Referenced memory will
 * not be cloned; it will be freed upon release of the variable value; this
 * is the case for VARTP_STRING, _BLOB and _ATTROPTS.
 */
void var_set_value (struct vartab *tab, varnum_t varnum, struct var_value *val) {
	struct var_value *dst = &tab->vars [varnum].value;
	struct list_attropt *here, *next;
	switch (dst->type) {
	case VARTP_STRING:
		free (dst->typed_string);
		break;
	case VARTP_BLOB:
		free (dst->typed_blob.str);
		break;
	case VARTP_ATTROPTS:
		here = dst->typed_attropts;
		while (here) {
			next = here->next;
			free (here);
		}
		break;
	default:
		break;
	}
	memcpy (dst, val, sizeof (struct var_value));
}

/* Share the value of a variable.  The structures are shared with the current
 * variable use.  Note that values may change, so this is only safe in a
 * single-threaded environment.  Returns NULL for an undefined variable.
 */
struct var_value *var_share_value (struct vartab *tab, varnum_t varnum) {
	if (tab->vars [varnum].value.type == VARTP_UNDEFINED) {
		return NULL;
	} else {
		return &tab->vars [varnum].value;
	}
}

bool var_test_is_generated (struct vartab *tab, varnum_t varnum) {
	return !bitset_isempty (tab->vars [varnum].generators);
}

bool var_test_is_generated_once (struct vartab *tab, varnum_t varnum) {
	return 1 == bitset_count (tab->vars [varnum].generators);
}

bitset_t *var_share_varpartition (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].shared_varpartition;
}

bitset_t *var_share_generators (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].generators;
}

bitset_t *var_share_conditions (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].conditions;
}

bitset_t *var_share_driversout (struct vartab *tab, varnum_t varnum) {
	return tab->vars [varnum].driversout;
}


/* Check that all variables (VARKIND_VARIABLE) are bound by at least one
 * generator.  Return NULL if all are bound; otherwise return the variables
 * that are not bound.
 */
bitset_t *vartab_unbound_variables (struct vartab *tab) {
	bitset_t *retval = NULL;
	varnum_t i;
	for (i=0; i<tab->count_vars; i++) {
		if (tab->vars [i].kind != VARKIND_VARIABLE) {
			continue;
		}
		if (bitset_isempty (tab->vars [i].generators)) {
			// Error found
			if (retval == NULL) {
				retval = bitset_new (&tab->vartype);
			}
			bitset_set (retval, i);
		}
	}
	return retval;
}


/* Check that all variables (VARKIND_VARIABLE) are bound at most once by a
 * generator.  Return NULL if all are bound at most once; otherwise return
 * the variables that are bound by multiple generators.
 */
bitset_t *vartab_multibound_variables (struct vartab *tab) {
	bitset_t *retval = NULL;
	varnum_t i;
	for (i=0; i<tab->count_vars; i++) {
		if (tab->vars [i].kind != VARKIND_VARIABLE) {
			continue;
		}
		if (1 < bitset_count (tab->vars [i].generators)) {
			// Error found
			if (retval == NULL) {
				retval = bitset_new (&tab->vartype);
			}
			bitset_set (retval, i);
		}
	}
	return retval;
}


void *vartab_iterate_partition (struct vartab *tab, unsigned int part,
			var_iterator_f *it, void *data) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		if (tab->vars [i].partition == part) {
			data = (*it) (tab, i, data);
		}
	}
	return data;
}

void *vartab_iterate_generator (struct vartab *tab, unsigned int generator,
			var_iterator_f *it, void *data) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		if (bitset_test (tab->vars [i].generators, generator)) {
			data = (*it) (tab, i, data);
		}
	}
	return data;
}

void *vartab_iterate_condition (struct vartab *tab, unsigned int condition,
			var_iterator_f *it, void *data) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		if (bitset_test (tab->vars [i].conditions, condition)) {
			data = (*it) (tab, i, data);
		}
	}
	return data;
}

void *vartab_iterate_driver (struct vartab *tab, unsigned int driverout,
			var_iterator_f *it, void *data) {
	int i;
	for (i=0; i<tab->count_vars; i++) {
		if (bitset_test (tab->vars [i].driversout, driverout)) {
			data = (*it) (tab, i, data);
		}
	}
	return data;
}

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
int vartab_analyse_cheapest_generators (struct vartab *tab) {
	varnum_t vi;
	bool all_have_soln = true;
	for (vi=0; vi<tab->count_vars; vi++) {
		bitset_iter_t gi;
		gennum_t soln_genlive;
		float soln_mincost;
		bitset_t *gens;
		if (tab->vars [vi].kind != VARKIND_VARIABLE) {
			continue;
		}
		soln_genlive = GENNUM_BAD;
		gens = tab->vars [vi].generators;
		bitset_iterator_init (&gi, gens);
		while (bitset_iterator_next_one (&gi, NULL)) {
			float cand_cost;
			gennum_t g = bitset_iterator_bitnum (&gi);
			cand_cost = gen_get_weight (
					gentab_from_type (tab->gentype), g);
			if ((soln_genlive == GENNUM_BAD) || (cand_cost < soln_mincost)) {
				soln_genlive = g;
				soln_mincost = cand_cost;
			}
		}
		if (soln_genlive != GENNUM_BAD) {
			tab->vars [vi].cheapest_generator = soln_genlive;
		} else {
			all_have_soln = false;
		}
	}
	return !all_have_soln;
}

/* Retrieve information for the given variable from prior low-cost generator
 * analysis in * vartab_analyse_cheapest_generators() namely:
 *  - return whether a generator exists at all; false indicates a script error
 *  - return the cheapest generator in out_gen if non-NULL
 *  - return the cost of the cheapest generator in out_gencost if non-NULL
 */
bool var_get_cheapest_generator (struct vartab *tab, varnum_t varnum,
			gennum_t *out_gen,
			float *out_gencost) {
	if (tab->vars [varnum].cheapest_generator != GENNUM_BAD) {
		gennum_t cheapgen = tab->vars [varnum].cheapest_generator;
		if (out_gen != NULL) {
			*out_gen  = cheapgen;
		}
		if (out_gencost != NULL) {
			*out_gencost = gen_get_weight (
				gentab_from_type (tab->gentype), cheapgen);
		}
		return true;
	} else {
		return false;
	}
}

