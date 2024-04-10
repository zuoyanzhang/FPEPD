#include "function.h"
#include "getresult.h"
#define SAMPLES 64
#define THRESHOLD 0.00000000001
#define SIZE 8
std::mutex mtx;

void processSubInterval(double a, double b, double c, double d, int samples, vector<SubIntervalResult> &results) {
    double totalUlp = 0;
    double maxUlp = 0;
    pair<double, double> maxUlpPoint;
    double setpx1 = (b - a) / samples;
    double setpx2 = (d - c) / samples;
    for (int i = 0; i < samples; ++i) {
        for (int j = 0; j < samples; ++j) {
            double x1 = a + i * setpx1;
            double x2 = c + j * setpx2;
            double origin = getDoubleOfOrigin2(x1, x2);
            double ulp = getULP2(x1, x2, origin);
//	    double ulp = getRelativeError2(x1, x2, origin);
            totalUlp += ulp;
            if (ulp > maxUlp) {
                maxUlp = ulp;
                maxUlpPoint = {x1, x2};
            }
        }
    }
    double avgUlp = totalUlp / (samples * samples);
    // 使用互斥锁保护共享数据
    {
        std::scoped_lock lock(mtx);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {{a, b}, {c, d}}});
    }
}

void detectAndRefine(double x1_l, double x1_r, double x2_l, double x2_r, int samples, double threshold, bool isFinalIteration = false) {
    vector<SubIntervalResult> results;
    vector<thread> threads;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            double sub_a = x1_l + (x1_r - x1_l) * i / SIZE;
            double sub_b = x1_l + (x1_r - x1_l) * (i + 1) / SIZE;
            double sub_c = x2_l + (x2_r - x2_l) * j / SIZE;
            double sub_d = x2_l + (x2_r - x2_l) * (j + 1) / SIZE;
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
        detectAndRefine(maxIt->interval.first.first, maxIt->interval.first.second, maxIt->interval.second.first, maxIt->interval.second.second, samples, threshold, false);
    } else if (!isFinalIteration) { // 如果当前不是最终迭代但已不需要进一步细化，则当前即为最终迭代
        detectAndRefine(x1_l, x1_r, x2_l, x2_r, samples, threshold, true);
    }
}

void detect2(double x1_l, double x1_r, double x2_l, double x2_r) {
    detectAndRefine(x1_l, x1_r, x2_l, x2_r, SAMPLES, THRESHOLD);
}
