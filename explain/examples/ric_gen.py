#!/usr/bin/python

import basic_sieve_e
import fermat_test

# A "primorial" is the product of the first n primes.  In this
# case, we'll use the 4th primorial:

def candidate_killed_by(candidate, prime):
    for offset in [0, 4, 6, 10, 12, 16]:
        if (candidate+offset) % prime == 0:
            return True
    return False

def add_next_prime(offsets, prime, primorial):
    base = 0
    new_offsets = []
    for rep in range(0, prime):
        for o in offsets:
            val = base + o
            if not candidate_killed_by(val, prime):
                new_offsets.append(val)
        base += primorial

    return new_offsets


primorial_max = 19

primorial_start = 7
primorial = 210  # Start with the one we figured out earlier: 2*3*5*7
offsets = [97]

for i in basic_sieve_e.genprime(primorial_max):
    if i <= primorial_start:
        continue

    offsets = add_next_prime(offsets, i, primorial)
    primorial *= i

for o in offsets:
    if fermat_test.fermat_is_valid_pow(o):
        print o

    
