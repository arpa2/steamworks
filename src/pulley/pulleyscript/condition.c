/* condition.c -- Handle conditions in Pulley scripts
 *
 * Conditions refer to a number of variables, and are the cause of partitioning
 * variables.  So, whenever a condition refers to a variable it will be
 * explicitly noted.
 *
 * Conditions reduce the number of cases under consideration, usually
 * reflected in a cost factor under 1.0, so they are ideally ordered as
 * early as possible on the "path of least resistence".  By necessity
 * however, at least one generator must run first for all the variables
 * needed by a condition.
 *
 * Conditions are the driving force behind the construction of the path of
 * least resistence; presented with a number of variables that are still to
 * be generated, they determine the least cost of doing so by looking at the
 * various generators configurations that could be helpful, and their cost
 * of running.  This yields a cost-of-running-now (that is, given the vars
 * that are yet to be determined).
 *
 * The path of least resistence is built by picking the condition with the
 * lowest cost-of-running-now, and preceding it with all the generators that
 * it picked for that action.  Those generators will of course be ordered
 * so that the biggest generators run last, just before the condition that
 * tears down their production.
 *
 * As may be clear from this, conditions have a bit of housekeeping to do.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "variable.h"
#include "generator.h"
#include "condition.h"
#include "parser.h"


/* The condition table holding all conditions (in an array, so direct access
 * is ill-advised).
 */
struct cndtab {
	type_t cndtype;
	type_t *vartype;
	struct condition *cnds;
	unsigned int allocated_cnds;
	unsigned int count_cnds;
};

static void *cndtab_index (void *data, unsigned int index);

struct cndtab *cndtab_new (void) {
	struct cndtab *retval = malloc (sizeof (struct cndtab));
	if (retval == NULL) {
		fatal_error ("Out of memory allocating condition table");
	}
	retval->cndtype.data = (void *) retval;
	retval->cndtype.index = cndtab_index;
	retval->cndtype.name = "Condition";
	retval->cnds = NULL;
	retval->count_cnds = 0;
	retval->allocated_cnds = 0;
	return retval;
}

static void cnd_cleanup (struct condition *cnd);

void cndtab_destroy (struct cndtab *tab) {
	int i;
	for (i=0; i<tab->count_cnds; i++) {
		cnd_cleanup (&tab->cnds [i]);
	}
	free (tab);
}

/* Hash handling */
void cnd_set_hash (struct cndtab *tab, cndnum_t cndnum, hash_t cndhash) {
	tab->cnds [cndnum].linehash = cndhash;
}

hash_t cnd_get_hash (struct cndtab *tab, cndnum_t cndnum) {
	return tab->cnds [cndnum].linehash;
}

/* Insert condition elements; note that sequence are specially inserted, namely as
 *    CND_SEQ_START, ..., CND_AND
 *    CND_SEQ_START, ..., CND_OR
 */
void cnd_pushop  (struct cndtab *tab, cndnum_t cndnum, int token) {
	int stacksz;
	int start, end, pos;
	int *calc;
	int count;
	if ((token != CND_AND) && (token != CND_OR)) {
		cnd_pushvar (tab, cndnum, token);
		return;
	}
	// Chase for the last start of the sequence, the last CND_SEQ_START symbol
	end  = tab->cnds [cndnum].calclen;
	calc = tab->cnds [cndnum].calc;
	start = end - 1;
	while (calc [start] != CND_SEQ_START) {
		start--;
	}
	memmove (calc + start, calc + start + 1, sizeof (int) * (end - start - 1));
	tab->cnds [cndnum].calclen = --end;
	count = 0;
	pos = end - 1;
	// Consume arguments, reducing the stack from 1 to 0
	stacksz = 1;
	while (pos >= start) {
		switch (calc [pos]) {
		case CND_NOT:
			/* neutral */;
			break;
		case CND_AND:
		case CND_OR:
		case CND_EQ:
		case CND_NE:
		case CND_LT:
		case CND_GT:
		case CND_LE:
		case CND_GE:
			/* need to collect an extra stack element */
			stacksz++;
			break;
		case CND_TRUE:
		case CND_FALSE:
		default:
			/* constants / variables can be collected as stack elements */
			stacksz--;
			break;
		}
		if (stacksz == 0) {
			/* found a start of expression */
			count++;
			if (count >= 2) {
				/* append CND_AND or CND_OR */
				cnd_pushvar (tab, cndnum, token);
			}
			/* chase for another complete expression */
			stacksz = 1;
		}
		pos--;
	}
	/* handle smaller cases: count==0 becomes neutral; count==1 is ignored */
	if (count == 0) {
		cnd_pushvar (tab, cndnum, (token == CND_AND)? CND_TRUE: CND_FALSE);
	}
}

