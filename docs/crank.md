# SteamWorks Crank #

The Crank is a source of configuration data in the
SteamWorks system. The purpose of the Crank is to enable
operators to view and modify and enter new configuration data
for the rest of the system.

The Crank connects to a single LDAP server, which may be
SyncRepl-enabled (but does not have to be for the Crank --
however, other parts of the SteamWorks system *do* require
SyncRepl-enabled servers). The Crank writes to the LDAP
server. At the other end of the SteamWorks toolchain,
the Pulley writes the information from the LDAP DIT
to some backend system.

## Crank JSON Interface ##

The Crank has a dozen primary commands and a handful of administrative
and informational commands. The primary commands connect to upstream,
query, update, modify, delete and manipulate the DIT, and stop the Crank.

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

(This is a generic SteamWorks component command) Stop the Crank
component and terminate the FCGI process. Processing steps that
have already started (e.g. in response to DIT changes) will be
completed, but no new work started.

 - Verb: `stop`
 - Return: nothing (there is no HTTP response, as none is sent)


### Search ###

Perform an LDAP search (query) on the LDAP server the Crank
is connected to. Synchronously returns the DIT data.

 - Verb: `search`
 - Argument: `base` A DN for the (sub-)tree to query. Uses the
   usual LDAP search notation.
   Example, `dc=example,dc=com`
 - Argument: `filter` A filter-expression for the (sub-)tree to
   query. Uses the usual LDAP filter notation.
   Example, `(objectclass=device)`
 - Return: HTTP status code and JSON object.
   The top-level keys of the JSON-object are the DNs that were
   found (e.g. `dc=www,dc=example,dc=com`). Each key points to
   a JSON object with attribute-data. The attribute-data JSON
   object has attribute-names as keys (e.g. `dn` and `cn` and
   `objectClass`) and each key has a value which may be a
   string or a JSON list (of strings).


TODO: deal with type-ambiguity
TODO: what about lists with a single value?


### Type Information ###

Get the type-information from the LDAP server that the Crank
is connected to. This queries the root DN of the server to
obtain the subschema-information.

 - Verb: `typeinfo`
 - Return: HTTP status code and JSON object.
   The JSON object is the same as would be returned from a
   corresponding search. The top-level key is `cn=Subschema`,
   which leads to a JSON object with two keys: `dn` (of course)
   and `objectClasses`, which is a list of strings describing
   the types in the DIT.

TODO: is that actually a useful result?
TODO: can we parse that stuff to provide a more structured return?


### Update ###

 - Verb: `update`
 - Argument: `values`
   A JSON list of JSON objects. Each object has a key `dn` specifying
   its DN, and zero or more other keys specifying attribute changes
   (e.g. `"description": "A device"`). The changes are applied as a
   modyify LDAP action and the attribute values are *replaced*.
 - Return: HTTP status code and empty JSON data.

TODO: better return information
TODO:  - return transaction information?
TODO:  - return complete new state of object?
TODO: what about failing updates?
TODO: what about list-valued / multi-valued attributes?
TODO: how about removing attributes? other verb? set to undefined?
TODO: what about prerequisites / assumptions about previous state?
