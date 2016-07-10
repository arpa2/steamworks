/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "bindingpp.h"
#include "binding.h"

#include <logger.h>

#include <assert.h>

varnum_t extract_varnum(uint8_t* p)
{
	return *(varnum_t *)p;
}

void SteamWorks::PulleyScript::decode_parameter_binding(vartab* vars, uint8_t* binding, uint32_t len, std::vector< std::string >& expressions)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

	{
		auto d = log.debugStream();
		d << "Decode binding @" << (void *)binding << ' ';
		SteamWorks::Logging::log_hex(d, binding, len);
	}

	unsigned int indent = 0;
	uint8_t* p = binding;
	uint8_t* end = p + len;
	// Count number of compared-vars; if there is more than one, create
	// a conjunction-expression for the LDAP search-expression.
	unsigned int varcount = 0;

	while (p < end)
	{
		auto d = log.debugStream();
		SteamWorks::Logging::log_indent(d, indent);

		uint8_t opcode = (*p) & 0xf;
		uint8_t operand_t = (*p) & 0x30;

		varnum_t v0 = VARNUM_BAD, v1 = VARNUM_BAD;

		std::string s;
		switch (opcode)
		{
		case BNDO_ACT_CMP:
			v0 = extract_varnum(p+1);
			v1 = extract_varnum(p+1+sizeof(varnum_t));

			s = var_get_name(vars, v0);
			s.append(1, '=');
			s.append(var_get_name(vars, v1));
			expressions.push_back(s);
			p += 1 + 2 * sizeof(varnum_t);
			break;
		case BNDO_ACT_DONE:
			goto done;
		default:
			d << "BAD OPCODE ";
			SteamWorks::Logging::log_hex(d, &opcode, 1);
			goto done;
		}
	}
done:
	;
}

void SteamWorks::PulleyScript::explain_binding(vartab* vars,
					       uint8_t* binding,
					       uint32_t len,
					       std::string* filterexp,
					       const std::vector<varnum_t>& bound_varnums,
					       std::vector<std::string>& variable_names)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

	{
		auto d = log.debugStream();
		d << "Explain binding @" << (void *)binding << ' ';
		SteamWorks::Logging::log_hex(d, binding, len);
	}

	unsigned int indent = 0;
	uint8_t* p = binding;
	uint8_t* end = p + len;
	// Count number of compared-vars; if there is more than one, create
	// a conjunction-expression for the LDAP search-expression.
	unsigned int varcount = 0;

	while (p < end)
	{
		auto d = log.debugStream();
		SteamWorks::Logging::log_indent(d, indent);

		uint8_t opcode = (*p) & 0xf;
		uint8_t operand_t = (*p) & 0x30;

		const char* operand_s = nullptr;
		switch (operand_t)
		{
		case BNDO_SUBJ_NONE:
			operand_s = "None";
			break;
		case BNDO_SUBJ_ATTR:
			operand_s = "attr";
			break;
		case BNDO_SUBJ_RDN:
			operand_s = "RDN";
			break;
		case BNDO_SUBJ_DN:
			operand_s = "DN";
			break;
		}

		varnum_t v0 = VARNUM_BAD, v1 = VARNUM_BAD;
		switch (opcode)
		{
		case BNDO_ACT_DOWN:
			d << "DOWN " << operand_s;
			indent++;
			p++;
			break;
		case BNDO_ACT_HAVE:
			d << "HAVE " << operand_s << ' ' << var_get_name(vars, extract_varnum(p+1));
			p += 1 + sizeof(varnum_t);
			break;
		case BNDO_ACT_BIND:
			v0 = extract_varnum(p+1);
			v1 = extract_varnum(p+1+sizeof(varnum_t));
			d << "BIND " << operand_s << ' ' << var_get_name(vars, v0) << " (" << v0 << ") ~ " << var_get_name(vars, v1) << " (" << v1 << ')';
			for (unsigned int i=0; i < bound_varnums.size(); i++)
			{
				if (v1 == bound_varnums[i])
				{
					variable_names[i] = var_get_name(vars, v0);
				}
			}
			p += 1 + 2 * sizeof(varnum_t);
			break;
		case BNDO_ACT_CMP:
			v0 = extract_varnum(p+1);
			v1 = extract_varnum(p+1+sizeof(varnum_t));
			d << "CMP  " << operand_s << ' ' << var_get_name(vars, v0) << "~" << var_get_name(vars, v1);
			if (filterexp && (var_get_kind(vars, v1) == VARKIND_CONSTANT))
			{
				// For constants, the name is the value. Due to syntactical oddities
				// in the pulleyscript, we can have a constant value that is
				//  - numeric
				//  - quoted with double-quotes
				// in the LDAP filter-expression, though, we need to drop those quotes.
				const char *cval = var_get_name(vars, v1);
				filterexp->append(1, '(');
				filterexp->append(var_get_name(vars, v0));
				filterexp->append(1, '=');
				if (cval[0] == '"')
				{
					// Avoid the start and end "
					size_t len = strlen(cval);
					assert(len >= 2);
					filterexp->append(cval+1, len-2);
				}
				else
				{
					filterexp->append(cval);
				}
				filterexp->append(1, ')');
				varcount++;
			}
			p += 1 + 2 * sizeof(varnum_t);
			break;
		case BNDO_ACT_OBJECT:
			d << "OBJECT";
			p += 1;
			break;
		case BNDO_ACT_DONE:
			d << "DONE";
			goto done;
		default:
			d << "UNKNOWN ";
			SteamWorks::Logging::log_hex(d, &opcode, 1);
			goto done;
		}
	}
done:
	if (filterexp && (varcount > 1))
	{
		// Turn (v0=c0)(v1=c1)...(vn=cn) into a proper conjunction
		// according to LDAP filter syntax.
		filterexp->insert(0, "(&");
		filterexp->append(1, ')');
	}
	log.debugStream() << " ..done binding @" << (void *)binding;
}