void cnd_pushvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum) {
	int end;
	int *calc;
	end  = tab->cnds [cndnum].calclen;
	calc = tab->cnds [cndnum].calc;
	if (end % 64 == 0) {
		calc = realloc (calc, (end + 64) * sizeof (int));
		if (calc == NULL) {
			fprintf (stderr, "Failed to allocate memory for condition storage\n");
			exit (1);
		}
		tab->cnds [cndnum].calc = calc;
	}
	calc [end++] = varnum; // negative values represent -LOG_xxx
	tab->cnds [cndnum].calclen = end;
}

/* Parser support for condition reproduction */

void cnd_share_expression (struct cndtab *tab, cndnum_t cndnum, int **exp, size_t *explen) {
	*exp    = tab->cnds [cndnum].calc;
	*explen = tab->cnds [cndnum].calclen;
}

void cnd_parse_operation (int *exp, size_t explen, int *operator, int *operands, int **subexp, size_t *subexplen) {
	*operator = exp [--explen];
	switch (exp [explen]) {
	case CND_TRUE:
	case CND_FALSE:
		*operands = 0;
		break;
	case CND_AND:
	case CND_OR:
		*operands = 2;
		/* Traverse the sequence of operators */
		while (exp [explen - 1] == exp [explen]) {
			(*operands)++;
			explen--;
		}
		break;
	case CND_EQ:
	case CND_NE:
	case CND_LT:
	case CND_GT:
	case CND_LE:
	case CND_GE:
		*operands = 2;
		break;
	case CND_NOT:
		*operands = 1;
		break;
	default:
		fprintf (stderr, "Unknown operator %d in condition\n", *operator);
		exit (1);
	}
	*subexp    = exp;
	*subexplen = explen;
}

void cnd_parse_operand (int **exp, size_t *explen, int **subexp, size_t *subexplen) {
	int stacksz;
	int walklen = *explen;
	int *walker = (*exp) + walklen;
	*subexplen = walklen;	// Will subtract new walklen later
	stacksz = 1;
	while (stacksz != 0) {
		walklen--;
		switch (*--walker) {
		case CND_NOT:
			/* neutral */
			break;
		case CND_AND:
		case CND_OR:
		case CND_EQ:
		case CND_NE:
		case CND_LT:
		case CND_GT:
		case CND_LE:
		case CND_GE:
			/* need to consume one more stack element */
			stacksz++;
			break;
		default:
			/* constants / variables may be consumed as a stack element */
			stacksz--;
			break;
		}
	}
	*explen     = walklen;
	*subexplen -= walklen;	// *subexplen now is the remaining length after explen
	*subexp     = walker;
}

varnum_t cnd_parse_variable (int **exp, size_t *explen) {
	return (*exp) [--(*explen)];
}



type_t *cndtab_share_type (struct cndtab *tab) {
	return &tab->cndtype;
}

static void *cndtab_index (void *data, unsigned int index) {
	return (void *) (&(((struct cndtab *) data)->cnds [index]));
}

struct cndtab *cndtab_from_type (type_t *cndtype) {
	return (struct cndtab *) cndtype->data;
}

void cndtab_print (struct cndtab *tab, char *head, FILE *stream, int indent) {
	cndnum_t i;
	int j;
	char *comma;
	fprintf (stream, "Condition table %s\n", head);
	for (i=0; i<tab->count_cnds; i++) {
		fprintf (stream, "C%d, lexhash=", i);
		hash_print (&tab->cnds [i].linehash, stream);
		fprintf (stream, ", vars_needed=");
		bitset_print (tab->cnds [i].vars_needed, "V", stream);
		comma = "\nexpression = ";
		for (j=0; j<tab->cnds [i].calclen; j++) {
			if (tab->cnds [i].calc [j] < 0) {
				fprintf (stream, "%s%c", comma, -tab->cnds [i].calc [j]);
			} else {
				fprintf (stream, "%sV%d", comma, tab->cnds [i].calc [j]);
			}
			comma = ",";
		}
		fprintf (stream, "\n");
	}
}

void cndtab_set_variable_type (struct cndtab *tab, type_t *vartp) {
	tab->vartype = vartp;
}



