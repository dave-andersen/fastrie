#!/usr/bin/python

import basic_sieve_e
import sys
do_output = False

N=10000000
if len(sys.argv) > 1:
    N = int(sys.argv[1])

count = 0

for candidate in basic_sieve_e.genprime(N):
    # Skip the first spots until our sieve will have eliminated
    # invalid candiates.  Only needed because we're starting small.
    if (candidate > 17 and candidate < N-17):
        if (basic_sieve_e.sieve_is_valid_pow(candidate)):
            count += 1
            if do_output:
                print candidate

print "Total of ", count, " prime sextuplets"
