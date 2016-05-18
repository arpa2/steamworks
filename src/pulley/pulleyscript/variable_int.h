
/* The data structure for an individual variable.  Note that the number is
 * vital for reference purposes; adding a variable may alter the address at
 * which it is stored.
 */
struct variable {
	char *name;
	varkind_t kind;		/* Is this a variable, constant, ...? */
	struct var_value value;	/* What is my type and value? */
	unsigned int partition;	/* Varpartition number for this variable */
	bitset_t *shared_varpartition; /* Owned where varnum==partition */
	bitset_t *generators;	/* Which refer/generate this variable? */
	bitset_t *conditions;	/* Which constrain this variable? */
	bitset_t *driversout;	/* Which drive out this variable? */
	gennum_t cheapest_generator;  /* The lowest-cost generator */
};


/* The variable table holding all variables (in an array, so direct access
 * is ill-advised).
 */
struct vartab {
	type_t vartype;
	type_t *gentype, *condtype, *drvtype;
	struct variable *vars;
	unsigned int allocated_vars;
	unsigned int count_vars;
};

