#!/usr/bin/python

import basic_sieve_e

# A "primorial" is the product of the first n primes.  In this
# case, we'll use the 4th primorial:

primorial = 2*3*5*7 # This equals 210

isCandidate = [True] * primorial

# Kill any location that is divisible by the primes in our primorial
for i in [2, 3, 5, 7]:
    ni = i
    while ni < (primorial+16):
        for offset in [0, 4, 6, 10, 12, 16]:
            if (ni-offset > 0 and ni-offset < primorial):
                isCandidate[ni-offset] = False
        ni += i

for i in range(3, primorial):
    if isCandidate[i]:
        print i

# The result:  With primorial 210, only +97 is not killed by
# one of 2, 3, 5, or 7.
