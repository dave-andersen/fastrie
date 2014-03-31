set key top left
set xlabel "Block number"
set ylabel "Log2(offset)"
set y2label "Difficulty"
set y2tics
set terminal png medium
set label "Ypool Rel" at 2500,110
set label "a00k/dga Rel" at 20000,280
set output "newgraph.png"
plot "diffs.txt" with lines axis x1y2 title "Difficulty", "block_logs.txt" with dots axis x1y1 title "Log2(offset)"

