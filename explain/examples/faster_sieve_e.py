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

  for i in range(3, n+1, 2):
      if (isprime[i/2]):
          yield i
          if (i < sN):
              # Skip multiples of 2 and 3 -- they're already 
              # marked off.  Alternate adding i*2 and i*4
              # by having 'skipthree' change from 2 to 4 and back
              # (2 xor 6 == 4,  4 xor 6 == 2...)
              ni = 5*i
              skipthree = 0x2;
              while (ni <= n):
                  isprime[ni/2] = False
                  ni += skipthree*i
                  skipthree = (skipthree ^ 0x6)

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
