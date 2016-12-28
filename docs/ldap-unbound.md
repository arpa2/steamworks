# A SteanWorks Backend for managing a LAN

Suppose you have a local (home) network, with a dozen or more
machines / devices attached. It's nice to be able to use DNS
to refer to the machines: give them all a name. Give the modem
the name "modem.local.lan", the smart TV could be "tv.local.lan",
etc. You could configure this with /etc/hosts, on UNIX-like
machines, but that won't work everywhere, and mobile devices
on your network may not have a hosts-file to configure.

So, you install a local caching, resolving DNS server on your
network and configure DHCP on the network to point to that DNS.

Then you need to configure your local DNS to match names to
IP addresses. This document assumes you are using the [unbound] [1]
DNS server.

[1] https://unbound.net/ Unbound

## Configuring local addresses

The unbound server can be configured through one or more configuration
files. On FreeBSD, the configuration files live in /var/unbound; there
is a subdirectory conf.d/ that can contain multiple individual configuration
files. The format is described in the [unbound documentation] [2],
and looks something like this for a local zone:

    server:
        local-zone: "local.lan" static
        local-data: "modem.local.lan.     IN A 192.168.0.2"
        local-data-ptr: "192.168.0.2  modem.local.lan"

Here the important thing is the mapping of a name ("modem.local.lan") to
an IPv4 address (192.168.0.2); this needs to be consistent back-and-forth.

[2] https://unbound.net/documentation/unbound.conf.html "Unbound.conf documentation"

## Storing local addresses in LDAP

OpenLDAP is shipped with an LDAP schema for RFC2307 ("Using LDAP as a 
Network Information Service"), which provides an object class "ipHost".
To use this, the nis.schema must be included in the server configuration.

To store information in the LDAP DIT, once this schema is included,
LDIF such as the following can be used (here the name of the device
is the distiguished name, so we're basically associating an IP address
with the name and not the other way around).

    dn: dc=modem,dc=local,dc=lan
    objectclass: ipHost
    dc: modem
    ipHostNumber: 192.168.0.2





