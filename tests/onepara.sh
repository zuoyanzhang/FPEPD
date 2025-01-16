#!/bin/bash
run_test() {
    local formula="$1"
    local range_start="$2"
    local range_end="$3"

    echo "---------------------------------------------------------------------------------------------------"
    echo "Testing formula: $formula"
    
    cd ..
    bin/geneMPFR.exe "$formula"
    cd pdmethod && make

    echo "Warming up with 5 runs..."
    for i in {1..5}; do
        echo "Warm-up run $i"
        bin/FPEPD.exe $range_start $range_end > /dev/null
    done

    echo -e "\nStarting timed runs..."
    declare -a times
    total=0

    for i in {1..10}; do
        time_output=$( { time bin/FPEPD.exe $range_start $range_end > /dev/null; } 2>&1 )
        runtime=$(echo "$time_output" | grep real | awk '{print $2}' | sed 's/0m\(.*\)s/\1/')
        
        times[$i]=$runtime
        total=$(echo "$total + $runtime" | bc -l)
        
        echo "Run $i: $runtime seconds"
    done

    average=$(echo "scale=6; $total / 10" | bc -l)
    echo -e "\nAverage execution time: $average seconds"
    bin/FPEPD.exe $range_start $range_end
}

formulas=(
    "1.0 + 0.5 * x - 0.125 * x * x + 0.0625 * x * x * x - 0.0390625 * x * x * x * x"
    "1.0 / (sqrt(x + 1.0) + sqrt(x))"
    "(exp(x) - 1.0) / x"
    "(exp(x) - 1.0) / log(exp(x))"
    "exp(x) - 1.0"
    "log(x + 1.0) - log(x)"
    "1.0 / x - 1.0 / tan(x)"
    "sqrt((exp(2 * x) - 1.0) / (exp(x) - 1.0))"
    "exp(x) / (exp(x) - 1.0)"
    "(x - sin(x)) / (x - tan(x))"
    "exp(x) - 2.0 + exp(-x)"
    "(4.0 * x) / (x / 1.11 + 1.0)"
    "(4.0 * x * x) / (1 + x / 1.11 * x / 1.11)"
    "log(exp(x) - 1.0)"
    "x - (x * x * x) / 6.0 + (x * x * x * x * x) / 120.0 - (x * x * x * x * x * x * x) / 5040.0"
    "((35000000.0 + ((0.401 * (1000.0 / x)) * (1000.0 / x))) * (x - (1000.0 * 4.27e-5))) - ((1.3806503e-23 * 1000.0) * 300.0)"
    "(1.0 - cos(x)) / (x * x)"
    "(((x + 1.0) * log(x + 1.0)) - (x * log(x))) - 1.0"
    "cbrt(x + 1) - cbrt(x)"
    "((1.0 / (x + 1.0)) - (2.0 / x)) + (1.0 / (x - 1.0))"
    "1.0 / (x + 1) - (1.0 / x)"
    "1.0 / sqrt(x) - 1.0 / sqrt(x + 1)"
    "1.0 / tan(x + 1) - 1.0 / tan(x)"
    "(1.0 - cos(x)) / sin(x)"
    "sqrt(x + 1) - sqrt(x)"
    "(x - 1) / (x * x - 1)"
    "1.0 / (x + 1.0)"
    "x / ( x + 1)"
    "(0.954929658551372 * x) - (0.12900613773279798 * ((x * x) * x))"
    "(-x * x * x) / 6.0"
    "log(1 - x) / log(1 + x)"
    "log((1 - x) / (1 + x))"
)

ranges=(
    "0 1"
    "1 1000"
    "0.01 0.5"
    "0.01 0.5"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.1 0.3"
    "0.1 0.3"
    "0.01 8"
    "-1.57079632679 1.57079632679"
    "0.1 0.5"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0.01 100"
    "0 100"
    "1.00001 2"
    "1.00001 2"
    "1 999"
    "-2 2"
    "0 1"
    "0.001 1"
    "0.001 1"
)

for i in "${!formulas[@]}"; do
    range=(${ranges[$i]})
    run_test "${formulas[$i]}" "${range[0]}" "${range[1]}"
done
