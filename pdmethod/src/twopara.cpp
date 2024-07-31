#include "function.h"
#include "getresult.h"

#define SAMPLES 512
#define INITIAL_SAMPLES 1000
#define THRESHOLD 1.0e-11

std::mutex mtx;

std::vector<std::pair<double, double>> generateMantissaVariants(double x1, double x2, int count) {
    std::vector<std::pair<double, double>> variants;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);

    uint64_t intRepresentation1 = *reinterpret_cast<uint64_t*>(&x1);
    uint64_t intRepresentation2 = *reinterpret_cast<uint64_t*>(&x2);
    uint64_t signAndExponentMask1 = 0xFFF0000000000000ULL;
    uint64_t signAndExponentMask2 = 0xFFF0000000000000ULL;

    for (int i = 0; i < count; ++i) {
        uint64_t newMantissa1 = dis(gen);
        uint64_t newMantissa2 = dis(gen);
        uint64_t newRepresentation1 = (intRepresentation1 & signAndExponentMask1) | newMantissa1;
        uint64_t newRepresentation2 = (intRepresentation2 & signAndExponentMask2) | newMantissa2;
        double newX1 = *reinterpret_cast<double*>(&newRepresentation1);
        double newX2 = *reinterpret_cast<double*>(&newRepresentation2);
        variants.push_back({newX1, newX2});
    }
    return variants;
}

void processSubInterval(double a, double b, double c, double d, int samples, vector<SubIntervalResult> &results) {
    double totalUlp = 0;
    double maxUlp = 0;
    pair<double, double> maxUlpPoint;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis1(a, b);
    std::uniform_real_distribution<double> dis2(c, d);

    std::vector<std::tuple<double, double, double>> errors;

    for (int i = 0; i < INITIAL_SAMPLES; ++i) {
        double x1 = dis1(gen);
        double x2 = dis2(gen);
        double origin = getDoubleOfOrigin2(x1, x2);
        double ulp = getULP2(x1, x2, origin);
        errors.push_back({x1, x2, ulp});
    }

    std::sort(errors.begin(), errors.end(), [](const std::tuple<double, double, double> &lhs, const std::tuple<double, double, double> &rhs) {
        return std::get<2>(lhs) > std::get<2>(rhs);
    });

    for (int i = 0; i < 10 && i < errors.size(); ++i) {
        auto variants = generateMantissaVariants(std::get<0>(errors[i]), std::get<1>(errors[i]), samples);
        for (auto &variant : variants) {
            double origin = getDoubleOfOrigin2(variant.first, variant.second);
            double ulp = getULP2(variant.first, variant.second, origin);
            totalUlp += ulp;
            if (ulp > maxUlp) {
                maxUlp = ulp;
                maxUlpPoint = variant;
            }
        }
    }

    double avgUlp = totalUlp / (10 * samples);
    // 使用互斥锁保护共享数据
    {
        std::scoped_lock lock(mtx);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {{a, b}, {c, d}}});
    }
}

void detectAndRefine(double x1_l, double x1_r, double x2_l, double x2_r, int samples, int SIZE, double threshold, bool isFinalIteration = false) {
    vector<SubIntervalResult> results;
    vector<thread> threads;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            double sub_a = x1_l + (x1_r - x1_l) * i / SIZE;
            double sub_b = x1_l + (x1_r - x1_l) * (i + 1) / SIZE;
            double sub_c = x2_l + (x2_r - x2_l) * j / SIZE;
            double sub_d = x2_l + (x2_r - x2_l) * (j + 1) / SIZE;

            if (sub_a >= sub_b || sub_c >= sub_d) {
                std::cerr << "Invalid interval: [" << sub_a << ", " << sub_b << "] [" << sub_c << ", " << sub_d << "]" << std::endl;
                continue;
            }

            threads.push_back(thread(processSubInterval, sub_a, sub_b, sub_c, sub_d, samples, ref(results)));
        }
    }
    for (auto &t : threads) {
        t.join();
    }
    auto maxIt = std::max_element(results.begin(), results.end(), [](const SubIntervalResult &lhs, const SubIntervalResult &rhs) {
        return lhs.avgUlp < rhs.avgUlp;
    });

//    cout << "Maximum Average ULP Error: " << maxIt->avgUlp
//              << " in X1 [" << maxIt->interval.first.first << ", " << maxIt->interval.first.second << "] X2 ["
//              << maxIt->interval.second.first << ", " << maxIt->interval.second.second << "]" << endl;
    if (isFinalIteration) {
        auto maxUlpIt = max_element(results.begin(), results.end(),
                                    [](const SubIntervalResult& lhs, const SubIntervalResult& rhs) {
                                        return lhs.maxUlp < rhs.maxUlp;
                                    });
        printf("Final Iteration - Maximum ULP Error: %.2lf, at X1: %.16lf, at X2: %.16lf, and Bit Error: %.2lf\n", maxUlpIt->maxUlp, maxUlpIt->maxUlpPoint.first, maxUlpIt->maxUlpPoint.second, log2(maxUlpIt->maxUlp + 1));
    }
    // 检查是否需要继续细化
    double x1_len = fabs(maxIt->interval.first.second - maxIt->interval.first.first);
    double x2_len = fabs(maxIt->interval.second.second - maxIt->interval.second.first);
    if (x1_len > threshold || x2_len > threshold) {
        detectAndRefine(maxIt->interval.first.first, maxIt->interval.first.second, maxIt->interval.second.first, maxIt->interval.second.second, samples, SIZE, threshold, false);
    } else if (!isFinalIteration) { // 如果当前不是最终迭代但已不需要进一步细化，则当前即为最终迭代
        detectAndRefine(x1_l, x1_r, x2_l, x2_r, samples, SIZE, threshold, true);
    }
}

void detect2(double x1_l, double x1_r, double x2_l, double x2_r) {
    int SIZE = 0;
    int range1 = (int)(x1_r - x1_l);
    int range2 = (int)(x2_r - x2_l);
    int range = std::min(range1, range2);
    if (range < 10000) SIZE = 50;
    else SIZE = ceil(sqrt(range));
    detectAndRefine(x1_l, x1_r, x2_l, x2_r, SAMPLES, SIZE, THRESHOLD);
}
