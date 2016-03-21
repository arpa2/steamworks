SQLite3 implementation of Pulley Scripts
========================================

>   *The Pulley script language is built on top of an SQL implementation,
>   specifically SQLite3 which is local and has all the desired properties,
>   including sufficiently clever query optimisation.*

Naming Databases
----------------

-   The database is named after the hash of the Pulley script, denoted in
    hexadecimal.

-   This makes it simple to try new scripts and see what they produce.

-   It is even possible to compare script output; great for debugging and
    testing.

-   The advise is to use test drivers on such scripts, and observe responses to
    changes.

-   Note it is quite possible to tap the same source in multiple scripts.

Mapping to Tables
-----------------

-   Below, we define a number of name prefixes for tables; these prefixes are
    followed by the hex digits that the parser found for the line; this naming
    may prove helpful with changes to the configuration

-   There is a `gen_` table for each generator that acts at least once as a
    co-generator

-   There is a `drv_all` table for each driver that has guards, meaning that not
    all variables from variable partitions end up in the output

-   The `gen_` tables have attributes named like the variables with `var_`
    prefixed, each typed as a `blob not null`; before these is a first column
    named `key_` plus the same extension as behind `gen_` of type `integer`
    which is a 64-bit hash of the further contents; it serves as an [integer
    primary key] of the table, and is in fact an alias for SQLite3’s `_rowid_`
    column, which indexes its B-trees.  Using the extension avoids collisions of
    these columns during `natural join` and it also makes the values easily
    recognisable in the output.

-   The `gen_` table may contain the variables in any order, as these are always
    retrieved by name, including in the eventual output from a `select`
    statement.  The order will be decided when the table is setup, and
    subsequent `insert` statements will also have to mention the variable names
    explicitly or have another mechanism to follow the intended order (such as
    query parameters named after the variables).  This is important to handle
    reprocessing table data after the order of the lines in the Pulley script
    has been shuffled, which might lead to other variable numbers and therefore
    other variable iteration orders.

-   The `drv_all` tables have a column with a `out_hash` value of type `integer`
    which serves as a primary key and a 4-byte `out_repeat` value of type
    `integer`; the `out_hash` is an [integer primary
    key](<https://www.sqlite.org/lang_createtable.html#rowid>) of the table, and
    is in fact an alias for SQLite3’s `_rowid_` column, which indexes its
    B-trees.

-   The `key_` index in the `gen_` tables runs over the lexical hash of the
    generator line and its generator variables in their table column order,
    prefixing each generator variable with a 4-byte length indicator in network
    byte order.  The hash algorithm used may be insecure; it currently is the
    FNV-1a hash on 64 bits.

-   The `out_hash` value in `drv_all` runs over the lexical hash of the driver
    line and its output variables in their order of occurrence, prefixing each
    output variable with a 4-byte length indicator in network byte order. The
    hash algorithm used may be insecure; it currently is the
    [FNV-1a](<https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function>) hash
    on 64 bits.

-   For the purposes of iteration over all `gen_` table entries, there is no use
    in setting up an index to the table. For removal of entries, this might be
    interesting, but it is uncertain what would make a good index, in general.
    We may leave this to the administrator to add?

-   For quickly finding an entry in the `drv_` tables, the `hash` column is a
    primary key and an index should be setup for it.

Making queries
--------------

-   Consider a generator `g0` that writes to `d`, while also involving
    co-generators `g1` and `g2`. Its variable partitions introduce conditions
    `c0`, `c1` and `c2`. The output variables for `d` are `v0`, `v1` and `v2`
    which hash into `h`; its guards are `u0` and `u1`

-   A query to implement this would be:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    select v0,v1,v2
    from g0,g1,g2
    where c0
    and c1
    and c2
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   When the fork due to `g0` whose hash is `_a5b4c3d2` adds a new record with
    variables `v0` and `u0`, then the following insertion would be warranted:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    insert into gen_a5b4c3d2
    values (?v0,?u0)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   Alternatively, when the fork removes that record, the following query would
    be the right thing to do:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    delete from gen_a5b4c3d2
    where v0=?v0
    and u0=?u0
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   Finally, there is a need to update the driver with the newly created output
    with hash `h`; we implement this with counters. We first find the current
    counter value,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    select counter
    from drv_6c5ad4a5
    where hash=?h
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Then, if a record was added and the counter is absent we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    insert into drv_6c5ad4a5
    values (?h,1)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was added and the counter was present, we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    update drv_6c5ad4a5
    set counter=counter+1
    where hash=?h
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was removed and the counter is `1` we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    delete from drv_6c5ad4a5
    values (?h,0)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was removed and the counter is not `1` we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    update drv_6c5ad4a5
    set counter=counter-1
    where hash=?h
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    This covers all the use cases at once; note that each as its parameters `h`
    alone.

-   All these queries are made within one transaction.

Static Query Optimisation
-------------------------

-   It is possible to prepare, and thereby optimise their execution, before
    starting the actual application. This is the result of queries being
    generated for the given LDAP structures and their use in a given Pulley
    script.

We Might do Better
------------------

-   One generator may drive multiple drivers.

-   Drivers pull in variables, and these will often overlap.

-   SQL independently iterates over the co-generators used by each driver.

-   We might construct a combined update for each driver by merging such
    queries.

-   This would only require marking variable changes with all interested
    drivers.

-   Initial code for doing this has been created, but not finished.

-   SQLite3 will first get a chance to prove if its performance is acceptable.

-   Performance penalty is at worst linear in the number of drivers for a
    generator.
