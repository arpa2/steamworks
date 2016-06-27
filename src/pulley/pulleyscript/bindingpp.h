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

namespace SteamWorks
{

namespace PulleyScript
{

#ifndef NDEBUG
/**
 * Explain the coded @p binding (for machine-language explanation,
 * see binding.h) relative to the variable table @p vars.
 * Outputs to the logfile.
 */
void explain_binding(vartab* vars, uint8_t* binding, uint32_t len);
#endif

}  // namespace PulleyScript
}  // namespace

#endif
