
struct gentab {
	type_t gentype;
	type_t *vartype, *drvtype;
	struct generator *gens;
	unsigned int allocated_gens;
	unsigned int count_gens;
	
};
