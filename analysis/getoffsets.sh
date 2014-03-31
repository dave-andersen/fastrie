#!/bin/bash
RIE=~/coin/rie/riecoind
START=`tail -1 blocks_trimmed | awk '{print $1}'`
START=`expr $START + 1`
END=`$RIE getmininginfo | grep blocks | head -1 | awk '{print $3}' | sed 's/,//'`


for block in $(seq $START $END)
do
echo -n "$block "
$RIE getblock `$RIE getblockhash $block` | grep nOffset | awk '{print $3}' | sed 's/"//g' | sed 's/,//'
done
