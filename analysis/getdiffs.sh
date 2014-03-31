#!/bin/bash
RIE=~/coin/rie/riecoind
START=`tail -1 diffs.txt | awk '{print $1}'`
START=`expr $START + 1`
END=`tail -1 blocks_trimmed | awk '{print $1}'`

for block in $(seq $START $END)
do
echo -n "$block "
$RIE getblock `$RIE getblockhash $block` | grep diff | awk '{print $3}' | sed 's/,//'
done
