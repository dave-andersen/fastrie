#!/usr/bin/python

import math

with open('blocks_trimmed','r') as f:
  for line in f:
    line = line.strip()
    block, val = line.split()
    intval = int(val, 16)
    logval = int(math.ceil(math.log(intval, 2)))
    print block, logval
