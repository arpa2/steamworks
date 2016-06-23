%option reentrant
%option extra-type="struct parser *"
%option bison-bridge

%{

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "pulley.tab.cacc"

#include "lexhash.h"
#include "variable.h"
#include "binding.h"


#define YY_DECL int yylex (YYSTYPE *yylval_param, yyscan_t yyscanner)


/* The fatal error routine can be called from here as well as other modules.
 */
#define YY_FATAL_ERROR(msg) fatal_error (msg)
void fatal_error (const char *msg) {
	fprintf (stderr, "%s\n", msg);
	exit (1);
}
void yyerror (struct parser *prs, char *msg) {
	fprintf (stderr, "%s\n", msg);
}

%}

/* Define an exclusive start condition for use inside attribute types.
 * This is used to detect the progressing with attribute options after
 * the name portion of an attribute type.
 *
 * This start condition is exclusive (%x) to indicate that the lexemes
 * without a start condition are disabled while it is active.  This is
 * used to cause lexical errors on all forms that are not explicitly
 * matched; we need this to ensure that the start condition terminates
 * immediately after it has been started.
 */
%x ATTRTP

/* Define an inclusive start condition for use in output driver lines.
 * In these, an identifier followed by an equals sign is a parameter,
 * and an identifier followed by an opening bracket is a drivername.
 *
 * This state is initiated when scanning the -> symbol, and it lasts
 * until the end of the line.
 */
%s DRIVER

/* Shorthand notations for later use.  Note that attribute type names have
 * their initial character converted from lowercase to uppercase, to aid
 * readability of LDAP attributes.  Variables have names that start with
 * lowercase characters or an underscore.  The special name comprising of
 * just an underscore is treated as a special case; it marks the variable
 * that does not connect to its other occurrences.
 */
NEWLINE		[\r\n]+
WHSPACE		[ \t]+
INDENT		^{WHSPACE}
IDENTIFIER	[a-z_][a-zA-Z0-9_]*
ATTRNAME	[A-Z][a-zA-Z0-9_]*
ATTROPT		[a-zA-Z0-9-]+
INTEGER		[-+]?([1-9][0-9]*|0[0-7]+|0[xX][0-9a-fA-F]+)
FLOAT		[-+]?[0-9]+(\.[0-9]+)?([eE][-+][0-9]+)?
HEXBYTE		[0-9a-fA-F]{2}
BLOB		#{HEXBYTE}+
STRING		\"[^\"\\\r\n]*(\\[^\r\n][^\"\\\r\n]*)*\"

%%

%{
	/* Declare intermediate storage while collecting attribute options.
	 */
	struct list_attropt **insert_attropt_here = NULL;

	/* Create a few direct pointers into struct parser *prs data
	 */
	struct parser *prs = yyextra;
	int bison_switch = prs->bison_switch;
	hashbuf_t *hbuf = &prs->scanhashbuf;

	/* Return a startup symbol to bison, to facilitate the switch rule
	 * that leads it to dynamically altered start symbols.  Bison requires
	 * such a symbol at the beginning of a grammar, and rejects it in all
	 * other places.
	 *
	 * This code is included in the top of yylex(), so it runs before
	 * the lexeme recognition code.
	 */
	if (bison_switch != 0) {
		prs->bison_switch = 0;
		return bison_switch;
	}

%}

^#.*$		/* Skip, and do not hash */
^{WHSPACE}$	/* Skip, and do not hash */
^{WHSPACE}.	{
			// When actually processed, hash the indent, not yytext
			hash_token_text (hbuf, INDENT, yytext);
			yyless (yyleng-1);	// Rescan the '.' character
			return INDENT;
		}
{NEWLINE}	{
			hash_endline (hbuf);
			BEGIN(INITIAL);		// Normal mode
			return NEWLINE;
		}
{WHSPACE}	/* Skip other whitespace in the lexer; do not hash it */

