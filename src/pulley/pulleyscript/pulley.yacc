
%{

#include <assert.h>

#include "parser.h"
#include "variable.h"
#include "condition.h"
#include "generator.h"
#include "driver.h"
#include "binding.h"

%}

%union {
	int intval;
	float fltval;
	char *strval;
	struct {
		size_t len;
		uint8_t *data;
	} binval;
	varnum_t varnum;
}

%{

// int yylex (YYSTYPE *yylval_param, yyscan_t yyscanner);
void yyerror (struct parser *,char *);
#define YYLEX_PARAM &yylval, prs->lexer

%}

%token NEWLINE INDENT COMMENT
%token GEN_FROM DRIVE_TO
%token OPEN CLOSE BRA KET
%token STAR PLUS COMMA COLON AT
%token CMP_EQ CMP_NE CMP_LT CMP_LE CMP_GT CMP_GE
%token LOG_NOT LOG_AND LOG_OR
%token ATTRTYPE ATTRNAME ATTROPT VARIABLE SILENCER DRIVERNAME PARAMETER
%token STRING BLOB INTEGER FLOAT

%type<varnum> INTEGER FLOAT STRING BLOB DRIVERNAME ATTRTYPE
%type<varnum> VARIABLE PARAMETER optvalue value const varnm dnvar bindrdnpart
%type<intval> filtcmp

%left COMMA

/* The parser is made re-entrant, with parameters for the various tables
 * that will store the semantics for later analysis.
 * Kludge: Bison seems to extract the last identifier for lex-param only,
 * so we've defined one to take that place
 */
%pure-parser
%lex-param { YYSTYPE *yylval_param, yyscan_t prs_arrow_scanner }
//HUH?// %define api.puri
%parse-param { struct parser *prs }

%{
#define prs_arrow_scanner prs->lexer
%}

/* Define a switching start symbol, and hav yylex() prefix the input stream
 * with a suitable start condition switch; this makes for flexibility in the
 * syntax scanned with bison.  The global bison_switch directs yylex() to
 * deliver these special starting tags.  The syntax below requires it, but
 * will only accept it at the beginning of the parsing process.
 */
%start switch
%token START_SCRIPT START_LINE START_GENERATOR START_CONDITION START_DRIVEROUT

%verbose

