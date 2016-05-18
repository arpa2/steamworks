# SteamWorks Pulley #

The Pulley is the part of the SteamWorks that transforms LDAP
into usable configuration, by transforming LDAP changes into
changes into some external configuration format (ini-files,
bdb files, whatever).

The Pulley subscribes to a single upstream LDAP. This must be
a SyncRepl-enabled LDAP server (usually one fed by a SteamWorks
Shaft or directly from a Crank). The Pulley follows zero or more
subtrees of the DIT. Changes to the DIT (in one of the followed
subtrees) are fed into the Pulley and transformed there.

## Pulley JSON Interface ##

The Pulley has three primary commands and a handful of administrative
and informational commands. The primary commands connect to upstream,
follow parts of the DIT, and stop the Pulley.

### Connect ###

(This is a generic SteamWorks component command) Connect to an upstream
LDAP server (source of configuration information).

 - Verb: `connect`
 - Argument: `uri` A string (usually starting with *ldap://*)
   specifying the LDAP server to connect to. Usually only a
   hostname is required. SteamWorks components automatically use STARTTLS.
   Example, `ldap:://ldap-srv.example.com/`.
 - Return: HTTP status code and empty JSON data.

### Server Information ###

(This is a generic SteamWorks informational command) Query the
upstream server for server information. Returns a JSON object.

 - Verb: `serverinfo`
 - Return: HTTP status code and JSON object. The top-level object
   has a single key `""` (the empty string). That key namess an
   object which has sub-objects for server information.

TODO: document corresponding ldapsearch query
TODO: document sub-object keys

### Stop ###

(This is a generic SteamWorks component command) Stop the Pulley
component and terminate the FCGI process. Processing steps that
have already started (e.g. in response to DIT changes) will be
completed, but no new work started.

 - Verb: `stop`
 - Return: nothing (there is no HTTP response, as none is sent)

### Follow ###

A Pulley follows zero or more DIT (sub-)trees in the LDAP server
it has connected to (see Connect). The follow verb sets up such
a (sub-)tree. Subsequent changes to the DIT are handed off by
the Pulley to the backend.

 - Verb: `follow`
 - Argument: `base` A DN for the (sub-)tree to follow. Uses the
   usual LDAP search notation.
   Example, `dc=example,dc=com`
 - Argument: `filter` A filter-expression for the (sub-)tree to
   follow. Uses the usual LDAP filter notation.
   Example, `(objectclass=device)`
 - Return: HTTP status code and empty JSON data.

TODO: more useful return?
TODO: implement this

### Unfollow ###

Stop following a DIT (sub-)tree that has previously been followed.
The arguments are the same as for follow.

 - Verb: `unfollow`
 - Argument: `base` A DN for the (sub-)tree to follow. Uses the
   usual LDAP search notation.
   Example, `dc=example,dc=com`
 - Argument: `filter` A filter-expression for the (sub-)tree to
   follow. Uses the usual LDAP filter notation.
   Example, `(objectclass=device)`
 - Return: HTTP status code and empty JSON data.

TODO: more useful return?
TODO: matching criteria when unfollowing?
TODO: can you unfollow a sub-sub-tree?


TODO: pulleyinfo command, to find out about the internal representation of the DIT
TODO: backend-manipulation commands (for much later, with pluggable backends)