<DRIVER>{IDENTIFIER}/{WHSPACE}?\=	{
			/* Note that driver parameters are shared; they do not
			 * have a value themselves, but merely serve to signal
			 * the names to the drivers.  The actual values are
			 * stored in variables that are passed to the driver
			 * during initialisation, and they can be stored in a
			 * static manner; their values will change but their
			 * addresses won't.  And even those may be shared.
			 */
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_PARAMETER);
			hash_token_text (hbuf, PARAMETER, yytext);
			return PARAMETER;
		}

<DRIVER>{IDENTIFIER}/{WHSPACE}?\(	{
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_DRIVERNAME);
			hash_token_text (hbuf, DRIVERNAME, yytext);
			return DRIVERNAME;
		}

{IDENTIFIER}	{
			if ((yyleng == 1) & (*yytext == '_')) {
				return SILENCER;
			}
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_VARIABLE);
			/* Ideally, consistent identifier renaming has no
			 * impact on hashing; but doing something like
			 * numbering them would make hashing dependent on line
			 * order, with a rather serious impact on the number
			 * of data cache flushes, and their predictability.
			 * The only way out would be to describe structures
			 * that are spun by the variable's connections, but
			 * that is too complex for this simple function.
			 */
			hash_token_text (hbuf, VARIABLE, yytext);
			return VARIABLE;
		}

{ATTRNAME}	{
			struct var_value val;
			val.type = VARTP_ATTROPTS;
			val.typed_attropts = NULL;
			insert_attropt_here = &val.typed_attropts;
			// Don't share ATTROPT, so use var_add(), not var_have()
			yylval->varnum = var_add (prs->vartab, yytext, VARKIND_ATTRTYPE);
			hash_token_text (hbuf, ATTRNAME, yytext);
			BEGIN(ATTRTP);
			// Continue scanning, pass insert_attropt_here along
		}
<ATTRTP>[^;]		{
				insert_attropt_here = NULL;
				yyless (yyleng-1);	// Rescan non-;
				BEGIN(INITIAL);		// Normal mode
				return ATTRTYPE;	// Return collection
			}
<ATTRTP>;{ATTROPT}	{
				struct list_attropt *val;
printf ("Attribute Option: %s\n", yytext);
				val = malloc (sizeof (struct list_attropt) + yyleng - 1 );
				if (val == NULL) {
					yy_fatal_error ("Out of memory while attaching an Attribute Option", prs);
				}
				val->next = NULL;
				strcpy (val->option, yytext + 1);
				*insert_attropt_here = val;
				insert_attropt_here = &val->next;
				hash_token_text (hbuf, ATTROPT, val->option);
				// Continue scanning, pass insert_attropt_here
			}
<ATTRTP>;[^a-zA-Z]	{
printf ("Funny semicolon in %s ignored\n", yytext);
}

{INTEGER}	{
			struct var_value val;
			val.type = VARTP_INTEGER;
			val.typed_integer = strtol (yytext, NULL, 0);
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_CONSTANT);
			var_set_value (prs->vartab, yylval->varnum, &val);
			hash_token_blob (hbuf, INTEGER, &val.typed_integer, sizeof (val.typed_integer));
			return INTEGER;
		}
{FLOAT}		{
			// Constants are stored in the variable table
			struct var_value val;
			val.type = VARTP_FLOAT;
			val.typed_float = strtof (yytext, NULL);
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_CONSTANT);
			var_set_value (prs->vartab, yylval->varnum, &val);
			hash_token_blob (hbuf, FLOAT, &val.typed_float, sizeof (val.typed_float));
			return FLOAT;
		}
{STRING}	{
			char *here = yytext;
			char *copy = malloc (yyleng - 2 + 1);
			struct var_value val;
			if (copy == NULL) {
				yy_fatal_error ("Out of memory replicating string", prs);
			}
			val.type = VARTP_STRING;
			val.typed_string = copy;
			while (*here) {
				if (*here == '\"') {
					here++;
					continue;
				}
				if (*here == '\\') {
					here++;
					// Continue with the next, even a quote
				}
				*copy++ = *here++;
			}
			*copy++ = 0;
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_CONSTANT);
			var_set_value (prs->vartab, yylval->varnum, &val);
			hash_token_text (hbuf, STRING, val.typed_string);
			return STRING;
		}
{BLOB}		{
			char *here = yytext + 1;
			uint8_t *copy = malloc (yyleng >> 1);
			struct var_value val;
			int hex;
			if (copy == NULL) {
				yy_fatal_error ("Out of memory replicating hexbyte string", prs);
			}
			val.type = VARTP_BLOB;
			val.typed_blob.str = copy;
			val.typed_blob.len = 0;
			hex = 1;
			while (*here) {
				hex <<= 4;
#if ('9' > 'A') || ('F' > 'a')
#error "ASCII assumption in hex decoder violated"
#endif
				if (*here <= '9') {
					hex += *here++ - '0';
				} else if (*here >= 'a') {
					hex += *here++ - 'a' + 10;
				} else {
					hex += *here++ - 'A' + 10;
				}
				if (hex >= 256) {
					*copy++ = hex & 0x00ff;
					val.typed_blob.len ++;
					hex = 1;
				}
			}
			yylval->varnum = var_have (prs->vartab, yytext, VARKIND_CONSTANT);
			var_set_value (prs->vartab, yylval->varnum, &val);
			hash_token_blob (hbuf, BLOB, val.typed_blob.str, val.typed_blob.len);
			return BLOB;
		}

