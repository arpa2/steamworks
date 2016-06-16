# Pulley Backend API

> *Pulley has a pluggable backend, where each plugin can be used to deliver
> the tuples being added or deleted over a transaction.  The API below is
> used to do just that, so it is implemented by backend plugins and used by
> Pulley.*


## Plugins in the File System

Plugins are loadable dynamic objects; on POSIX systems, they will take the
form of `.so` files, while on Windows they become so-called `.dll` files.

Plugins are loaded from a fixed directory, which helps packagers to construct
packages with specific plugins; such packages would require Pulley to be
installed and they would simply add the plugin to the said directory.

Though the storage directory for plugins may vary across distributions,
we suggest to use `/usr/share/steamworks/pulleyback/` as a default.


## Initialisation and Cleanup

Once the library is loaded, its dynamic symbols are resolved and can be
invoked.  This is done through standard dynamic library support with
`dlopen()`, `dlsym()` and `dlclose()`.  Some of the symbols may be
optional, but only when explicitly noted in the text below.

Within the library, we can open any number of instances of a given backend,
normally in response to corresponding lines in the Pulley Script.  The
opening and closing calls for instances are:

    void *pulleyback_open (int argc, char **argv, int varc);
    void pulleyback_close (void *pbh);

The `argc` and `argv` arguments to `pulleyback_open()` are similar in style
to `main()`, where the values passed in come from the command line in the
Pulley Script, and more specifically from the instantiation of the driver.
The argument `varc` gives the number of variables that are passed to the
driver for addition or removal of variables.  This number is mentioned
explicitly to permit for error checking.  TODO:TYPING?
The function returns a pulley-back handle as a pointer, or it returns NULL and
sets `errno` to indicate failure.

The argument to `pulleyback_close()` is a pulley-back handle as obtained
from `pulleyback_open()`.  In a proper program execution, every succeeded
call to `pulleyback_open()` should be matched by one later call to
`pulleyback_close()` and there should be no other invocations to the latter.


## Adding and Removing Forks

The primary function of a backend is to have forks added to and removed from
an instance.  This is done with the respective functions

    int pulleyback_add (void *pbh, uint8_t **forkdata);
    int pulleyback_del (void *pbh, uint8_t **forkdata);

These functions return 1 on success and 0 on failure; any failure status
is retained within an instance, and reported for future additions and
removals of forks, as well as for attempts to commit the transaction.
Because none of the changes is made instantly, they are stored as part
of a current transaction, which is always implicitly open.

The first parameter to the calls is an open pulley-back instance handle,
the second
points to an array of data fields describing the fork.  Guards are not
passed down when they are not also mentioned as parameters, because they
are handled inside Pulley.

Note that Pulley keeps track of the count of guards for a given set of
`forkdata` values.  It will avoid invoking `pulleyback_add()` more than
once on the same `forkdata` without first having called `pulleyback_del()`
on it.  It will also avoid calling `pulleyback_del()` on `forkdata` values
unless they have been added with `pulleyback_add()` and since not removed by
`pulleyback_del()`.  These statements do apply over a sequence of sessions,
each of which is marked by loading and unloading the backend plugin module.

Finally, a call exists to clear out an entire database, so it can be
filled from scratch:

    int pulleyback_reset (void *pbh);

This will result in all data being deleted, as part of the currently
ongoing transaction.  Since this does not match what is being shown
externally, it is possible to rebuild the database without glitches on
the data that has not changed.  It can be a great help with error recovery
and other resynchronisation operations.


## Transaction Processing Support

Transactions are used in the Pulley Backend to release all information
at the same time.  This makes it possible, for instance, to remove something
and add it again, without it disappearing from the external view.

Ideally, all backend plugins would have two-phase commit facilities, but
not all underlying structures will be able to support this.  It is
possible to achieve the same level of certainty with any number of
two-phase and a single one-phase backend, so it is useful to detect
a backend's support for two-phase commit.  We do this by checking if
the dynamic symbol for prepare-to-commit resolves.

The following API functions support transactions on an open instance:

    int pulleyback_prepare   (void *pbh);  /* OPTIONAL */
    int pulleyback_commit    (void *pbh);
    void pulleyback_rollback (void *pbh);

The functions implement prepare-to-commit, commit and rollback, respectively.
When only single-phase commit is supported, then `pulleback_prepare()` will
not resolve, which is permitted as it is marked optional.  The result from
`pulleyback_prepare()` predicts the success of an upcoming
`pulleyback_commit()`, but still makes it possible to run
`pulleyback_rollback()` instead.  When `pulleyback_prepare()` succeeds
then the following `pulleyback_commit()` must also succeed; in fact, the
calling application is under no obligation to check the result in that case.

The normal Pulley sequence is to perform `pulleyback_prepare()` on all
backends, and when all succeed to run `pulleyback_commit()` on them,
and otherwise run `pulleyback_rollback()` on all of them.

It is permitted to invoke `pulleyback_rollback()` or `pulleyback_commit()`
on an instance
without prior call to `pulleback_prepare()`, in which case their outcome
is effective immediately, and the update is done as atomically as possible.

It is not permitted to invoke anything but `pulleyback_close()`,
`pulleyback_commit()` or `pulleyback_rollback()` on an instance
after `pulleyback_prepare()` has been invoked.

Note that `pulleyback_close()` does an implicit `pulleyback_rollback()`
on any outstanding changes.  Please do not rely on this though, it is
only there as a stop-gap measure for unexpected program termination.


