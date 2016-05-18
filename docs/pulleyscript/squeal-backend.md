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
    column, which indexes its B-trees. Using the extension avoids collisions of
    these columns during `natural join` and it also makes the values easily
    recognisable in the output.

-   The `gen_` table may contain the variables in any order, as these are always
    retrieved by name, including in the eventual output from a `select`
    statement. The order will be decided when the table is setup, and subsequent
    `insert` statements will also have to mention the variable names explicitly
    or have another mechanism to follow the intended order (such as query
    parameters named after the variables). This is important to handle
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
    byte order. The hash algorithm used may be insecure; it currently is the
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

Making Queries
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
    values (?hash,?v0,?u0)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   Alternatively, when the fork removes that record, the following query would
    be the right thing to do:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    delete from gen_a5b4c3d2
    where key_a5b4c3d2 = ?hash
    and var_v0=?v0
    and var_u0=?u0
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-   Finally, there is a need to update the driver with the newly created output
    with hash `h`; we implement this with counters. We first find the current
    counter value,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    select max (out_repeat)
    from ( select out_repeat
           from drv_all
           where out_hash = ?hash
    union values (0) )
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Then, if a record was added and the counter is absent we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    insert into drv_all
    values (?hash,1)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was added and the counter was present, we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    update drv_all
    set out_repeat = out_repeat + 1
    where out_hash = ?hash
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Note: We can combine the last three queries into one more complex query,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    insert or replace into drv_all
    select ?hash, 1 + max (out_repeat)
    from ( select out_repeat
           from drv_all
           where out_hash = ?hash
           union values (0) );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was removed we would also start retrieving the current
    counter,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    select max (out_repeat)
    from ( select out_repeat
           from drv_all
           where out_hash = ?hash
    union values (0) )
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    and the counter value is `1` we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    delete from drv_all
    where out_hash = ?hash
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Or, if a record was removed and the counter is not `1` we continue with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    update drv_all
    set out_repeat = out_repeat - 1
    where hash = ?hash
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    This covers all the use cases at once; note that each as its parameters `h`
    alone.  
    Once again, the last three queries can be combined into something that
    always works,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    update drv_all
    set out_repeat = out_repeat - 1
    where out_hash = ?hash
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    and some vacuuming at regular intervals but not necessarily immediately,

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    delete from drv_all
    where out_repeat = 0
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    the latter may be automated with a trigger using

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    create trigger if not exists drv_all_dropzero
    after update on drv_all
    when new.out_repeat = 0
    begin delete from drv_all
          where out_repeat = 0;
    end
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    The above optimisations are a bit silly though… we do need to know when the
    counter is created/removed because that indicates backend action to be
    invoked.

-   All these queries are made within one transaction.

Static Query Optimisation
-------------------------

-   It is possible to prepare, and thereby optimise their execution, before
    starting the actual application. This is the result of queries being
    generated for the given LDAP structures and their use in a given Pulley
    script.

-   With SQLite3, there is actually no way of not doing this :-)

-   In the implementation, we have finally chosen to not use `?v0` variable
    names, but instead their `?003` variable numbers or `varnum_t` values, as
    provided by the applicable `struct vartab` context.  Printing is done with
    format string `"%03d"` using these tables, and this works consistently
    because renumbering will not occur without re-parsing and re-preparing the
    SQLite3 statements.  The numbers are consistent within a given parse tree
    and semantic analysis.  The tables are the way to crossover between
    versions, and tables use `var_v0` names to represent variables in a manner
    compatible to the Pulley script.

We Might do Better
------------------

**TODO:** Implementation.

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

-   This can probably be worked into the SQLite3 backend as well, although
    clumsily.

Processing LDAP SyncRepl restart
--------------------------------

**TODO:** Implementation.

When LDAP SyncRepl fails to continue, it needs to restart from scratch.  The
following procedure helps to do this without invasive invocations to backend
drivers.

1.  Copy the contents of all `gen_` tables to a temporary `oldgen_` table.

2.  Set the `out_repeats` value to 1 for all entries currently in `drv_all`.

3.  Start the synchronisation from scratch.

4.  Using the `oldgen_` tables, reconstruct all the hashes in `drv_all`.

5.  Lookup `out_repeats` for each hash; when 1, delete the record and inform the
    driver.

6.  There should be no remaining records with `out_repeats` set to 1 in
    `drv_all`.

7.  Subtract 1 from all `out_repeats` in `drv_all`, not notifying drivers when 0
    is reached.

8.  Drop the temporary `oldgen_` tables.

This procedure relies on the availability of the data in all `gen_` tables,
including those that are/have no co-generators.  This is currently optimised
out, but for reliable restart of LDAP SyncRepl without confusing impact on
driver backends, this ought to be present; it is unreliable to download the data
of non-co-generators at some later time, it may have advanced beyond the state
present in `drv_all`.  This is especially likely to become a noticeable problem
when a backend driver can have long-lasting data and LDAP is restarted after a
relatively long period of time.

Processing Script changes
-------------------------

**TODO:** Implementation.

Ideally, a new Pulley script does not mean processing all data from scratch;
this can be especially tiresome during a development cycle on a live system
hosting lots of data.  Quick turnover is key in such situations, so scalability
is valuable.

-   The new Pulley script will result in a new overall lexhash, and end up in a
    new database.

-   We assume that the lexhash of the preceding script version is also known.

-   Matched generators in the script will produce the same `gen_` table names.

-   Such same `gen_` table names can be copied if they exist in the old
    database.

-   New `gen_` table names (new co-gen role?) can be downloaded explicitly.

-   Removed `gen_` table names can be quietly forgotten in the new database.

-   It seems helpful to download non-co-gen `gen_` tables, and erase them after
    updating.

-   Output records cannot be derived from `drv_all` table.

-   Reproducing output from `drv_all` is possible, combining `gen_` (including
    downloads).

A smart procedure may be helpful to migrate `drv_all` to the new database:

1.  Copy `drv_all` from old to new, setting `out_repeats` to 1.

2.  Reproduce all driver records from the `gen_` tables, including downloads, in
    add mode.

3.  Invoke the driver about any new entries that this creates.

4.  Reproduce all output tuples from the old `gen_` tables and downloads.

5.  When an output tuple is set to 1 in `drv_all`, remove it, notifying the
    driver in del mode.

6.  There should be no remaining records with `out_repeats` set to 1 in
    `drv_all`.

7.  Subtract 1 from all remaining `out_repeats` in the `drv_all` table, not
    notifying drivers.

Note that this procedure suggests downloading all records always, even when no
co-generator role is played by a generator.  This might indeed be useful as a
development mode, because it speeds up this script update process, but more
importantly making it more reliable than downloading what happens to be the
record set at that later time into the old database.

General Resync logic
--------------------

The general steps for resynchronisation without overzealous driver stop/start
could be:

1.  Have a clear notion of old and new data sets

2.  Increment the `out_repeats` in `drv_all` by 1, to avoid intermediate driver
    take-down

3.  Add to `out_repeats` in `drv_all` by producing output with new generator
    records

4.  Decrement the `out_repeats` in `drv_all` by 1, which should not cause
    removals

5.  Delete from `out_repeats` in `drv_all` by producing output with old
    generator records

6.  During the last step, when a counter hits 0, go through driver removal
