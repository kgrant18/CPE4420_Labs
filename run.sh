IT=140
L=optimized4T
CMD="./sobel_optimized --threads 4 --iterations $IT"

vcgencmd measure_temp 
vcgencmd get_throttled

$CMD > /dev/null

perf stat -r 5 -e cycles:u,instructions:u                      -- $CMD > results/${L}_cycles_instructions.txt 2>&1
#perf stat -r 5 -e cache-references:u,cache-misses:u            -- $CMD > results/${L}_cache.txt 2>&1
perf stat -r 5 -e task-clock,context-switches,cpu-migrations,page-faults -- $CMD > results/${L}_os_activity.txt 2>&1

vcgencmd measure_temp
vcgencmd get_throttled


