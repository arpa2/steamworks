/*
Copyright (c) 2016 InternetWide.org and the ARPA2.net project
All rights reserved. See file LICENSE for exact terms (2-clause BSD license).

Adriaan de Groot <groot@kde.org>
*/

/**
 * This is a C++ wrapper around the PulleyScript parser / generator / etc.
 * complex. It handles the bindings "machine language".
 */
#ifndef STEAMWORKS_PULLEYSCRIPT_BINDINGPP_H
#define STEAMWORKS_PULLEYSCRIPT_BINDINGPP_H

#include "variable.h"

#include <string>
#include <vector>

namespace SteamWorks
{

namespace PulleyScript
{

/**
 * Explain the coded @p binding (for machine-language explanation,
 * see binding.h) relative to the variable table @p vars.
 * Outputs to the logfile.
 *
 * If @p filterexp is not a null-pointer, builds an LDAP filter
 * expression for this binding based on the constant-comparisons
 * found in the binding.
 *
 * The vector @p bound_varnums lists the bound variables for
 * this generator (e.g. the variables associated with the generator
 * being explained), and the vector @p variable_names (which is
 * assumed to be the same size) is filled with the attributes
 * bound to those variables.
 */
void explain_binding(vartab* vars,
		     uint8_t* binding,
		     uint32_t len,
		     std::string* filterexp,
		     const std::vector<varnum_t>& bound_varnums,
		     std::vector<std::string>& variable_names);

}  // namespace PulleyScript
}  // namespace

#endif
