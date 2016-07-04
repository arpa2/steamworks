/* compiler.c for Pulley Scripts
 *
 * This is a demo compiler program to load files and other sources into
 * the Pulley Script language, and extract ever more structural knowledge
 * from it.  This demonstrates the pattern that is needed to process
 * Pulley scripts into Paths of Least Resistence that can be executed at
 * run time.
 *
 * Use this program if you need a dump of the analysis results, including
 * intermediate semantics gathered, from any given Pulley script.  The test
 * compiler will drown you in information.  Do not, however, distribute this
 * program in packages.  Instead, copy the principles in your own code and
 * adapt where needed.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "parser.h"
#include "lexhash.h"
#include "variable.h"
#include "generator.h"
#include "condition.h"
#include "driver.h"
#include "squeal.h"


void print_status (struct parser *prs, char *status) {
	vartab_print (prs->vartab, status, stdout, 0);
	gentab_print (prs->gentab, status, stdout, 0);
	cndtab_print (prs->cndtab, status, stdout, 0);
	drvtab_print (prs->drvtab, status, stdout, 0);
}

int collect_input (struct parser *prs, int argc, char *argv []) {
	int prsret = 0;
	FILE *fh;
	hash_t compiled_hash;
	int i;

	if (argc <= 1) {
		fprintf (stderr, "Usage: %s filenames...\n", argv [0]);
		return 1;
	}

	for (i=1; i<argc; i++) {
		if (strcmp (argv [i], "-") == 0) {
			char line [257];
			int linelen;
			// Read a line from stdin
			fprintf (stdout, "pulley> ");
			fflush (stdout);
			if (fgets (line, 256, stdin) == NULL) {
				fprintf (stderr, "Failed to read your input\n");
				prsret = 1;
			} else {
				linelen = strlen (line);
				if ((linelen > 0) && (line [linelen-1] == '\n')) {
					line [--linelen] = '\0';
				}
				prsret = pulley_parser_buffer (prs, PARSER_LINE, line, linelen);
			}
		} else {
			// Normal input file
			fh = fopen (argv [i], "r");
			if (!fh) {
				fprintf (stderr, "Failed to open %s\n", argv [i]);
				return 1;
			}
			printf ("Loading %s\n", argv [i]);
			prsret = pulley_parser_file (prs, fh);
			fclose (fh);
		}
		if (prsret != 0) {
			fprintf (stderr, "Parser returned error %d\n", prsret);
			break;
		}
	}

	pulley_parser_hash (prs);
	printf ("Complete source hash: ");
	hash_print (&prs->scanhash, stdout);
	printf ("\n");

	pulley_parser_cleanup_syntax (prs);

	return prsret;
}

int structural_analysis (struct parser *prs) {
	int prsret = 0;

	// Use conditions to drive variable partitions;
	// collect shared_varpartitions for them.
	// This enables
	//  - var_vars2partitions()
	//  - var_partitions2vars()
	cndtab_drive_partitions (prs->cndtab);
	vartab_collect_varpartitions (prs->vartab);

	// Collect the things a driver needs for its work
	drvtab_collect_varpartitions (prs->drvtab);
	drvtab_collect_conditions (prs->drvtab);
	drvtab_collect_generators (prs->drvtab);
	drvtab_collect_cogenerators (prs->drvtab);
	drvtab_collect_genvariables (prs->drvtab);
	drvtab_collect_guards (prs->drvtab);

	// Collect the things a generator needs for its work.
	// This enables
	//  - gen_share_driverouts()	XXX already done
	//  - gen_share_conditions()	XXX never used (yet)
	//  - gen_share_generators()	XXX never used (yet)
	//NONEED// gen_push_driverout() => gen_share_driverouts()
	//NONEED// gen_push_condition() => gen_share_conditions()
	//NONEED// gen_push_generator() => gen_share_generators()

	return prsret;
}


/* Check if the structure of the complete program is consistent:
 *  - "must have at least one generator binding each variable"
 *  - "must have at least one generator binding each variable"
 *  - "must have at least one variable in each condition"
 *  - "must have at least one variable in each driverout"
 */
