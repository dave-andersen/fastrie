#!/usr/bin/python

def fermat_prime_p(n):
    if n == 2:
        return True
    if not n & 1:
        return False
    if pow(2, n-1, n) == 1:
        return True
    return False