%{

// The following code implements a stack, together with a virtual production
// rule _pushV that does the opposite of _popV.
//
// This may not be the best option; we might also return bitsets of variables
// from the various non-terminals in the grammar, set their %type and use $i.
// The vital matter in that case is allocation and reuse of the variable sets.

// _tosV is the top of the variable stack
bitset_t *_tosV (struct parser *prs) {
	if (prs->varstack_sp < 0) {
		fatal_error ("Cannot share top of empty stack");
	}
	return prs->varstack [prs->varstack_sp];
}

void _pushV (struct parser *prs) {
	prs->varstack_sp++;
	if (prs->varstack_sp >= PARSER_VARIABLE_STACK_ENTRIES) {
		fatal_error ("Parser ran out of variable stack levels");
	}
	bitset_empty (prs->varstack [prs->varstack_sp]);
}

// _popV retrieves the top element of the variable stack
bitset_t *_popV (struct parser *prs) {
	if (prs->varstack_sp <= 0) {
		fatal_error ("Attempt to pop top element of variable stack");
	}
	return prs->varstack [prs->varstack_sp--];
}

// _clrV wipes the variable stack
void _clrV (struct parser *prs) {
	prs->varstack_sp = 0;
	bitset_empty (prs->varstack [0]);
}

// The following code implements a reverse bufffer, in which binding actions
// variable/constant numbers are collected.  This is done before allocating
// memory for them and setting it up in a variable.  The buffer starts with
// the BNDO_ACT_DONE installed and grows towards the beginning, minding its
// maximum size while at it.  After it is cloned into the variable space,
// the buffer is reset to just BNDO_ACT_DONE.

// _actO prefixes an action code to the buffer
void _actO (struct parser *prs, uint8_t action) {
	varnum_t rematch = VARNUM_BAD;
	size_t here = prs->action_sp;
	//TODO// Throw parser error instead
	assert (here >= sizeof (action));
	here--;
	prs->action_sp = here;
	prs->action [here] = action;
	// Same variable bound in follow-up?  Then make that a comparison!
	if ((action & BNDO_ACT_MASK) == BNDO_ACT_BIND) {
		here += 1 + sizeof (varnum_t);
		rematch = * (varnum_t *) &prs->action [here];
		here += 0 + sizeof (varnum_t);
		while (here < sizeof (prs->action)) {
			switch (prs->action [here] & BNDO_ACT_MASK) {
			case BNDO_ACT_DOWN:
			case BNDO_ACT_OBJECT:
			case BNDO_ACT_DONE:
				// Uses no parameters
				here += 1;
				break;
			case BNDO_ACT_HAVE:
				// Only uses $1
				here += 1 + sizeof (varnum_t);
				break;
			case BNDO_ACT_BIND:
				// Uses both $1 and $2
				if (rematch == * (varnum_t *) &prs->action [here + 1 + sizeof (varnum_t)]) {
					// BIND is overruled by the new one
					prs->action [here] &= ~BNDO_ACT_MASK;
					prs->action [here] |=  BNDO_ACT_CMP;
					// There will not be any more of these
					here = sizeof (prs->action);
					break;
				}
				// ...continue into BNDO_ACT_CMP...
			case BNDO_ACT_CMP:
				// Uses both $1 and $2
				here += 1 + 2 * sizeof (varnum_t);
				break;
			}
		}
	}
}

// _varO stores a variable number before the rest of the buffer.
// The variable number represents either a constant or a variable.
void _varO (struct parser *prs, varnum_t var) {
	size_t actionbuf_freespace = prs->action_sp;
	//TODO// Throw parser error instead
	assert (actionbuf_freespace >= sizeof (var));
	prs->action_sp -= sizeof (varnum_t);
	* (varnum_t *) &prs->action [prs->action_sp] = var;
}

// _clrO clears the action buffer.
void _clrO (struct parser *prs) {
	prs->action_sp = sizeof (prs->action) -1;
}

// Stores the action buffer as a variable called @p bndname,
// using the struct @p varval for intermediate storage.
// Returns the variable number of the stored action buffer.
varnum_t _store_binding (struct parser *prs, char *bndname, struct var_value *varval) {
	varnum_t binding = var_have (prs->vartab, bndname, VARKIND_BINDING);
	varval->type = VARTP_BLOB;
	varval->typed_blob.len = sizeof (prs->action) - prs->action_sp;
	varval->typed_blob.str = malloc (varval->typed_blob.len);
	assert (varval->typed_blob.str != NULL);
	memcpy (varval->typed_blob.str, &prs->action [prs->action_sp],
			varval->typed_blob.len);
	var_set_value (prs->vartab, binding, varval);

	return binding;
}

// _flushO stores the action buffer as bnd_<linehash>
void _flushO (struct parser *prs, hash_t linehash, gennum_t g) {
	char bndname [4 + 2 * sizeof (hash_t) + 1];
	struct var_value varval;
	varnum_t binding;
	int len = sizeof (hash_t);
	uint8_t *ptr = (uint8_t *) &linehash;
	memcpy (bndname, "bnd_", 4);
	uint8_t *hexstr = bndname + 4;
	while (len-- > 0) {
		sprintf (hexstr, "%02x", *ptr++);
		hexstr += 2;
	}
	binding = _store_binding (prs, bndname, &varval);
//DEBUG//
printf ("Binding %s V%d G%d created: >>> ", bndname, binding, g);
ptr = varval.typed_blob.str;
len = varval.typed_blob.len;
while (len-- > 0) {
	printf ("%02x ", *ptr++);
}
printf ("<<<\n");

	gen_set_binding(prs->gentab, g, binding);
}

%}

%%

/* The definition of a (fake) construct that pushes that variable stack.
 * This is used to separate contexts of variables within a line.  The new
 * top-of-stack accumulates new variables, and starts empty after _pushV().
 */
