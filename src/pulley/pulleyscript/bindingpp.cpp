/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

#include "bindingpp.h"
#include "binding.h"

#include <logger.h>

static char hex[] = "0123456789ABCDEF";

/**
 * Log a BLOB to the given logging stream.
 */
static void dump_blob(SteamWorks::Logging::LoggerStream& log, uint8_t *p, uint32_t len)
{
	for (uint32_t i=0; i<len; i++, p++)
	{
		char c0 = hex[(*p & 0xf0) >> 4];
		char c1 = hex[*p & 0x0f];
		log << c0 << c1 << ' ';
	}
}

static void _indent(SteamWorks::Logging::LoggerStream& log, unsigned int indent)
{
	for (unsigned int i=0; i<indent; i++)
	{
		log << ' ' << ' ';
	}
}

varnum_t extract_varnum(uint8_t* p)
{
	return *(varnum_t *)p;
}

void SteamWorks::PulleyScript::explain_binding(vartab* vars, uint8_t* binding, uint32_t len, std::string* filterexp)
{
	auto& log = SteamWorks::Logging::getLogger("steamworks.pulleyscript");

	{
		auto d = log.debugStream();
		d << "Explain binding @" << (void *)binding << ' ';
		dump_blob(d, binding, len);
	}

	unsigned int indent = 0;
	uint8_t* p = binding;
	uint8_t* end = p + len;

	while (p < end)
	{
		auto d = log.debugStream();
		_indent(d, indent);

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
			d << "BIND " << operand_s << ' ' << var_get_name(vars, extract_varnum(p+1)) << "~" << var_get_name(vars, extract_varnum(p+1+sizeof(varnum_t)));
			p += 1 + 2 * sizeof(varnum_t);
			break;
		case BNDO_ACT_CMP:
			d << "CMP  " << operand_s << ' ' << var_get_name(vars, extract_varnum(p+1)) << "~" << var_get_name(vars, extract_varnum(p+1+sizeof(varnum_t)));
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
			dump_blob(d, &opcode, 1);
			goto done;
		}
	}
done:
	log.debugStream() << " ..done binding @" << (void *)binding;
}
