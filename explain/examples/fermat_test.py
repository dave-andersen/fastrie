#!/usr/bin/python

def fermat_prime_p(n):
    if n == 2:
        return True
    if not n & 1:
        return False
    if pow(2, n-1, n) == 1:
        return True
    return False

def fermat_is_valid_pow(n):
    for offset in [0, 4, 6, 10, 12, 16]:
        if not fermat_prime_p(n+offset):
            return False
    return True