int structural_consistency (struct parser *prs) {
	int prsret = 0;
	bitset_t *errors;

	// "must have at least one generator binding each variable"
	errors = vartab_unbound_variables (prs->vartab);
	if (errors != NULL) {
		if (!bitset_isempty (errors)) {
			fprintf (stderr, "Structural inconsistency: Variables must be bound by at least one generator:\n");
			vartab_print_names (prs->vartab, errors, stderr);
			fprintf (stderr, "\n");
			prsret = 1;
		}
		bitset_destroy (errors);
	}

	// "must have precisely one generator binding each variable"
	errors = vartab_multibound_variables (prs->vartab);
	if (errors != NULL) {
		if (!bitset_isempty (errors)) {
			fprintf (stderr, "Structural inconsistency: Variables must be bound by at most one generator:\n");
			vartab_print_names (prs->vartab, errors, stderr);
			fprintf (stderr, "\n");
			prsret = 1;
		}
		bitset_destroy (errors);
	}

	// "must have at least one variable in each condition"
	errors = cndtab_invariant_conditions (prs->cndtab);
	if (errors != NULL) {
		if (!bitset_isempty (errors)) {
			fprintf (stderr, "Structural inconsistency: Conditions must refer to at least one variable\n");
			bitset_print (errors, "C", stderr);
			fprintf (stderr, "\n");
			prsret = 1;
		}
		bitset_destroy (errors);
	}

	// "must have at least one variable in each driverout"
	errors = drvtab_invariant_driverouts (prs->drvtab);
	if (errors != NULL) {
		if (!bitset_isempty (errors)) {
			fprintf (stderr, "Structural inconsistency: Drivers must output at least one variable\n");
			bitset_print (errors, "D", stderr);
			fprintf (stderr, "\n");
			prsret = 1;
		}
		bitset_destroy (errors);
	}

	return prsret;
}

/* Analyse the cost of the individual generators and conditions, and perhaps
 * of driver outputs.  This will be used to construct Paths of Least Resistence
 * which are the actual outputs of the compiler.  These paths are named to
 * reflect that they are supposed to have a near-optimal order.
 */
int cost_analysis (struct parser *prs) {
	int prsret = 0;
	// vartab_analyse_cheapest_generators() enables various functions:
	//  - var_get_cheapest_generator()
	//  - cnd_estimate_total_cost()
	prsret = vartab_analyse_cheapest_generators (prs->vartab);
	if (prsret) {
		fprintf (stderr, "Not all variables have a generator attached to them\n");
	}

	return prsret;
}


/* Generate Paths of Least Resistence.
 *
 * Paths of Least Resistence are recipes for the calculation of all variables
 * that may need to be output by a driver.
 *
 * Normally, variable values are constructed in push mode, where a generator's
 * addition or retraction of a "fork" leads to the iteration of other variables
 * and submission of data to all impacted drivers.  In this mode, the variables
 * from the generator are taken as a given, and the rest is appended and
 * subjected to conditions until it can be driven out.
 *
 * The alternate mode is pull mode, where an output driver makes an enquiry
 * whether a given combination of output variables could occur.  This starts
 * from only those variables that count as output, running all the generators
 * and conditions that could constrain it.  The result in pull mode is a series
 * of callbacks to the driver, or one if only a yes/no response is sought.
 * TODO: Decide whether output drivers can query with part of their outputs.
 *
 * Path generation is normally done lazily, because not all paths may actually
 * be required, and also because at startup time there may be more pressing
 * matters thant his relatively expensive process.
 * In this demonstration compiler however, we force the generation of all
 * Paths of Least Resistence so see each appear in our semantics dumps.
 */
int generate_paths (struct parser *prs) {
	int prsret = 0;
	//TODO// - path_have()		[driver-pull counterpart?]
	return prsret;
}


/* Generate code through the squeal engine.  This constructs SQL code to be run on
 * SQLite3 to implement concepts like co-generator iteration.
 */
