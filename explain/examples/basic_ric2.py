#!/usr/bin/python

import faster_sieve_e

N=10000000

for candidate in faster_sieve_e.genprime(N):
    # Skip the first spots until our sieve will have eliminated
    # invalid candiates.  Only needed because we're starting small.
    if (candidate > 17 and candidate < N-17):
        if (faster_sieve_e.sieve_is_valid_pow(candidate)):
            print candidate
