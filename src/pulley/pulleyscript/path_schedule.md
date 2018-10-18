# Generating Paths of Least Resistence

## Approach 1.  Exhaustive

**Given:**

-   Every condition c\_i reduces by an estimated factor f\_i

-   Every generator g\_j produces estimated new tuples r\_j

-   Let D\_i be the set of generators with variables overlapping c\_i

-   Generators that need to run are in G

-   Conditions that need to run are in C

**Requested:**

-   An ordering of G,C elements that runs fast…

    -   …meaning, order to have as little nesting as needed

    -   …or having the smallest factors f\_i and r\_j first

**Solution:**

-   Determine a total factor t\_i for all c\_i in C

-   Incorporate generators that still need to run: D\_i & G and name it
    R\_i

-   Calculate t\_i as the product of f\_i and all the r\_j for g\_j in
    R\_i

-   Start with the c\_i that has the lowest t\_i (will have as few
    generators as possible)

-   Order the R\_i by ascending g\_i to find its running order, follow
    immediately by c\_i

-   Remove c\_i from C; remove R\_i from G

-   While G non-empty iterate back to top

-   Order remaining c\_i in C by ascending f\_i to find its running
    order

-   Now C can be made/considered empty

**Questions:**

-   Is this order the same for all did-run generators?

    -   …no, because it might be very heavy but promote a timid g\_i and
        a major c\_i

    -   …so, need to do this depending on the initial generator that ran

    -   …would be relatively simple to do this for all output drivers in
        parallel

-   What is the set G and C that is activated in the beginning?

    -   First look at a generator that coughed up new data

    -   What writers does it influence? Find all variables

    -   For the variables, find all conditions C (to apply to the *new*
        data)

    -   For the variables, find all overlapping G (to run *on top of*
        the coughing one)

        -   Note that the originating generator will not be included!

-   Can these *paths of least resistence* be statically pre-calculated?

    -   Yes, why not?

    -   It probably is a very good idea, saving time on small increments
        from LDAP

    -   Very similar to SQL language preprocessing… :-)

    -   Except it is not part of the interactive loop that the user
        waits on :-D

## Approach 2. Estimation

-   Static analysis *O(V\*G)*: Determine the cheapest generator for each
    variable

-   For each initiating generator *g*, determine the Path of Least
    Resistence *p(g)*

    -   Determine the conditions *C* that are impacted by the variables
        that *g* generates

    -   Determine the variables *V* that are needed for all the
        conditions in *C*

    -   Determine the generators *G* that are needed to generate all
        variables in *V*

    -   Determine the drivers *D* that are impacted by newly generated
        variables in *V*

    -   Remove the variables that *g* generates from *V*

    -   Loop as long as *C* is non-empty, aim to add the cheapest
        condition to *p(g)*

        -   Collect, for all variables in *V* that are needed for *c*,
            the generation cost

        -   Order the variables by ascending generation cost

        -   Starting at the beginning, find the prefix of generators
            that is required for *c*

        -   Expand *p(g)* with the prefix of generators for *c.min*, and
            then *c.min*

        -   Remove the prefix of generators for *c.min* from *G*

        -   Remove *c.min* from *C*

    -   Order any remaining elements in *G* by their cost, append that
        to *p(g)*

    -   Insert the drivers in *D*

        -   Take care to invoke them only for variables that have
            changed

-   Total analysis complexity is *O(G\*C\*V)* due to the three nested
    loops

-   Complexity is worst for tightly connected partitions; independent
    variables as lighter