_pushV:	{ _pushV (prs); }

/* When initiated by the pulley_parser_XXX() functions, they will be setup
 * to parse either a single line, possibly even of a particular type, or a
 * sequence of lines informally known as a script.  The switch implements
 * this behaviour, in unison with a bit of startup code in the lexer that
 * inserts the special START_xxx symbols at the head of the lexeme stream.
 */
switch: START_SCRIPT script
switch: START_LINE line
switch: START_GENERATOR line_generator
switch: START_CONDITION line_condition
switch: START_DRIVEROUT line_driverout

script: /* %empty */
script: script line NEWLINE
script: script error NEWLINE

line: line_generator | line_condition | line_driverout | line_empty

/* Empty lines constitute acceptable grammar withing scripts, or if the line
 * type has not been specified.  It does not make any semantical modifications.
 */
line_empty:

/* Generator lines contain variables that are bound to the environment, they
 * extract information from a DN-type variable and they have annotations that
 * can define guard variables and a line weight.
 */
line_generator: error GEN_FROM dnvar annotations { _clrV (prs); _clrO (prs); }
line_generator: binding GEN_FROM _pushV dnvar annotations {
	bitset_t *guards = _popV (prs);
	varnum_t dnvar = $4;
	bitset_t *bindvars = _tosV (prs);
	gennum_t gennew = gen_new (prs->gentab, dnvar);
	bitset_iter_t lvi;
	hash_t linehash;
printf ("FOUND %d bound variables in generator line G%d\n", bitset_count (bindvars), gennew);
	bitset_iterator_init (&lvi, bindvars);
	while (bitset_iterator_next_one (&lvi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&lvi);
		var_used_in_generator (prs->vartab, varnum, gennew);
		gen_add_variable (prs->gentab, gennew, varnum);
	}
	if (! bitset_isempty (guards)) {
		//TODO// Process guards (but first discover what they mean!)
		fprintf (stderr, "WARNING: Ignoring explicit guards to generator, unsure about semantics...\n");
	}
	gen_set_weight (prs->gentab, gennew, prs->weight_set? prs->weight: 250.0);
	hash_curline (&prs->scanhashbuf, &linehash);
	gen_set_hash (prs->gentab, gennew, linehash);
	_flushO (prs, linehash, gennew);
	_clrV (prs);
	_clrO (prs);
}

/* Condition lines simply mention a filter and annotations.  As part of their
 * most valuable impact on semantics, they cause variable partitions; but
 * this is not currently worked out.  Instead, sets of variables used in
 * conditions are setup for later processing with cndtab_drive_partitions().
 * Annotations may set a weight and could introduce more variables as guards
 * that will also be taken into account during partitioning.
 */
line_condition: filter annotations {
	bitset_t *filtervars = _tosV (prs);	// Includes explicit guards!
	bitset_iter_t fvi;
	bitset_iterator_init (&fvi, filtervars);
	hash_t linehash;
	while (bitset_iterator_next_one (&fvi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&fvi);
		var_used_in_condition (prs->vartab, varnum, prs->newcnd);
		cnd_needvar (prs->cndtab, prs->newcnd, varnum);
	}
	cnd_set_weight (prs->cndtab, prs->newcnd, prs->weight_set? prs->weight: 0.02);
	hash_curline (&prs->scanhashbuf, &linehash);
	cnd_set_hash (prs->cndtab, prs->newcnd, linehash);
	_clrV (prs);
	prs->newcnd = CNDNUM_BAD;
}

/* Driver output lines specify a number of variables that will be sent out,
 * they name an output driver module as well as constant settings for named
 * parameters that this driver module may expect.  The constant settings are
 * provided while configuring the driver module; the variables will be supplied
 * when actually transmitting data through the driver module.
 * Processing of the line's information is done as soon as it is recognised,
 * to save internal structures beyond the sets that we wish to maintain.
 * Although a weight could be defined, it hardly has an impact on ordering;
 * although guards could be specified, it barely seems interesting to do so.
 */
