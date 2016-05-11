# SteamWorks Pulley #

The Pulley is the part of the SteamWorks that transforms LDAP
into usable configuration, by transforming LDAP changes into
changes into some external configuration format (ini-files,
bdb files, whatever).

The pulley subscribes to a single upstream LDAP. This must be
a SyncRepl-enabled LDAP server (usually one fed by a SteamWorks
Shaft or directly from a Crank). The pulley follows zero or more
subtrees of the DIT. Changes to the DIT (in one of the followed
subtrees) are fed into the pulley and transformed there.

## Pulley JSON Interface ##