\<\-		hash_token (hbuf, GEN_FROM); return GEN_FROM;
\-\>		hash_token (hbuf, DRIVE_TO); BEGIN(DRIVER); return DRIVE_TO;

\@		hash_token (hbuf, AT); return AT;
\,		hash_token (hbuf, COMMA); return COMMA;
\+		hash_token (hbuf, PLUS); return PLUS;
\:		hash_token (hbuf, COLON); return COLON;

\=		hash_token (hbuf, CMP_EQ); return CMP_EQ;
\!\=		hash_token (hbuf, CMP_NE); return CMP_NE;
\<\=		hash_token (hbuf, CMP_LE); return CMP_LE;
\<		hash_token (hbuf, CMP_LT); return CMP_LT;
\>\=		hash_token (hbuf, CMP_GE); return CMP_GE;
\>		hash_token (hbuf, CMP_GT); return CMP_GT;

\(		hash_token (hbuf, OPEN); return OPEN;
\)		hash_token (hbuf, CLOSE); return CLOSE;
\&		hash_token (hbuf, LOG_AND); return LOG_AND;
\|		hash_token (hbuf, LOG_OR ); return LOG_OR ;
\!		hash_token (hbuf, LOG_NOT); return LOG_NOT;

\*		hash_token (hbuf, STAR); return STAR;
\[		hash_token (hbuf, BRA); return BRA;
\]		hash_token (hbuf, KET); return KET;

.		{
			fprintf (stderr, "Unexpected character '%c'\n", yytext [0]);
			fatal_error ("scanning failed");
		}

%%


#include "types.h"
#include "variable.h"
#include "generator.h"
#include "condition.h"


/* Translation from the (portable) parser_type to Bison-defined %token values.
 */
static int pt2bison [] = {
	START_SCRIPT, START_LINE,
	START_GENERATOR, START_CONDITION, START_DRIVEROUT
};

/* Initialise a parser buffer for collection of data.  This routine allocates
 * basic tables for variables, generators, conditions and drivers; and it
 * connects them.  Failure to allocate memory leads to halting the program.
 * Success is returned as a non-zero value.
 */
