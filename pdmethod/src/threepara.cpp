//
// Created by 99065 on 2024/3/26.
//
#include "function.h"
#include "getresult.h"
#define SIZE 4
#define SAMPLES 16
#define THRESHOLD 0.0000001
std::mutex mtx3;

void processIntervalT(double a, double b, double c, double d, double e, double f, int samples, vector<SubIntervalResultT> &results) {
    double totalUlp = 0.0;
    double maxUlp = 0.0;
    tuple<double, double, double> maxUlpPoint;
    double setpx1 = (b - a) / samples;
    double setpx2 = (d - c) / samples;
    double setpx3 = (f - e) / samples;

    for (int i = 0; i < samples; ++i) {
        for (int j = 0; j < samples; ++j) {
            for (int k = 0; k < samples; ++k) {
                double x1 = a + i * setpx1;
                double x2 = c + j * setpx2;
                double x3 = e + k * setpx3;
                double origin = getDoubleOfOrigin3(x1, x2, x3);
                double ulp = getULP3(x1, x2, x3, origin);
//		double ulp = getRelativeError3(x1, x2, x3, origin);
                totalUlp += ulp;
                if (ulp > maxUlp) {
                    maxUlp = ulp;
                    maxUlpPoint = std::make_tuple(x1, x2, x3);
                }
            }
        }
    }
    double avgUlp = totalUlp / (samples * samples * samples);

    {
        std::scoped_lock lock(mtx3);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, make_tuple(make_pair(a, b), make_pair(c, d), make_pair(e, f))});
    }
}

void detectAndRefineT(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r, int samples, double threshold, bool isFinalIteration = false) {
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
                         std::get<0>(std::get<2>(maxIt->interval)), std::get<1>(std::get<2>(maxIt->interval)), samples, threshold, false);
    } else if (!isFinalIteration) {
        // If not final iteration and below threshold, start final iteration
        detectAndRefineT(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r, samples, threshold, true);
    }
}
void detect3(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r) {
    detectAndRefineT(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r, SAMPLES, THRESHOLD);
}