int generate_squeal (struct parser *prs) {
	int TODO_produce_outputs (struct drvtab *drvtab, gennum_t gennum, drvnum_t drvnum);
	struct squeal *s3db;

	s3db = squeal_open(prs->scanhash, gentab_count (prs->gentab), drvtab_count (prs->drvtab));
	assert (s3db != NULL);
	gennum_t g, gentabcnt = gentab_count (prs->gentab);
	bitset_t *drv;
	bitset_iter_t di;
	drvnum_t d;
	assert (squeal_have_tables (s3db, prs->gentab, 0) == 0);
	for (g=0; g<gentabcnt; g++) {
		drv = gen_share_driverout (prs->gentab, g);
		printf ("Found %d drivers for generator %d\n", bitset_count (drv), g);
		bitset_iterator_init (&di, drv);
		while (bitset_iterator_next_one (&di, NULL)) {
			d = bitset_iterator_bitnum (&di);
			printf ("Generating for generator %d, driver %d\n", g, d);
			assert (TODO_produce_outputs (prs->drvtab, g, d) == 0);
		}
	}
	squeal_close (s3db);
	squeal_unlink (prs->scanhash);
	s3db = NULL;
	return 0;
}


/* The main program of the compiler passes a number of sources into the
 * parser, to collect semantical structures from it.  These end up in
 * vartab/gentab/cndtab/drvtab variables under the parser structure.
 *
 * The sources may be filenames entered as commandline arguments, or
 * interactively read lines for arguments "-".  All are composed, in an
 * order that does not matter.
 *
 * All sources are hashed together; a change in syntax or order of sources
 * does not impact the hash.  The use of this is to save costly repeated
 * analysis.  This is not implemented here, because it serves tests to
 * always run the remainder of the compilation process.
 *
 * The compiler then proceeds with the following phases, each of which
 * are spelled out in separate functions:
 *  - structural analysis, passing around semantical values in sets
 *  - structural consistency, checking if silly structures span the source
 *  - cost analysis, determining how the cost of each condition and generator
 *  - path generation, forced for test purposes, followed by printing
 *
 * The Paths of Least Resistence constitute the output of the compiler.
 * Each Path of Least Resistence is a recipe to construct additional work
 * from either a generator's addition or removal of a fork, or from a driver's
 * pull request.
 * The work done by a Path of Least Resistence constitutes applying generators,
 * testing conditions and eventually driving output.  Driven output is
 * differential, meaning that information sent out is an addition or removal.
 *
 * The differences driven towards an output is unique, as long as the total
 * of the output variables and guards are taken into account.  It is possible
 * to summerise the guards into something like a (secure) hash, but when
 * ignoring guards there may be multiple occurrences of the same output
 * variables driven.  This could lead to five additions that are all erased
 * on account of one removal, so the driver output would merely suffice as
 * a hint that the said output may have been deleted.  When taking guards
 * into account, even if just counting them and not storing them, then the
 * outputs driven are indeed... accountable :)
 */
int main (int argc, char *argv []) {
	struct parser prs;
	int prsret = 0;

	pulley_parser_init (&prs);

	prsret = collect_input (&prs, argc, argv);

	print_status (&prs, "after parsing");

	if (prsret == 0) {
		prsret = structural_analysis (&prs);
		print_status (&prs, "after structural analysis");
	}

#if 0
	if (prsret == 0) {
		prsret = structural_consistency (&prs);
	}
#endif

#ifdef TODO_PATH_OF_LEAST_RESISTENCE_APPROACH
	if (prsret == 0) {
		prsret = cost_analysis (&prs);
		print_status (&prs, "after cost analysis");
	}

	if (prsret == 0) {
		prsret = generate_paths (&prs);
		print_status (&prs, "after generating paths of least resistence");
	}
#else
	if (prsret == 0) {
		prsret = generate_squeal (&prs);
	}
#endif

	pulley_parser_cleanup_semantics (&prs);

	printf ("Exit value %d\n", prsret? 1: 0);
	exit (prsret? 1: 0);
}

