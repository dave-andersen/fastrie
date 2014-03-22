#!/usr/bin/python

##
# Building block:  A slightly more optimized Sieve of Eratosthenes
# that generates primes up to n.
##

import math

isprime = [True]

def genprime(n):
  global isprime
  isprime = [True] * ((n+2)/2)
  sN = int(math.floor(math.sqrt(n)))
  for nI in range(9, n, 6):
    isprime[nI>>1] = False

  yield 2
  yield 3

  for i in range(5, n+1, 2):
      if (isprime[i>>1]):
          yield i
          if (i < sN):
              # Skip multiples of 2 and 3 -- they're already 
              # marked off.  Alternate adding i*2 and i*4
              # by having 'skipthree' change from 1 to 2 and back
              # and shifting left by that amount to multiply.
              # (1 xor 1 == 2,  2 xor 3 == 1...)
              ni = 5*i
              skipthree = 0x1;
              while (ni <= n):
                  isprime[ni>>1] = False
                  ni += i<<skipthree
                  skipthree = (skipthree ^ 0x3)

def is_prime_p(n):
  global isprime
  if (n & 1 == 0):
      return False
  return isprime[n/2]

def sieve_is_valid_pow(n):
  global isprime
  if (n & 1 == 0):
      return False
  for offset in [0, 4, 6, 10, 12, 16]:
    if not isprime[(n+offset)/2]:
      return False
  return True

if __name__ == "__main__":
    print list(genprime(50))
