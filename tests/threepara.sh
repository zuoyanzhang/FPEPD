#!/bin/bash
run_test() {
    local formula="$1"
    local x_start="$2"
    local x_end="$3"
    local y_start="$4"
    local y_end="$5"
    local z_start="$6"
    local z_end="$7"

    echo "---------------------------------------------------------------------------------------------------"
    echo "Testing formula: $formula"
    
    cd ..
    bin/geneMPFR.exe "$formula"
    cd pdmethod && make

    echo "Warming up with 5 runs..."
    for i in {1..5}; do
        echo "Warm-up run $i"
        bin/FPEPD.exe $x_start $x_end $y_start $y_end $z_start $z_end > /dev/null
    done

    echo -e "\nStarting timed runs..."
    declare -a times
    total=0

    for i in {1..10}; do
        time_output=$( { time bin/FPEPD.exe $x_start $x_end $y_start $y_end $z_start $z_end > /dev/null; } 2>&1 )
        runtime=$(echo "$time_output" | grep real | awk '{print $2}' | sed 's/0m\(.*\)s/\1/')
        
        times[$i]=$runtime
        total=$(echo "$total + $runtime" | bc -l)
        
        echo "Run $i: $runtime seconds"
    done

    average=$(echo "scale=6; $total / 10" | bc -l)
    echo -e "\nAverage execution time: $average seconds"
    bin/FPEPD.exe $x_start $x_end $y_start $y_end $z_start $z_end
}

formulas=(
    "(-(331.4 + (0.6 * x3)) * x2) / (((331.4 + (0.6 * x3)) + x1) * ((331.4 + (0.6 * x3)) + x1))"
    "(((x0 + x1) - x2) + ((x1 + x2) - x0)) + ((x2 + x0) - x1)"
    "((-(x1 * x2) - ((2.0 * x2) * x3)) - x1) - x3"
    "((((((2.0 * x1) * x2) * x3) + ((3.0 * x3) * x3)) - (((x2 * x1) * x2) * x3)) + ((3.0 * x3) * x3)) - x2"
    "((3.0 + (2.0 / (x3 * x3))) - (((0.125 * (3.0 - (2.0 * x1))) * (((x2 * x2) * x3) * x3)) / (1.0 - x1))) - 4.5"
    "((6.0 * x1) - (((0.5 * x1) * (((x2 * x2) * x3) * x3)) / (1.0 - x1))) - 2.5"
    "((3.0 - (2.0 / (x3 * x3))) - (((0.125 * (1.0 + (2.0 * x1))) * (((x2 * x2) * x3) * x3)) / (1.0 - x1))) - 0.5"
    "(((x0 + x1) - x2) + ((x1 + x2) - x0)) + ((x2 + x0) - x1)"
)

ranges=(
    "-100 100 20 20000 -30 50"
    "1 2 1 2 1 2"
    "-15 15 -15 15 -15 15"
    "-15 15 -15 15 -15 15"
    "-4.5 -0.3 -2.5 0.9 3.8 7.8"
    "-4.5 -0.3 -2.5 0.9 3.8 7.8"
    "-4.5 -0.3 -2.5 0.9 3.8 7.8"
    "1 2 1 2 1 2"
)

for i in "${!formulas[@]}"; do
    range=(${ranges[$i]})
    run_test "${formulas[$i]}" "${range[0]}" "${range[1]}" "${range[2]}" "${range[3]}" "${range[4]}" "${range[5]}"
done