#!/bin/bash
run_test() {
    local formula="$1"
    local x_start="$2"
    local x_end="$3"
    local y_start="$4"
    local y_end="$5"

    echo "---------------------------------------------------------------------------------------------------"
    echo "Testing formula: $formula"
    
    cd ..
    bin/geneMPFR.exe "$formula"
    cd pdmethod && make

    echo "Warming up with 5 runs..."
    for i in {1..5}; do
        echo "Warm-up run $i"
        bin/FPEPD.exe $x_start $x_end $y_start $y_end > /dev/null
    done

    echo -e "\nStarting timed runs..."
    declare -a times
    total=0

    for i in {1..10}; do
        time_output=$( { time bin/FPEPD.exe $x_start $x_end $y_start $y_end > /dev/null; } 2>&1 )
        runtime=$(echo "$time_output" | grep real | awk '{print $2}' | sed 's/0m\(.*\)s/\1/')
        
        times[$i]=$runtime
        total=$(echo "$total + $runtime" | bc -l)
        
        echo "Run $i: $runtime seconds"
    done

    average=$(echo "scale=6; $total / 10" | bc -l)
    echo -e "\nAverage execution time: $average seconds"
    bin/FPEPD.exe $x_start $x_end $y_start $y_end
}

formulas=(
    "((-12.0 * x1) - (7.0 * x2)) + x2 * x2"
    "(x1 * x1 + x2 - 11.0) * (x1 * x1 + x2 - 11.0) + (x1 + x2 * x2 - 7.0) * (x1 + x2 * x2 - 7.0)"
    "atan(x2 / x1) * (180.0 / 3.14159265359)"
    "sin(x1 + x2) - sin(x1)"
    "sqrt(x1 + x2 * x2)"
    "sin(x1 * x2)"
    "(x1 + x2) / (x1 - x2)"
    "x1 * cos(x2 * (3.14159265359 / 180.0))"
    "x1 * sin(x2 * (3.14159265359 / 180.0))"
    "pow((x1 + 1.0), (1.0 / x2)) - pow(x1, (1.0 / x2))"
    "0.5 * sqrt((2.0 * (sqrt(x1 * x1 + x2 * x2) + x1)))"
    "(0.5 * sin(x1)) * (exp(-x2) - exp(x2))"
    "x1 + (((((((((2.0 * x1) * (((((3.0 * x1) * x1) + (2.0 * x2)) - x1) / ((x1 * x1) + 1.0))) * ((((((3.0 * x1) * x1) + (2.0 * x2)) - x1) / ((x1 * x1) + 1.0)) - 3.0)) + ((x1 * x1) * ((4.0 * (((((3.0 * x1) * x1) + (2.0 * x2)) - x1) / ((x1 * x1) + 1.0))) - 6.0))) * ((x1 * x1) + 1.0)) + (((3.0 * x1) * x1) * (((((3.0 * x1) * x1) + (2.0 * x2)) - x1) / ((x1 * x1) + 1.0)))) + ((x1 * x1) * x1)) + x1) + (3.0 * (((((3.0 * x1) * x1) - (2.0 * x2)) - x1) / ((x1 * x1) + 1.0))))"
    "(((333.75 * pow(b, 6.0)) + (pow(a, 2.0) * (((((11.0 * pow(a, 2.0)) * pow(b, 2.0)) - pow(b, 6.0)) - (121.0 * pow(b, 4.0))) - 2.0))) + (5.5 * pow(b, 8.0))) + (a / (2.0 * b))"
)

ranges=(
    "0 2 0 3"
    "-5 5 -5 5"
    "1 100 1 100"
    "0.01 100 0.01 100"
    "0.1 10 -5 5"
    "0.1 10 -5 5"
    "0 1 -1 -0.1"
    "1 10 0 360"
    "1 10 0 360"
    "0.01 100 0.01 100"
    "0.01 100 0.01 100"
    "0.01 100 0.01 100"
    "-5 5 -20 5"
    "0.01 100 0.01 100"
)

for i in "${!formulas[@]}"; do
    range=(${ranges[$i]})
    run_test "${formulas[$i]}" "${range[0]}" "${range[1]}" "${range[2]}" "${range[3]}"
done