//USELESS???// line_driveout: error DRIVE_TO DRIVERNAME OPEN parmlist CLOSE annotations { _clrV (); }
//USELESS???// line_driveout: error CLOSE annotations { _clrV (); }
line_driverout: drvout_vallist_s DRIVE_TO {
	bitset_t *varout = _tosV (prs);
	bitset_iter_t voi;
	bitset_iterator_init (&voi, varout);
	while (bitset_iterator_next_one (&voi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&voi);
	}
	_clrV (prs);
} DRIVERNAME {
	char *module = var_get_name (prs->vartab, $4);
	drv_set_module (prs->drvtab, prs->newdrv, module);
	bitset_t *driver = _tosV (prs);
	_clrV (prs);
	_clrO (prs);
} OPEN parmlist CLOSE {
	char bndname [7 + 2 * sizeof (hash_t) + 1];
	struct var_value varval;
	varnum_t binding;
	snprintf (bndname, sizeof(bndname), "bnd_DRV%d", prs->newdrv);
	binding = _store_binding (prs, bndname, &varval);
	drv_set_module_parameters (prs->drvtab, prs->newdrv, binding);
	bitset_t *params = _tosV (prs);
	_clrV (prs);
} annotations {
	bitset_t *guards = _tosV (prs);
	bitset_iter_t gvi;
	bitset_iterator_init (&gvi, guards);
	while (bitset_iterator_next_one (&gvi, NULL)) {
		varnum_t varnum = bitset_iterator_bitnum (&gvi);
		var_used_in_driverout (prs->vartab, varnum, prs->newdrv);
		drv_add_guardvar (prs->drvtab, prs->newdrv, varnum);
	}
	drv_set_weight (prs->drvtab, prs->newdrv, prs->weight_set? prs->weight: 3.0);
	hash_t linehash;
	hash_curline (&prs->scanhashbuf, &linehash);
	drv_set_hash (prs->drvtab, prs->newdrv, linehash);
	_clrV (prs);
	prs->newdrv = DRVNUM_BAD;
}

annotations: annotation1 annotation2
annotation1: /* %empty */
annotation1: BRA varlist KET
annotation2: /* %empty */ { prs->weight_set = false; }
annotation2: STAR FLOAT { prs->weight_set = true; prs->weight = /*TODO*/ 0.1; }

binding: bindatr { _actO (prs, BNDO_ACT_OBJECT); }
binding: bindatr { _actO (prs, BNDO_ACT_OBJECT); } COMMA bindsteps
binding: bindsteps
bindsteps: bindstep
bindsteps: bindsteps COMMA bindstep
bindstep: AT dnvar {
	bitset_set (_tosV (prs), $2);
	_varO (prs, $2);
	_varO (prs, VARNUM_BAD);
	_actO (prs, BNDO_SUBJ_DN | BNDO_ACT_BIND);
	// We do not move down at this point, we just harvest the DN
}
bindstep: bindrdn {
	_actO (prs, BNDO_ACT_DOWN);
}
bindrdn: bindrdnpart
bindrdn: bindrdn PLUS bindrdnpart
bindrdnpart: ATTRTYPE CMP_EQ optvalue {
	//TODO// Recognise special names, like DCList
	//TODO// Alternatively, append dnvarmods ::= ? | + | *
	if ($3 == VARNUM_BAD) {
		// SILENCER means just require having $1
		_varO (prs, $1);
		_actO (prs, BNDO_SUBJ_RDN | BNDO_ACT_HAVE);
	} else {
		uint8_t action;
		 if (var_get_kind (prs->vartab, $3) == VARKIND_CONSTANT) {
			action = BNDO_SUBJ_RDN | BNDO_ACT_CMP;
		} else {
			action = BNDO_SUBJ_RDN | BNDO_ACT_BIND;
		}
		_varO (prs, $3);
		_varO (prs, $1);
		_actO (prs, action);
		bitset_set (_tosV (prs), $3);
	}
}
bindatr: atmatch
bindatr: bindatr PLUS atmatch
atmatch: ATTRTYPE COLON optvalue {
	if ($3 == VARNUM_BAD) {
		// SILENCER means just require having $1
		_varO (prs, $1);
		_actO (prs, BNDO_SUBJ_ATTR | BNDO_ACT_HAVE);
	} else {
		uint8_t action;
		if (var_get_kind (prs->vartab, $3) == VARKIND_CONSTANT) {
			action = BNDO_SUBJ_ATTR | BNDO_ACT_CMP;
		} else {
			action = BNDO_SUBJ_ATTR | BNDO_ACT_BIND;
		}
		_varO (prs, $3);
		_varO (prs, $1);
		_actO (prs, action);
		bitset_set (_tosV (prs), $3);
	}
}