int pulley_parser_init (struct parser *prs) {
	yylex_init_extra (prs, &prs->lexer);
	prs->varstack_sp = PARSER_VARIABLE_STACK_ENTRIES;
	while (prs->varstack_sp > 0) {
		prs->varstack [--prs->varstack_sp] = bitset_new (vartab_share_type (prs->vartab));
	}
	prs->action_sp = sizeof (prs->action) -1;
	prs->action [prs->action_sp] = BNDO_ACT_DONE;
	hash_start (&prs->scanhashbuf);
	prs->vartab = vartab_new ();
	prs->gentab = gentab_new ();
	prs->cndtab = cndtab_new ();
	prs->drvtab = drvtab_new ();
	prs->newdrv = DRVNUM_BAD;
	prs->newcnd = DRVNUM_BAD;
	type_t *vartp = vartab_share_type (prs->vartab);
	type_t *gentp = gentab_share_type (prs->gentab);
	type_t *cndtp = cndtab_share_type (prs->cndtab);
	type_t *drvtp = drvtab_share_type (prs->drvtab);
	cndtab_set_variable_type  (prs->cndtab, vartp);
	gentab_set_variable_type  (prs->gentab, vartp);
	gentab_set_driverout_type (prs->gentab, drvtp);
	vartab_set_generator_type (prs->vartab, gentp);
	vartab_set_condition_type (prs->vartab, cndtp);
	vartab_set_driverout_type (prs->vartab, drvtp);
	drvtab_set_vartype (prs->drvtab, vartp);
	drvtab_set_gentype (prs->drvtab, gentp);
	drvtab_set_cndtype (prs->drvtab, cndtp);

	return 1;
}

/* Cleanup the syntax-oriented structures used while parsing, and possibly
 * processed further.  The data for Bison and Flex are stored inside the
 * parser structure, and they will be cleaned up inasfar as necessary.
 *
 * It is assumed that pulley_parser_init() has succeeded, meaning that the
 * internal storage structures were all allocated successfully.
 *
 * The parsed semantics, notably vartab/gentab/cndtab/drvtab, will continue
 * to exist until these are separately cleaned up.
 */
void pulley_parser_cleanup_syntax (struct parser *prs) {
	yyset_in (NULL, prs);
	prs->varstack_sp = 0;
	while (prs->varstack_sp < PARSER_VARIABLE_STACK_ENTRIES) {
		bitset_destroy (prs->varstack [prs->varstack_sp++]);
	}
	yylex_destroy (prs->lexer);
}

/* Cleanup the semantical part of the parser structures.  This eradicates
 * the last traces of a parsing process.
 *
 * It is assumed that pulley_parser_init() and pulley_parser_cleanup_syntax()
 * have both run.
 */
void pulley_parser_cleanup_semantics (struct parser *prs) {
	vartab_destroy (prs->vartab);
	gentab_destroy (prs->gentab);
	cndtab_destroy (prs->cndtab);
	drvtab_destroy (prs->drvtab);
}

/* Parse a (piece of a) Pulley script that is stored in a memory buffer.
 *
 * The memory buffer may be directly taken from a struct berval that has
 * arrived over LDAP.  They will be combined to form the larger whole of
 * the complete Pulley script.
 */
int pulley_parser_buffer (struct parser *prs, enum parser_type pt, char *buf, unsigned int buflen) {
	int retval;
	YY_BUFFER_STATE lexbuf;
	prs->bison_switch = pt2bison [pt];
	lexbuf = yy_scan_bytes (buf, buflen, prs->lexer);
	yy_switch_to_buffer (lexbuf, prs->lexer);
	retval = yyparse (prs);
	yy_flush_buffer (lexbuf, prs->lexer);
	yy_delete_buffer (lexbuf, prs->lexer);
	return retval;
}

/* Parse a Pulley script that is stored in a file.  Assume that the file is
 * opened at the proper position to be parsed, and that parsing continues to
 * the end of file.
 *
 * Multiple files may be parsed.  They will be combined to form the larger
 * whole of the complete Pulley script.
 */
int pulley_parser_file (struct parser *prs, FILE *f) {
	prs->bison_switch = START_SCRIPT;
	yyset_in (f, prs->lexer);
	return yyparse (prs);
}

/* Collect the hash value over the set of Pulley scripting lines collected
 * since program start or the last pulley_parse_cleanup().  Store the value
 * in prs->scanhash.
 *
 * Note that the hash_t value should be treated as an opaque byte array;
 * its size is to be determined with sizeof() and comparing or moving its
 * contents should be done with memcmp() and memcpy().  Do not assume that
 * the contents are printable, readable or NUL-terminated strings.
 */
void pulley_parser_hash (struct parser *prs) {
	hash_finish (&prs->scanhashbuf, &prs->scanhash);
}