cndnum_t cnd_new (struct cndtab *tab) {
	struct condition *newcnd;
	if (tab->count_cnds >= tab->allocated_cnds) {
		int alloc = tab->allocated_cnds + 100;
		struct condition *newcnds;
		newcnds = realloc (tab->cnds, alloc * sizeof (struct condition));
		if (newcnds == NULL) {
			fatal_error ("Out of memory allocating condition");
		}
		tab->cnds = newcnds;
		tab->allocated_cnds = alloc;
	}
	newcnd = &tab->cnds [tab->count_cnds];	/* Incremented on return */
	newcnd->weight = 0.1;	/* Default setting */
	newcnd->vars_needed = bitset_new (tab->vartype);
	newcnd->calc = NULL;
	newcnd->calclen = 0;
	return tab->count_cnds++;
}

static void cnd_cleanup (struct condition *cnd) {
	/* This function is only used in the cndtab_destroy() context.
	 * It's utility is to centralise symbol cleanup operations.
	 */
	if (cnd->calc != NULL) {
		free (cnd->calc);
		cnd->calclen = 0;
	}
}

void cnd_set_weight (struct cndtab *tab, cndnum_t cndnum, float weight) {
	if (weight > 1.0) {
		fprintf (stderr, "WARNING: Obscure condition weight %f is over 1.0\n", weight);
	}
	tab->cnds [cndnum].weight = weight;
}

float cnd_get_weight (struct cndtab *tab, cndnum_t cndnum) {
	return tab->cnds [cndnum].weight;
}

void cnd_add_guardvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum) {
	// There is no distinction with treatment of normally referenced vars
	bitset_set (tab->cnds [cndnum].vars_needed, varnum);
}

void cnd_needvar (struct cndtab *tab, cndnum_t cndnum, varnum_t varnum) {
	bitset_set (tab->cnds [cndnum].vars_needed, varnum);
}

/* Based on the variables needed in each condition, partition the variables.
 * Initially, each variable is created within its own, size-1 partition.
 * Conditions that mention multiple variables however, pull variables together
 * into the same partition.  This is done by renumbering one partition to
 * another partition.
 */
static void cnd_drive_partitions (struct cndtab *tab, cndnum_t cndnum) {
	bitset_iter_t bi;
	varnum_t onevar, othvar;
	struct vartab *vartab;
	bitset_iterator_init (&bi, tab->cnds [cndnum].vars_needed);
	if (!bitset_iterator_next_one (&bi, NULL)) {
		return;
	}
	vartab = vartab_from_type (tab->vartype);
	onevar = bitset_iterator_bitnum (&bi);
	while (bitset_iterator_next_one (&bi, NULL)) {
		othvar = bitset_iterator_bitnum (&bi);
		vartab_merge_partitions (vartab, onevar, othvar);
	}
}

void cndtab_drive_partitions (struct cndtab *tab) {
	cndnum_t i;
	for (i=0; i<tab->count_cnds; i++) {
		cnd_drive_partitions (tab, i);
	}
}


/* Check that all conditions refer to at least one variable; it is silly to
 * have a constant-based (or context-free) expression to filter information
 * that passes through Pulley.
 * Return NULL if all are bound; otherwise return the variables
 * that are not bound.
 */
bitset_t *cndtab_invariant_conditions (struct cndtab *tab) {
	bitset_t *retval = NULL;
	cndnum_t i;
	for (i=0; i<tab->count_cnds; i++) {
		if (bitset_isempty (tab->cnds [i].vars_needed)) {
			// Error found
			if (retval == NULL) {
				retval = bitset_new (&tab->cndtype);
			}
			bitset_set (retval, i);
		}
	}
	return retval;
}

#ifdef FEEL_LIKE_DOING_MORE_THAN_SEEMS_SANE
/* This is a complex, recursive function to determine the cost for
 * calculating a condition after any still-needed variables have been
 * generated.  Cost is measured in terms of the anticipated number of
 * generated new variable sets, as provided by "*float" annotations
 * on generator and condition lines.
 *
 * Call this with a condition and varsneeded.  The soln_xxx fields will
 * be filled with the requested information, namely the total cost
 * and the generators needed to get there.  The soln_generators bitset
 * is assumed to already exist.
 *
 * This function may not always be able to determine a solution; this
 * can be the case if variables are mentioned in conditions but not in
 * generators, for instance.  In such situations, the function returns
 * false.  It is considered a success if all variables that are needed
 * for a condition are resolved by at least one generator.
 *
 * This is an important function when determining the "path of least
 * resistence" from any generator activity; the least costly condition
 * will always be calculated first, then the calculations are redone,
 * and what is then the least will be selected, and so on.  The
 * generators required will be inserted into the path before their
 * dependent conditions, in the order of least-costly ones first.
 *
 * This is a rather heavy form of analysis, but it is executed only
 * once, at startup of the environment.  After that, the SyncRepl
 * updates can ripple through in great speed.  Note that the complexity
 * and time delays of these procedures rise with the (usually modest)
 * degree of interrelationships between generators and conditions.
 */
