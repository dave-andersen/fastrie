#!/usr/bin/python

import basic_sieve_e

N=10000000

def sieve_is_valid_pow(n):
    for offset in [0, 4, 6, 10, 12, 16]:
        if not basic_sieve_e.is_prime_p(n+offset):
            return False
    return True

for candidate in basic_sieve_e.genprime(N):
    # Skip the first spots until our sieve will have eliminated
    # invalid candiates.  Only needed because we're starting small.
    if (candidate > 17 and candidate < N-17):
        if (sieve_is_valid_pow(candidate)):
            print candidate
