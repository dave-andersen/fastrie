#!/usr/bin/python

import fermat_test

T=10**15  # one trillion

primorial = 210
offset = 97

# Step 1:  Round up to the nearest multiple of our primorial (210)
# Step 2:  Add in the offset (97)
# Step 3:  Add 210 and test for valid PoW.

remainder = int(T % primorial) #  Remainder after division
round_up_amount = primorial - remainder
candidate = T + round_up_amount
candidate += offset

count = 0

while True:
    count += 1
    if fermat_test.fermat_is_valid_pow(candidate):
        print candidate
        break
    candidate += primorial

print "Took ", count, " tries to find a valid PoW"