filter: OPEN {
	if (prs->newcnd == CNDNUM_BAD) {
		prs->newcnd = cnd_new (prs->cndtab);
	}
} filtbody CLOSE
filter: OPEN error CLOSE
filtbody: filtbase
filtbody: LOG_NOT filter {
	cnd_pushop (prs->cndtab, prs->newcnd, CND_NOT);
}
filtbody: LOG_AND {
	cnd_pushop (prs->cndtab, prs->newcnd, CND_SEQ_START);
} filters {
	cnd_pushop (prs->cndtab, prs->newcnd, CND_AND);
}
filtbody: LOG_OR  {
	cnd_pushop (prs->cndtab, prs->newcnd, CND_SEQ_START);
} filters {
	cnd_pushop (prs->cndtab, prs->newcnd, CND_OR);
}
filters: /* %empty */
filters: filters filter
filtcmp:  CMP_EQ { $$ = CND_EQ; }
	| CMP_NE { $$ = CND_NE; }
	| CMP_LT { $$ = CND_LT; }
	| CMP_LE { $$ = CND_LE; }
	| CMP_GT { $$ = CND_GT; }
	| CMP_GE { $$ = CND_GE; }
filtbase: varnm filtcmp value {
	var_used_in_condition (prs->vartab, $1, prs->newcnd);
	var_used_in_condition (prs->vartab, $3, prs->newcnd);
	cnd_needvar (prs->cndtab, prs->newcnd, $1);
	cnd_needvar (prs->cndtab, prs->newcnd, $3);
	/* TODO: Type checking? Operator applicability checking? */
	cnd_pushvar (prs->cndtab, prs->newcnd, $1);
	cnd_pushvar (prs->cndtab, prs->newcnd, $3);
	cnd_pushop  (prs->cndtab, prs->newcnd, $2);
}

// value_s: value
// value_s: BRA valuelist KET
// valuelist: value
// valuelist: valuelist COMMA value
optvalue: value { $$ = $1; }
optvalue: SILENCER { $$ = VARNUM_BAD; }
value: varnm | const
dnvar: varnm { $$ = $1; }
drvout_vallist_s: drvout_vallist_s COMMA drvout_vallist_s
drvout_vallist_s: value {
	if (prs->newdrv == DRVNUM_BAD) {
		prs->newdrv = drv_new (prs->drvtab);
	}
	drv_output_variable (prs->drvtab, prs->newdrv, $1);
	var_used_in_driverout (prs->vartab, $1, prs->newdrv);
}
drvout_vallist_s: BRA {
	drv_output_list_begin (prs->drvtab, prs->newdrv);
} drvout_vallist KET {
	drv_output_list_end (prs->drvtab, prs->newdrv);
}
drvout_vallist: drvout_vallist COMMA drvout_vallist
drvout_vallist: value {
	if (prs->newdrv == DRVNUM_BAD) {
		prs->newdrv = drv_new (prs->drvtab);
	}
	drv_output_variable (prs->drvtab, prs->newdrv, $1);
	var_used_in_driverout (prs->vartab, $1, prs->newdrv);
}
varlist: varnm
varlist: varlist COMMA varnm
varnm: VARIABLE
const: STRING | INTEGER | FLOAT | BLOB
parmlist: parmlist COMMA parm
parmlist: parm
parm: PARAMETER CMP_EQ const {
	_varO (prs, $3);
	_varO (prs, $1);
	_actO (prs, BNDO_ACT_CMP);  /* Not really a comparison, but assignment */
}

%%