bool cnd_get_min_total_cost (struct condition *cc, bitset_t *varsneeded,
				float *soln_mincost,
				bitset_t *soln_generators) {
	bool have_soln = false;
	float soln_mincost;
	varnum_t v, v_max;
	gennum_t g, g_max;
	//
	// Allocate bitsets that will be used while cycling, but not
	// across recursive uses.  To that and, a pile or pool of
	// bitsets could come in handy...  TODO: Optimisation
	//
	bitset_t *cand_varsneeded = bitset_new (varsneeded->type);
	bitset_t *cand_generators = bitset_new (soln_generators->type);
	//
	// If there is no need for further variables to calculate this
	// condition, then terminate recursion on this function.  There
	// will be no requirement to include a generator, and so there
	// will be no generator-incurred expenses.  There will only be
	// the cost of the condition itself, which must still compare
	// with other conditions that might be tighter.
	//
	if (bitset_isempty (varsneeded)) {
		/* Recursion ends, so neither generators nor their cost */
		bitset_empty (out_generators);
		return cc->cost;
	}
	//
	// Iterate over the variables that will need to be generated.
	// For each, iterate over the generators producing it.
	// See how this reduces the variable need, and recurse.
	//
	v_max = bitset_max (varsneeded);
	for (v=0; v<v_max; v++) {
		//
		// Skip this loop iteration if v is not actually needed.
		//
		if (!bitset_test (varsneeded, v)) {
			continue;
		}
		//
		// Iterate over generators for this variable.
		// Note that there may be none left?
		//
		bitset_t *vargenerators;
		vargenerators = var_share_generators (cc->vartab, v);
		g_max = bitset_max (vargenerators);
		if (g_max = BITNUM_BAD) {
			// Apparently, there are no generators for v
			// Skip for-loop (and trouble looping to BITNUM_BAD).
			continue;
		}
		for (g=0; g<g_max; g++) {
			bitset_t *cand_generators = NULL;
			bitset_t *generatorvars;
			float cand_cost;
			//
			// Reduce the variable need.
			//
			cand_varsneeded = bitset_copy (varsneeded);
			generatorvars = gen_share_variables (v);
			bitset_subtract (cand_varsneeded, generatorvars);
			//
			// Determine the cost for generating the remainder.
			//
			if (!cnd_get_cost_total (
					cc, cand_varsneeded,
					&cand_cost, cand_generators)) {
				continue;
			}
			cand_mincost *= generator_cost (g);
			if (have_soln && (cand_cost > soln_mincost)) {
				continue;
			}
			tmp_generators = soln_generators;
			soln_generators = cand_generators;
			cand_generators = tmp_generators; // Reuse in loops
			bitset_set (soln_generators, g);
			soln_mincost = cand_mincost;
			have_soln = true;
		}
	}
	//
	// Cleanup temporary allocations.
	//
	bitset_destroy (cand_varsneeded);
	bitset_destroy (cand_generators);
	//
	// Collect results.  Return success only if we do indeed have
	// found a solution.
	//
	return have_soln;
}
#endif


#ifdef DEPRECATED_BITSET_ITERATIONS_SPREADING_CONTROL_ACCROSS_FUNCTIONS
struct iter_var_gen {
	bool okay;
	struct vartab *vartab;
	struct gentab *gentab;
	bitset_t *varneed;
	bitset_t *generate;
	float total_cost;
	float cheapest_gennum;
	float cheapest_gencost;
};

static void *var_cheap_generator_itgen (unsigned int gennr, void *data) {
	struct iter_var_gen *ivg = (struct iter_var_gen *) data;
	if (ivg->okay) {
		float gencost = gen_get_weight (gentab, gennr);
		if ((ivg->cheapest_gennum == GENNUM_BAD)
				|| (ivg->cheapest_gencost > gencost)) {
			ivg->cheapest_gennum = gennr;
			ivg->cheapest_gencost = gencost;
		}
	}
	return ivg;
}

static void *var_cheap_generator_itvar (unsigned int varnr, void *data) {
	struct iter_var_gen *ivg = (struct iter_var_gen *) data;
	if (ivg->okay) {
		bitset_t *vargens = var_share_generators (
					ivg->vartab, (varnum_t) bitnr);
		if (bitset_isempty (vargens)) {
			ivg->okay = false;
		}
		ivg->cheapest_gennum = GENNUM_BAD;
		bitset_iterate (vargens, var_cheap_generator_itgen, &ivg);
		if (ivg->cheapest_gennum == GENNUM_BAD) {
			ivg->okay = false;
		} else {
			ivg->total_cost *= ivg->cheapest_gencost;
			gennum_t = ivg->cheapest_gennum;
			bitset_t *gv = gen_share_variables (ivg->gentab, g);
			bitset_subtract (ivg->varneed, gv);
			bitset_set (ivg->generate, g);
		}
	}
	return ivg;
}

