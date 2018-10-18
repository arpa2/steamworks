Driver API
----------

The backend drivers to Pulley are pluggable components that take the form of
shared C-style libraries. They exhibit a versioned API over which Pulley can
instruct it to do drive changes out in a personalised manner.

The main task of the API is to pass through additions and removals of tuples of
variables. Depending on the configuration of the backend, the names of variables
may be provided, and a few other things may also be configured. In general, the
setup of the line in the Pulley script contains constants which are provided
during driver instantion, which occurs when the script is loaded.

The driver has the responsibility of also handling the guards, although it may
choose to ignore it and accept that additions and removals of variable lists are
mere hints that they might have been added or removed, and that local caching
may need updates to reflect that. In other situations, the driver will only
count occurrences of guards, but not actually store them. The requirements of
the driver are provided to handle this.

The following drivers are initially considered for Pulley:

-   **elektra** is an interface to libelektra, a small library written in C to
    deliver information to a standardised tree structure; comment fields are
    used to store a counter (in text) and an optional description provided in
    the driver line of the Pulley script. Elektra supports a multitude of
    plugins that can be mounted into its tree for delivery of stored information
    to a plethora of destinations. The `kdbSet` command provides an atomic, but
    not two-phase committing interface to writing out sets of changes; `ksDup`
    could be used to relieve some of the concerns, and have a rollback facility.

-   **augeas** is an interface to configuration files, which are each
    independently driven with plugins. The transaction mechanism will protect
    files from being overwritten until the final commit is requested, which may
    come down to Augeas' `save` command. Augeas may be used through elektra.

-   **uci** is an interface, not dissimilar to augeas except in how it works,
    for OpenWRT routers.  It has a transactional interface.

-   **json** and **xml** provide a portable manner to release information into
    generic JSON and XML formats; although this may not be usable to generate an
    already-defined format of such files, it will be useful to dump all
    information provided in some form that can be used by applications willing
    to accept the generic format as provided. One example application could be a
    static webserver environment which uses the information in client-side
    scripts. Some facilities for indexing may be required as part of this setup.

-   **python?** is a mapping of the driver API to Python, with backends being
    loaded from a dedicated shared library directory. The name of the Python
    script to load is provided as a required configuration parameter.  We might
    also opt for a template language, either within Python or otherwise.

-   **bash?** is a mapping of the API to shell script invocations?

-   **linux-hotplug?** is a mapping of the API to Linux' hotplugging interface?

-   **linux-ufs?** is a mapping to the Linux User FileSystem?

