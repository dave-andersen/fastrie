#!/usr/bin/python

import faster_sieve_e
import fermat_test
import array
import sys
import invert_mod

#T=10**15  # one trillion
# Target from first block in the Riecoin blockchain
T=0x801A2F60588BF10BB614D6796A726025F88C7156E3FBDF68685FC0617F4266358C0000000000
print T
tuple_offsets = [0, 4, 6, 10, 12, 16]
Pn = 7
primorial = 1
for p in faster_sieve_e.genprime(Pn):
    primorial *= p

offset = 97 # Only works up to Pn < 97!

# Step 1:  Round up to the nearest multiple of our primorial
# Step 2:  Add in the offset

remainder = int(T % primorial) #  Remainder after division
round_up_amount = primorial - remainder
start_candidate = T + round_up_amount
start_candidate += offset

count = 0

# Step 3:  Generate a sieve relative to the primorial.
sievesize = 1024*512
max_sieve_prime = 1000000
primes = array.array('I')
sieve_loc = array.array('I')

for p in faster_sieve_e.genprime(max_sieve_prime):
    if (p <= Pn):
        continue
    primes.append(p)
    inverse = invert_mod.modinv(primorial, p)
    for o in tuple_offsets:
        candidate_plus = start_candidate + o
        candidate_mod_p = candidate_plus % p
        index = ((p - candidate_mod_p)*inverse)%p
        sieve_loc.append(index)

def notworking():
    inverse = invert_mod.modinv(primorial, p)
    candidate_mod_p = start_candidate % p

    for o in tuple_offsets:
        offset_remainder = (candidate_mod_p + o) % p
        if p > offset_remainder:
            offset_remainder = p - offset_remainder
        else:
            offset_remainder = -offset_remainder
        i = ((offset_remainder%p)*inverse) % p
        sieve_loc.append(i)

sieve = [True] * sievesize
loop_count = 0

while True:

    # Step 1:  Filter the sieve using sieve_loc
    sieve_loc_index = 0
    print "Sieving"
    for p in primes:
        for i in range(0,6):
            o = sieve_loc[sieve_loc_index]
            while o < sievesize:
                sieve[o] = False
                o += p
            sieve_loc[sieve_loc_index] = o - sievesize
            sieve_loc_index += 1

    # Step 2:  Test candidates
    print "Testing"

    for n in range(0, sievesize):
        if sieve[n]:
            candidate = start_candidate + (loop_count * sievesize + n) * primorial
            if fermat_test.fermat_is_valid_pow(candidate):
                print "Found: ", candidate
                print "Offset from target: ", (candidate-T)
                sys.exit(0)
        sieve[n] = True # resetting sieve behind us
    loop_count += 1
