#!/usr/bin/python

import math

N=100000


def genprime(n):
  isprime = [True] * (N+1)
  sN = int(math.floor(math.sqrt(n)))

  for i in range(3, n+1, 2): 
      if (isprime[i]):
          yield i
          if (i < sN):
              ni = 2*i
              while (ni <= n):
                  isprime[ni] = False
                  ni += i

print list(genprime(10))
