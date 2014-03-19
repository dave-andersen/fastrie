#!/usr/bin/python

##
# Building block:  A very basic Sieve of Eratosthenes
# that generates primes up to n.
##

import math

isprime = [True]

def genprime(n):
  global isprime
  isprime = [True] * (n+1)
  sN = int(math.floor(math.sqrt(n)))

  for i in range(3, n+1, 2): 
      if (isprime[i]):
          yield i
          if (i < sN):
              ni = 2*i
              while (ni <= n):
                  isprime[ni] = False
                  ni += i

def is_prime_p(n):
  global isprime
  return isprime[n]

if __name__ == "__main__":
    print list(genprime(10))
