/* binding.h -- Grammar describing how variables are bound in generators
 *
 * Bindings are combinations of LDAP dn traversals, pattern matches and
 * variable assignments (or, upon repeated encounter, comparisons).
 * Bindings are stored in the vartab, their name is bnd_<hash> where
 * <hash> is the lexer's line hash, their kind is VARKIND_BINDING and
 * their type is VARTP_BLOB.
 *
 * This header defines the byte code format for the computation of a
 * binding.  The format generally starts at the baseDN from which the
 * generator pulls its (potential) forks.  From there, the bindings walk
 * down to an end-object from which they process the attributes.
 *
 * With each step of the binding, actions are run that can influence the
 * number of potential forks that pass through to the next step.  The
 * actions are:
 *  - requiring the presence of an RDN attribute, with any value
 *  - requiring that the value of a variable or constant matches the
 *    value of an RDN component attribute
 *  - stepping down to the next RDN, while requiring that one exists
 *  - ending the RDN sequence, or else discard the potential fork
 *  - requiring that the value of a variable or constant matches one
 *    of a number of values of a given attribute (matching a variable
 *    is done on second encounter of a variable)
 *  - binding a variable to a DN node (not yet implemented)
 *  - binding a variable to an attribute in an RDN
 *  - binding a variable to each value of a given attribute in turn
 *  - ending to the binding evaluation, to continue into fork processing
 *
 * Note the use of a few special names for RDN sequences:
 *  - DCList=dnsname binds sequences of one or more dc= RDNs, and combines
 *    the components found into a DNS name in variable dnsname
 *  - SkipOneLevel=rdn binds one RDN regardless of its attribute class(es)
 *    and stores the matched result into variable rdn, which is in fact
 *    same type of DN-variable as bound by @z
 *  - SkipSubtree=rdns binds sequences of zero or more RDNs, and is
 *    otherwise similar to SkipOneLevel rdns
 * These special forms can match zero, one or multiple RDNs at once, and
 * it is necessary to try all possibilities so that continued evaluation
 * of the binding will work for all possible ways of matching the DN of
 * the fork-provoking LDAP update.  These variants are solely recognised
 * by their special name!
 * TODO: Or should we annotate in a general manner, using ? + * flags?
 *       This would however not lead to the same nice dnsname as with DCList!
 *       We might have to allow for special cases, or rely on backends?
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


/* The abstract operation that indicates how to process the following
 * attribute or RDN step(s).
 */

// Masks for the multiplicity marker, attr-or-RDN, and "plain" action
#define BNDO_MULTI_MASK  	0xc0
#define BNDO_SUBJ_MASK		0x30
#define BNDO_ACT_MASK	 	0x0f

// Masks for generic repeaters for 1 / 0,1 / 1.. / 0.. (not currently used)
#define BNDO_MULTI_ONCE		0x00
#define BNDO_MULTI_MAYBE	0x40
#define BNDO_MULTI_ONEPLUS	0x80
#define BNDO_MULTI_ZEROPLUS	( BDNO_MULTI_ONEPLUS | BNDO_MULTI_MAYBE )

// Indicator of whether act on an attribute, RDN part, or current DN
#define BNDO_SUBJ_NONE		0x00	/* No list in SUBJ, so no iteration */
#define BNDO_SUBJ_ATTR		0x10	/* Apply to attribute */
#define BNDO_SUBJ_RDN		0x20	/* Apply to RDN parts at current node */
#define BNDO_SUBJ_DN		0x30	/* Apply to DN of current node */

// Plain actions applied to deal with attributes and RDNs, ends in _DONE
#define BNDO_ACT_HAVE		0x00	/* SUBJ $1 must exist */
#define BNDO_ACT_CMP		0x01	/* SUBJ $1 must match var/const $2 */
#define BNDO_ACT_BIND		0x02	/* SUBJ $1 values each bind to var $2 */
#define BNDO_ACT_DOWN		0x0d	/* Must step down one RDN level */
#define BNDO_ACT_OBJECT		0x0e	/* Must have arrived at LDAP object */
#define BNDO_ACT_DONE		0x0f	/* Fork with current variable binding */

// In the above, $1 and $2 indicate arguments following the command, in the
// form of a varnum_t, stored at byte alignment in the machine's normal format.
// Some commands have 0, 1 or 2 arguments, as can be seen, so the next action
// is found at 0, 1 or 2 times sizeof (varnum_t).

// Variables and constants are stored in the same vartab, and their kind
// must be requested to learn if we are dealing with a variable or constant.


// EXAMPLE 1
//
// Mail=x, OU="Secretaries", O="Example Corp" <- world
//
// Binding bnd_b8a4bd87 created: >>>
//
// 0d - DOWN
// 21 - RDN CMP
//		04 00 00 00 - V4, attr,  O
//		05 00 00 00 - V5, const, "Example Corp"
// 0d - DOWN
// 21 - RDN CMP
//		02 00 00 00 - V2, attr,  OU
//		03 00 00 00 - V3, const, "Secretaries"
// 0d - DOWN
// 22 - RDN BIND
//		00 00 00 00 - V0, attr,  Mail
//		01 00 00 00 - V1, var,   x
// 0f - DONE
//
// <<<


// EXAMPLE 2
//
// Mail:y, CN="Backups", @z, CN=person, OU="People", O="Example Corp" <- world
//
// Binding bnd_007674c6 created: >>>
// 0d - DOWN
// 21 - RDN CMP
//		16 00 00 00 - V22, attr,  O
//		05 00 00 00 - V5,  const, "Example Corp"
// 0d - DOWN
// 21 - RDN CMP
//		15 00 00 00 - V21, attr,  OU
//		0b 00 00 00 - V11, const, "People"
// 0d - DOWN
// 22 - RDN BIND
//		13 00 00 00 - V19, attr,  CN
//		14 00 00 00 - V20, var,   person
// 32 - DN BIND
//		ff ff ff ff - VARNUM_BAD, space filler for the BIND form
//		12 00 00 00 - V18, var,   z
// 0d - DOWN
// 21 - RDN CMP
//		10 00 00 00 - V16, attr,  CN
//		11 00 00 00 - V17, const, Backups
// 0e - OBJECT
// 12 - ATTR BIND
//		0e 00 00 00 - V14, attr,  Mail
//		0f 00 00 00 - V15, var,   y
// 0f - DONE
//
// <<<


// EXAMPLE 3:
//
// CN:someperson+CN:x+CN:y, CN=someperson + CN=x + CN=z, OU=z <- world
//
// Binding bnd_8c1f64e7 created: >>>
// 0d - DOWN
// 22 - RDN BIND
//		11 00 00 00 - V17, attr,  OU
//		10 00 00 00 - V18, var,   z
// 0d - DOWN
// 21 - RDN CMP
//		0f 00 00 00 - V15, attr,  CN
//		10 00 00 00 - V16, var    z
// 22 - RDN BIND
//		0e 00 00 00 - V14, attr,  CN
//		01 00 00 00 - V1,  var,   x
// 22 - RDN BIND
//		0d 00 00 00 - V13, attr,  CN
//		09 00 00 00 - V9,  var,   someperson
// 0e - DOWN
// 12 - ATTR BIND
//		0b 00 00 00 - V11, attr,  CN
//		0c 00 00 00 - V12, var,   y
// 11 - ATTR CMP
//		0a 00 00 00 - V10, attr,  CN
//		01 00 00 00 - V1,  var,   x
// 11 - ATTR CMP
//		08 00 00 00 - V8,  attr,  CN
//		09 00 00 00 - V9,  var,   someperson
// 0f - DONE
//
// <<<

