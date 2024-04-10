//
// Created by 99065 on 2024/3/26.
//
#include "function.h"
#include "getresult.h"
#define SIZE 128
#define SAMPLES 512
#define THRESHOLD 0.00000000001
std::mutex mtx1;

void processInterval(double a, double b, int samples, vector<IntervalResult> &results) {
    double totalUlp = 0.0;
    double maxUlp = 0.0;
    double maxUlpPoint = 0.0;
    double step = (b - a) / (double)samples;
    for (int i = 0; i < samples; i++) {
        double x = a + i * step;
        double origin = getDoubleOfOrigin(x);
        double ulp = getULP(x, origin);
//        double ulp = getRelativeError(x, origin);
        totalUlp += ulp;
        if (ulp > maxUlp) {
            maxUlp = ulp;
            maxUlpPoint = x;
        }
    }
    double avgUlp = totalUlp / (double)samples;
    {
        std::scoped_lock lock(mtx1);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {a, b}});
    }
}

void detectAndReOne(double x_l, double x_r, int samples, double threshold, bool isFinalIteration = false) {
    vector<IntervalResult> results;
    vector<thread> threads;
    for (int i = 0; i < SIZE; ++i) {
        double sub_a = x_l + (x_r - x_l) * i / (double)SIZE;
        double sub_b = x_l + (x_r - x_l) * (i + 1) / (double)SIZE;
        threads.push_back(thread(processInterval, sub_a, sub_b, samples, ref(results)));
    }
    for (auto &t : threads) {
        t.join();
    }
    auto maxIt = max_element(results.begin(), results.end(), [](const IntervalResult &lhs, const IntervalResult &rhs) {
        return lhs.avgUlp < rhs.avgUlp;
    });
//    std::cout << "Maximum Average Relative Error: " << maxIt->avgUlp
//              << " in X [" << maxIt->interval.first << ", " << maxIt->interval.second << "]" << std::endl;
    if (isFinalIteration) {
        printf("Final Iteration - Maximum ULP Error: %.2lf, at X: %.16lf, and Bit Error: %.2lf\n", maxIt->maxUlp, maxIt->maxUlpPoint, log2(maxIt->maxUlp + 1));
    }
    double x_len = fabs(maxIt->interval.second - maxIt->interval.first);
    if (x_len > threshold) {
        detectAndReOne(maxIt->interval.first, maxIt->interval.second, samples, threshold, false);
    } else if (!isFinalIteration) {
        detectAndReOne(x_l, x_r, samples, threshold, true);
    }
}

void detect1(double x1_l, double x1_r) {
    detectAndReOne(x1_l, x1_r, SAMPLES, THRESHOLD);
}