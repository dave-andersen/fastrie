#!/usr/bin/python

import fermat_test
N=10000000

def fermat_is_valid_pow(n):
    for offset in [0, 4, 6, 10, 12, 16]:
        if not fermat_test.fermat_prime_p(n+offset):
            return False
    return True

for candidate in range(17, N, 2):
    if fermat_is_valid_pow(candidate):
        print candidate