/* Make an estimate of the total cost of determining the given condition
 * with the given set of still-needed variables.  Return true on success,
 * and then also set soln_mincost with the cost of running all generators
 * and the condition, and set soln_generators (which must be allocated)
 * to the set of generators used to get to this point.
 */
bool DEPRECATED_cnd_estimate_total_cost (struct condition *cc,
				bitset_t *varsneeded,
				float *soln_mincost,
				bitset_t *soln_generators) {
	//
	// First, for all varsneeded, determine their cheapest generator
	//
	struct iter_var_gen ivg;
	ivg.okay = true;
	ivg.vartab = cc->vartab;
	ivg.total_cost = cc->weight;
	ivg.varneed = bitset_clone (varsneeded);
	ivg.generate = soln_generators;
	bitset_empty (ivg.generate);
	bitset_iterate (varsneeded, var_cheap_generator_it, &ivg);
	bitset_destroy (ivg.varneed);
	*soln_mincost = ivg.total_cost;
	return ivg.okay;
}
#endif



/* Make an estimate of the total cost of determining the given condition
 * with the given set of still-needed variables.  Return true on success,
 * and then also set soln_mincost with the cost of running all generators
 * and the condition, and set soln_generators (which must be allocated)
 * to the set of generators used to get to this point.
 *
 * This cost estimator assumes that vartab_analyse_cheapest_generators()
 * has already been run, and it only succeeds when all varsneeded have
 * a generator assigned to them (which should be caught out by that prior
 * phase, but it nonetheless returns an OK as a boolean).
 *
 * The soln_mincost represents the minimum cost estimation for setting up
 * all the variables.  The soln_generators provides the generators that
 * need to run before the varsneeded have been generated, ordered by
 * ascending weight.  The soln_generator_count provides the number of
 * entries filled into this table; the assumption made is that at least
 * as many entries are available as the number of elements in varsneeded.
 * Room up to this point may be overwritten by this routine, even if this
 * exceeds the impression given by a lower soln_generator_count.
 */
bool cnd_estimate_total_cost (struct cndtab *cctab, cndnum_t cndnum,
				bitset_t *varsneeded,
				float *soln_mincost,
				struct gentab *gentab,
				unsigned int *soln_generator_count,
				gennum_t soln_generators []) {
	bitset_iter_t v;
	bitset_t *vneeded;
	*soln_generator_count = 0;
	//
	// Find the cheapest generator for each of the varsneeded
	bitset_iterator_init (&v, varsneeded);
	while (bitset_iterator_next_one (&v, NULL)) {
		gennum_t unit_gen;
		bitset_t *genvars;
		unsigned int gi = bitset_iterator_bitnum (&v);
		//
		// The first step generates a list with the cheapest generators;
		// this could actually return an ordered _list_ of generators
		// and would cut off the most expensive ones when varsneeded
		// are generated as by-products in other generators.
		// This leads to more accurate estimates, and more efficiency.
		if (!var_get_cheapest_generator (
					vartab_from_type (cctab->vartype),
					gi,
					&unit_gen, NULL)) {
			return false;
		}
		soln_generators [*soln_generator_count++] = unit_gen;
	}
	//
	// Now sort the soln_generators by their weight.
	//
	// This is a sort for optimisation purposes; if it returns an error,
	// then the cause is normally memory shortage, in which case the
	// lost efficiency here is less vital than other things going awry.
	qsort_r (soln_generators,
			*soln_generator_count,
			sizeof (struct generator *),
			gentab,
			gen_cmp_weight);
	//
	// From the sorted soln_generators list, subtract generated variables
	// from varsneeded until all are done.  Cut off the output list at
	// that point by returning a possibly lower soln_generator_count.
	//
	//TODO// Might be able to skip generators that add nothing of interest
	*soln_generator_count = 0;
	*soln_mincost = cnd_get_weight (cctab, cndnum);
	vneeded = bitset_clone (varsneeded);
	while (!bitset_isempty (vneeded)) {
		gennum_t g = soln_generators [*soln_generator_count++];
		*soln_mincost *= gen_get_weight (gentab, g);
		bitset_t *genvars = gen_share_variables (gentab, g);
		bitset_subtract (vneeded, genvars);
	}
	bitset_destroy (vneeded);
	return true;
}

