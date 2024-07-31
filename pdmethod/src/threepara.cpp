//
// Created by 99065 on 2024/3/26.
//
#include "function.h"
#include "getresult.h"

#define SAMPLES 512
#define INITIAL_SAMPLES 1000
#define THRESHOLD 1.0e-11

std::mutex mtx3;

std::vector<std::tuple<double, double, double>> generateMantissaVariants(double x1, double x2, double x3, int count) {
    std::vector<std::tuple<double, double, double>> variants;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);

    uint64_t intRepresentation1 = *reinterpret_cast<uint64_t*>(&x1);
    uint64_t intRepresentation2 = *reinterpret_cast<uint64_t*>(&x2);
    uint64_t intRepresentation3 = *reinterpret_cast<uint64_t*>(&x3);
    uint64_t signAndExponentMask1 = 0xFFF0000000000000ULL;
    uint64_t signAndExponentMask2 = 0xFFF0000000000000ULL;
    uint64_t signAndExponentMask3 = 0xFFF0000000000000ULL;

    for (int i = 0; i < count; ++i) {
        uint64_t newMantissa1 = dis(gen);
        uint64_t newMantissa2 = dis(gen);
        uint64_t newMantissa3 = dis(gen);
        uint64_t newRepresentation1 = (intRepresentation1 & signAndExponentMask1) | newMantissa1;
        uint64_t newRepresentation2 = (intRepresentation2 & signAndExponentMask2) | newMantissa2;
        uint64_t newRepresentation3 = (intRepresentation3 & signAndExponentMask3) | newMantissa3;
        double newX1 = *reinterpret_cast<double*>(&newRepresentation1);
        double newX2 = *reinterpret_cast<double*>(&newRepresentation2);
        double newX3 = *reinterpret_cast<double*>(&newRepresentation3);
        variants.push_back({newX1, newX2, newX3});
    }
    return variants;
}

void processIntervalT(double a, double b, double c, double d, double e, double f, int samples, vector<SubIntervalResultT> &results) {
    double totalUlp = 0.0;
    double maxUlp = 0.0;
    tuple<double, double, double> maxUlpPoint;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis1(a, b);
    std::uniform_real_distribution<double> dis2(c, d);
    std::uniform_real_distribution<double> dis3(e, f);

    std::vector<std::tuple<double, double, double, double>> errors;

    for (int i = 0; i < INITIAL_SAMPLES; ++i) {
        double x1 = dis1(gen);
        double x2 = dis2(gen);
        double x3 = dis3(gen);
        double origin = getDoubleOfOrigin3(x1, x2, x3);
        double ulp = getULP3(x1, x2, x3, origin);
        errors.push_back({x1, x2, x3, ulp});
    }

    std::sort(errors.begin(), errors.end(), [](const std::tuple<double, double, double, double> &lhs, const std::tuple<double, double, double, double> &rhs) {
        return std::get<3>(lhs) > std::get<3>(rhs);
    });

    for (int i = 0; i < 10 && i < errors.size(); ++i) {
        auto variants = generateMantissaVariants(std::get<0>(errors[i]), std::get<1>(errors[i]), std::get<2>(errors[i]), samples);
        for (auto &variant : variants) {
            double origin = getDoubleOfOrigin3(std::get<0>(variant), std::get<1>(variant), std::get<2>(variant));
            double ulp = getULP3(std::get<0>(variant), std::get<1>(variant), std::get<2>(variant), origin);
            totalUlp += ulp;
            if (ulp > maxUlp) {
                maxUlp = ulp;
                maxUlpPoint = variant;
            }
        }
    }

    double avgUlp = totalUlp / (10 * samples);

    {
        std::scoped_lock lock(mtx3);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, make_tuple(make_pair(a, b), make_pair(c, d), make_pair(e, f))});
    }
}

