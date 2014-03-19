#!/usr/bin/python

import fermat_test
N=10000000

for candidate in range(17, N, 2):
    if fermat_test.fermat_is_valid_pow(candidate):
        print candidate
