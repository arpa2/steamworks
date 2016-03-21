Pulley Script Language Concepts
===============================

>   *This is the conceptual model behind the Pulley script language.*

The Pulley script language was created to pull information from LDAP and have
some intelligence for combining entries to form output that is suitable for end
user programs.  This allows delivery of LDAP data from a variety of structures
directly into the storage formats that applications use.  Being served in their
native storage format, there is no requirement for such applications to be aware
of LDAP themselves, only to be able to process configuration updates.

Primary and Secondary script language Concepts
----------------------------------------------

The Pulley script language consists primarily of:

-   *Generators*, which extract data from LDAP, and receive additions and
    removals

-   *Records*, the set of variables that a generator adds or removes by
    “forking”

-   *Conditions*, the constraints on variables as they occur in records

-   *Partitions*, the transitive closure of variables mentioned in conditions

-   *Drivers*, the output statements that add or remove variable values for
    applications

-   *Outputs*, the sets of variables that are added or removed as one in a
    driver

A few secondary concepts are also of interest:

-   *Guards*, to ensure the presence of variables in records, partitions or
    outputs

    -   *Implicit Guards* are derived by the compiler

    -   *Explicit Guards* are added to the records, partitions, outputs defined
        by a rule

-   *Weight*, to set the weight or record multiplier for generator, condition or
    driver

Connecting dots
---------------

The general flow of information is

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Generator --> Conditions --> Driver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each of these stages has its own reference set of variables.  For clarity of the
semantics, it is vitally important how these are connected.

-   The conditions applied to drivers encompass all variables written to them;
    the other variables that occur together with them in conditions also matter,
    and are added to the driver as implicit guards.

-   The outputs stored in a driver include directly used variables as well as
    guards.  The guards may be the distinction between otherwise same outputs.
    It is possible to store these guards in relation to the outputs, or their
    hashes, or even just a counter for them.  This is a generic function that
    the Pulley may need to take over, so as to avoid confusing applications with
    multiple outputs.

-   Multiple drivers may be directed at the same application.  In this case,
    there is a use for one extra implicit guard, namely to distinguish the
    production rule.  This can be useful to allow for configuration data from a
    number of derivation rules.

-   One or more generators work together to provide the values needed in
    conditions; they orthogonally produce these pieces of information and when a
    new generator produces output there may be a need to iterate over previously
    generated results from other generators.  The records of such collaborating
    generators must be the smallest set to produce at least the partition of a
    condition.  
    TODO: This suggests the requirement to store records, rather than output?
    though output may itself be iterable? and incorporate guards? but what if
    one generator is productive while another is still empty? but this is quite
    a burden on drivers and not necessarily a win; it also explodes the
    information needed to store! and having such an SQL store directly provides
    the weight information for each generator, which may be combined with the
    conditions that it enables. we may still precompile statically though.

-   One generator may not supply a partition with all the information required;
    to that end, the previous output from other generators must be reproduced
    and taken into account.

-   One generator may supply multiple drivers with informations for their
    outputs.  There is no crossover-influence between drivers, other than that
    they will partake in a concerted update implemented through a two-phase
    commit transaction.

Database Storage
----------------

There are two kinds of data that we need to store; the output of generators for
future iteration, and a manner to keep track of guards that imply multiplicity
of output variables.  The model is so close to the SQL model that we can barely
expect to improve on existing implementations.

The most concentrated storage form for the reproduction of generated records is
direct storage of these records in tables that represent the generator.  Their
combination through a cartesian product, removal of combinations through
conditions and eventual production of output for drivers is precisely what SQL
is designed to do.  Plus, the insertion or removal of a record can be
incorporated into the two-phase commit transaction that applies to backends.

A driver may or may register with support for multiplicity.  To support it, a
driver must be able to accept as many removals as additions, each annotated with
guards to form unique combinations.  To facilitate drivers without multiplicity
support, the database should either store the guards along with the records
written, or at least count them.  A compact manner of doing this is to store a
secure digest for the driver’s output variables, along with a counter.  This can
be used to provide an output to the driver on the first entry and to retract an
output on last removal.  The changes to this database storage should also be
taken into the 2-phase commit transaction.

A database to support this scheme should:

-   Manage data as sets of records with cartesian product and selection

-   Participate in 2-phase commit transactions

-   Provide lightweight storage in the file system

-   Support variable-sized blobs to hold attribute values

-   Support a secure digest algorithm over combinations of blobs

    -   Alternatively, combine secure digests by treating them as
        prime-multiplied integers

The SQLite3 database seems to be perfect, with its

-   simple file-based storage model

-   [transactions](<https://www.sqlite.org/lang_transaction.html>) and
    [savepoints](<https://www.sqlite.org/lang_savepoint.html>) to facilitate
    2-phase commit

-   [variable-length BLOB storage](<https://www.sqlite.org/different.html#flex>)

-   query prepare with [query
    optimisation](<https://www.sqlite.org/queryplanner-ng.html>) that may use
    [statistical analysis](<https://www.sqlite.org/compile.html#enable_stat4>).

On brief encounter, one might wonder if Pulley adds anything, compared to SQL at
all, but indeed it does — aside from subscribing for virtually-instant updates,
any generated records are processed incrementally, and only things that actually
change are passed on as driver output.  The increments are derived from the
Pulley script language in an automatic manner, allowing multi-way pass-throughs
without any effort of declaring (or programming) these flows separately.
