//
// Created by 99065 on 2024/3/26.
//
#include "function.h"
#include "getresult.h"

#define SAMPLES 512
#define INITIAL_SAMPLES 1000
#define THRESHOLD 1.0e-11
std::mutex mtx1;

std::vector<double> generateMantissaVariants(double x, int count) {
    std::vector<double> variants;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);

    uint64_t intRepresentation = *reinterpret_cast<uint64_t*>(&x);
    uint64_t signAndExponentMask = 0xFFF0000000000000ULL;

    for (int i = 0; i < count; ++i) {
        uint64_t newMantissa = dis(gen);
        uint64_t newRepresentation = (intRepresentation & signAndExponentMask) | newMantissa;
        double newX = *reinterpret_cast<double*>(&newRepresentation);
        variants.push_back(newX);
    }
    return variants;
}

void processInterval(double a, double b, int samples, vector<IntervalResult> &results) {
    double totalUlp = 0.0;
    double maxUlp = 0.0;
    double maxUlpPoint = 0.0;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(a, b);

    std::vector<std::pair<double, double>> errors;

    for (int i = 0; i < INITIAL_SAMPLES; ++i) {
        double x = dis(gen);
        double origin = getDoubleOfOrigin(x);
        double ulp = getULP(x, origin);
        errors.push_back({x, ulp});
    }

    std::sort(errors.begin(), errors.end(), [](const std::pair<double, double> &lhs, const std::pair<double, double> &rhs) {return lhs.second > rhs.second;} );

    for (int i = 0; i < 10 && i < errors.size(); ++i) {
        auto variants = generateMantissaVariants(errors[i].first, samples);
        for (auto variant : variants) {
            double origin = getDoubleOfOrigin(variant);
            double ulp = getULP(variant, origin);
            totalUlp += ulp;
            if (ulp > maxUlp) {
                maxUlp = ulp;
                maxUlpPoint = variant;
            }
        }
    }
    
    double avgUlp = totalUlp / (10 * (double)samples);
    {
        std::scoped_lock lock(mtx1);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {a, b}});
    }
}

void detectAndReOne(double x_l, double x_r, int samples, int SIZE, double threshold, bool isFinalIteration = false) {
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
        detectAndReOne(maxIt->interval.first, maxIt->interval.second, samples, SIZE, threshold, false);
    } else if (!isFinalIteration) {
        detectAndReOne(x_l, x_r, samples, SIZE, threshold, true);
    }
}

void detect1(double x1_l, double x1_r) {
    int SIZE = 0;
    int range = (int)(x1_r - x1_l);
    if (range <= 10000) SIZE = 100;
    else SIZE = ceil(sqrt(range));
    detectAndReOne(x1_l, x1_r, SAMPLES, SIZE, THRESHOLD);
}