void detectAndRefineT(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r, int samples, int SIZE, double threshold, bool isFinalIteration = false) {
    vector<SubIntervalResultT> results;
    vector<thread> threads;

    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            for (int k = 0; k < SIZE; ++k) {
                double sub_a = x1_l + (x1_r - x1_l) * i / SIZE;
                double sub_b = x1_l + (x1_r - x1_l) * (i + 1) / SIZE;
                double sub_c = x2_l + (x2_r - x2_l) * j / SIZE;
                double sub_d = x2_l + (x2_r - x2_l) * (j + 1) / SIZE;
                double sub_e = x3_l + (x3_r - x3_l) * k / SIZE;
                double sub_f = x3_l + (x3_r - x3_l) * (k + 1) / SIZE;
                threads.emplace_back(processIntervalT, sub_a, sub_b, sub_c, sub_d, sub_e, sub_f, samples, std::ref(results));
            }
        }
    }

    for (auto &t : threads) {
        t.join();
    }

    auto maxIt = max_element(results.begin(), results.end(),
                             [](const SubIntervalResultT &lhs, const SubIntervalResultT &rhs) {
                                 return lhs.avgUlp < rhs.avgUlp;
                             });

//    cout << "Maximum Average ULP Error: " << maxIt->avgUlp
//         << " in X1 [" << std::get<0>(std::get<0>(maxIt->interval)) << ", " << std::get<1>(std::get<0>(maxIt->interval)) << "] x2 ["
//         << std::get<0>(std::get<1>(maxIt->interval)) << ", " << std::get<1>(std::get<1>(maxIt->interval)) << "] x3 ["
//         << std::get<0>(std::get<2>(maxIt->interval)) << ", " << std::get<1>(std::get<2>(maxIt->interval)) << "]" << endl;

    if (isFinalIteration) {
        auto maxUlpIt = max_element(results.begin(), results.end(),
                                    [](const SubIntervalResultT& lhs, const SubIntervalResultT& rhs) {
                                        return lhs.maxUlp < rhs.maxUlp;
                                    });
        printf("Final Iteration - Maximum ULP Error: %.2lf, at X1: %.16lf, at X2: %.16lf, at X3: %.16lf, and Bit Error: %.2lf\n",
               maxUlpIt->maxUlp, std::get<0>(maxUlpIt->maxUlpPoint), std::get<1>(maxUlpIt->maxUlpPoint), std::get<2>(maxUlpIt->maxUlpPoint),
               log2(maxUlpIt->maxUlp + 1));
    }

    double x1_len = fabs(std::get<1>(std::get<0>(maxIt->interval)) - std::get<0>(std::get<0>(maxIt->interval)));
    double x2_len = fabs(std::get<1>(std::get<1>(maxIt->interval)) - std::get<0>(std::get<1>(maxIt->interval)));
    double x3_len = fabs(std::get<1>(std::get<2>(maxIt->interval)) - std::get<0>(std::get<2>(maxIt->interval)));

    if (x1_len > threshold || x2_len > threshold || x3_len > threshold) {
        // Recursive call for subinterval with greatest error
        detectAndRefineT(std::get<0>(std::get<0>(maxIt->interval)), std::get<1>(std::get<0>(maxIt->interval)), std::get<0>(std::get<1>(maxIt->interval)), std::get<1>(std::get<1>(maxIt->interval)),
                         std::get<0>(std::get<2>(maxIt->interval)), std::get<1>(std::get<2>(maxIt->interval)), samples, SIZE, threshold, false);
    } else if (!isFinalIteration) {
        // If not final iteration and below threshold, start final iteration
        detectAndRefineT(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r, samples, SIZE, threshold, true);
    }
}
void detect3(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r) {
    int SIZE = 0;
    int range1 = (int)(x1_r - x1_l);
    int range2 = (int)(x2_r - x2_l);
    int range3 = (int)(x3_r - x3_l);
    int range = std::min(range1, std::min(range2, range3));
    if (range < 10000) SIZE = 10;
    else SIZE = ceil(sqrt(range));
    detectAndRefineT(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r, SAMPLES, SIZE, THRESHOLD);
}
