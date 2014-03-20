#!/usr/bin/python

import fermat_test
import sys

N=10000000
if len(sys.argv) > 1:
    N = int(sys.argv[1])

do_output = False

count = 0

for candidate in range(17, N, 2):
    if fermat_test.fermat_is_valid_pow(candidate):
        count += 1
        if do_output:
            print candidate

print "Total of ", count, " prime sextuplets"
