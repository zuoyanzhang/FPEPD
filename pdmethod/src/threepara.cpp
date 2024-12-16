#include "function.h"
#include "getresult.h"

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if(stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
    }
    
    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers)
            worker.join();
    }
};

enum class ErrorLevel {
    MINIMAL,   // < 1e-15
    SMALL,     // 1e-15 ~ 1e-10
    MEDIUM,    // 1e-10 ~ 1e-5
    LARGE,     // 1e-5 ~ 1e-1
    CRITICAL   // > 1e-1
};

template<ErrorLevel E>
struct SamplingStrategy {
    static constexpr int INITIAL_SAMPLES = 100; 
    static constexpr int VARIANT_SAMPLES = 64;  
    static constexpr int TOP_K = 5;            
};

template<>
struct SamplingStrategy<ErrorLevel::MINIMAL> {
    static constexpr int INITIAL_SAMPLES = 100;
    static constexpr int VARIANT_SAMPLES = 64;
    static constexpr int TOP_K = 3;
};

template<>
struct SamplingStrategy<ErrorLevel::SMALL> {
    static constexpr int INITIAL_SAMPLES = 250;
    static constexpr int VARIANT_SAMPLES = 128;
    static constexpr int TOP_K = 5;
};

template<>
struct SamplingStrategy<ErrorLevel::MEDIUM> {
    static constexpr int INITIAL_SAMPLES = 500;
    static constexpr int VARIANT_SAMPLES = 256;
    static constexpr int TOP_K = 7;
};

template<>
struct SamplingStrategy<ErrorLevel::LARGE> {
    static constexpr int INITIAL_SAMPLES = 750;
    static constexpr int VARIANT_SAMPLES = 384;
    static constexpr int TOP_K = 8;
};

template<>
struct SamplingStrategy<ErrorLevel::CRITICAL> {
    static constexpr int INITIAL_SAMPLES = 1000;
    static constexpr int VARIANT_SAMPLES = 512;
    static constexpr int TOP_K = 10;
};

const unsigned int FIXED_SEED = 2;
std::mutex mtx3;
std::atomic<uint64_t> total_points_tested3{0};

template<ErrorLevel E>
std::vector<std::tuple<double, double, double>> generateMantissaVariants3(
    double x1, double x2, double x3, int count) {
    std::vector<std::tuple<double, double, double>> variants;
    variants.reserve(count);
    
    const uint64_t bits1 = *reinterpret_cast<const uint64_t*>(&x1);
    const uint64_t bits2 = *reinterpret_cast<const uint64_t*>(&x2);
    const uint64_t bits3 = *reinterpret_cast<const uint64_t*>(&x3);
    
    const uint64_t pattern1 = bits1 & 0xF000000000000;
    const uint64_t pattern2 = bits2 & 0xF000000000000;
    const uint64_t pattern3 = bits3 & 0xF000000000000;
    
    const uint64_t mask1 = bits1 & 0xFFF0000000000000;
    const uint64_t mask2 = bits2 & 0xFFF0000000000000;
    const uint64_t mask3 = bits3 & 0xFFF0000000000000;
    
    std::mt19937 gen(static_cast<unsigned int>(bits1 + bits2 + bits3));
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);
    
    for (int i = 0; i < count; ++i) {
        const uint64_t new_mantissa1 = pattern1 | (dis(gen) & 0x0FFFFFFFFFFFF);
        const uint64_t new_mantissa2 = pattern2 | (dis(gen) & 0x0FFFFFFFFFFFF);
        const uint64_t new_mantissa3 = pattern3 | (dis(gen) & 0x0FFFFFFFFFFFF);
        
        const uint64_t new_bits1 = mask1 | new_mantissa1;
        const uint64_t new_bits2 = mask2 | new_mantissa2;
        const uint64_t new_bits3 = mask3 | new_mantissa3;
        
        variants.emplace_back(*reinterpret_cast<const double*>(&new_bits1),
                            *reinterpret_cast<const double*>(&new_bits2),
                            *reinterpret_cast<const double*>(&new_bits3));
    }
    return variants;
}

double performInitialDetection3(double x1_l, double x1_r, double x2_l, double x2_r, 
                              double x3_l, double x3_r) {
    const double range = std::max({x1_r - x1_l, x2_r - x2_l, x3_r - x3_l});
    const int QUICK_SAMPLES = (range <= 1e3) ? 1024 :
                            (range <= 1e6) ? std::max(1024, std::min(static_cast<int>(range * 0.1), 50000)) :
                            std::max(5000, std::min(static_cast<int>(sqrt(range) * 10), 100000));
    
    double max_error = 0.0;
    std::mt19937 gen(FIXED_SEED);
    std::uniform_real_distribution<double> dis1(x1_l, x1_r);
    std::uniform_real_distribution<double> dis2(x2_l, x2_r);
    std::uniform_real_distribution<double> dis3(x3_l, x3_r);
    
    for (int i = 0; i < QUICK_SAMPLES; ++i) {
        const double x1 = dis1(gen);
        const double x2 = dis2(gen);
        const double x3 = dis3(gen);
        const double origin = getDoubleOfOrigin3(x1, x2, x3);
        max_error = std::max(max_error, getRelativeError3(x1, x2, x3, origin));
    }
    total_points_tested3 += QUICK_SAMPLES;
    return max_error;
}

inline ErrorLevel determineErrorLevel3(double error) {
    if (error > 1e-1) return ErrorLevel::CRITICAL;
    if (error > 1e-5) return ErrorLevel::LARGE;
    if (error > 1e-10) return ErrorLevel::MEDIUM;
    if (error > 1e-15) return ErrorLevel::SMALL;
    return ErrorLevel::MINIMAL;
}

template<ErrorLevel E>
void processIntervalTemplate3(double a, double b, double c, double d, double e, double f,
                            vector<SubIntervalResultT>& results) {
    std::vector<std::tuple<double, double, double, double>> errors;
    errors.reserve(SamplingStrategy<E>::INITIAL_SAMPLES);
    
    const unsigned int seed = static_cast<unsigned int>(a * 1000 + b * 100 + c * 10 + d + e + f);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis1(a, b);
    std::uniform_real_distribution<double> dis2(c, d);
    std::uniform_real_distribution<double> dis3(e, f);

    double quick_max_error = 0.0;
    std::tuple<double, double, double> quick_max_point;
    total_points_tested3 += SamplingStrategy<E>::INITIAL_SAMPLES;

    for (int i = 0; i < SamplingStrategy<E>::INITIAL_SAMPLES; ++i) {
        const double x1 = dis1(gen);
        const double x2 = dis2(gen);
        const double x3 = dis3(gen);
        const double origin = getDoubleOfOrigin3(x1, x2, x3);
        const double error = getRelativeError3(x1, x2, x3, origin);
        if (error > quick_max_error) {
            quick_max_error = error;
            quick_max_point = {x1, x2, x3};
        }
        errors.emplace_back(x1, x2, x3, error);
    }

    if (quick_max_error < 1e-17) {
        std::scoped_lock lock(mtx3);
        results.push_back({quick_max_error, quick_max_error, quick_max_point,
                          {{a, b}, {c, d}, {e, f}}});
        return;
    }

    std::partial_sort(errors.begin(), 
                     errors.begin() + SamplingStrategy<E>::TOP_K,
                     errors.end(),
                     [](const auto& lhs, const auto& rhs) {
                         return std::get<3>(lhs) > std::get<3>(rhs);
                     });

    double totalUlp = 0.0;
    double maxUlp = quick_max_error;
    std::tuple<double, double, double> maxUlpPoint = quick_max_point;

    for (int i = 0; i < SamplingStrategy<E>::TOP_K && i < errors.size(); ++i) {
        auto variants = generateMantissaVariants3<E>(std::get<0>(errors[i]),
                                                   std::get<1>(errors[i]),
                                                   std::get<2>(errors[i]),
                                                   SamplingStrategy<E>::VARIANT_SAMPLES);
        total_points_tested3 += variants.size();
        
        for (const auto& variant : variants) {
            const double origin = getDoubleOfOrigin3(std::get<0>(variant),
                                                   std::get<1>(variant),
                                                   std::get<2>(variant));
            const double error = getRelativeError3(std::get<0>(variant),
                                                 std::get<1>(variant),
                                                 std::get<2>(variant),
                                                 origin);
            totalUlp += error;
            if (error > maxUlp) {
                maxUlp = error;
                maxUlpPoint = variant;
            }
        }
    }
    
    const double avgUlp = totalUlp / (SamplingStrategy<E>::TOP_K * 
                                     static_cast<double>(SamplingStrategy<E>::VARIANT_SAMPLES));
    {
        std::scoped_lock lock(mtx3);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {{a, b}, {c, d}, {e, f}}});
    }
}

template<ErrorLevel E>
void detectAndRefineTemplate3(double x1_l, double x1_r,
                            double x2_l, double x2_r,
                            double x3_l, double x3_r,
                            int SIZE, double threshold,
                            bool isFinalIteration = false) {
    vector<SubIntervalResultT> results;
    results.reserve(SIZE * SIZE * SIZE);
    
    static ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> completed_tasks{0};
    std::mutex completion_mutex;
    std::condition_variable completion_cv;
    
    const int total_tasks = SIZE * SIZE * SIZE;
    
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            for (int k = 0; k < SIZE; ++k) {
                const double sub_a = x1_l + (x1_r - x1_l) * i / static_cast<double>(SIZE);
                const double sub_b = x1_l + (x1_r - x1_l) * (i + 1) / static_cast<double>(SIZE);
                const double sub_c = x2_l + (x2_r - x2_l) * j / static_cast<double>(SIZE);
                const double sub_d = x2_l + (x2_r - x2_l) * (j + 1) / static_cast<double>(SIZE);
                const double sub_e = x3_l + (x3_r - x3_l) * k / static_cast<double>(SIZE);
                const double sub_f = x3_l + (x3_r - x3_l) * (k + 1) / static_cast<double>(SIZE);

                pool.enqueue([&, sub_a, sub_b, sub_c, sub_d, sub_e, sub_f] {
                    processIntervalTemplate3<E>(sub_a, sub_b, sub_c, sub_d,
                                             sub_e, sub_f, ref(results));
                    int completed = ++completed_tasks;
                    if (completed == total_tasks) {
                        completion_cv.notify_one();
                    }
                });
            }
        }
    }
    
    {
        std::unique_lock<std::mutex> lock(completion_mutex);
        completion_cv.wait(lock, [&] { return completed_tasks == total_tasks; });
    }
    
    auto maxIt = max_element(results.begin(), results.end(), 
        [](const SubIntervalResultT& lhs, const SubIntervalResultT& rhs) {
            return lhs.avgUlp < rhs.avgUlp;
        });

    if (isFinalIteration) {
        printf("Final Iteration - Maximum Relative Error: %e, at X1: %.16lf, at X2: %.16lf, at X3: %.16lf\n",
               maxIt->maxUlp, 
               std::get<0>(maxIt->maxUlpPoint),
               std::get<1>(maxIt->maxUlpPoint),
               std::get<2>(maxIt->maxUlpPoint));
        printf("Total points tested: %llu\n", total_points_tested3.load());
        return;
    }

    const double x1_len = fabs(std::get<1>(std::get<0>(maxIt->interval)) - 
                              std::get<0>(std::get<0>(maxIt->interval)));
    const double x2_len = fabs(std::get<1>(std::get<1>(maxIt->interval)) - 
                              std::get<0>(std::get<1>(maxIt->interval)));
    const double x3_len = fabs(std::get<1>(std::get<2>(maxIt->interval)) - 
                              std::get<0>(std::get<2>(maxIt->interval)));
    
    int dynamic_size = SIZE;
    if (x1_len < threshold * 10 || x2_len < threshold * 10 || x3_len < threshold * 10) {
        dynamic_size = std::max(2, SIZE / 2);
    }

    if (x1_len > threshold || x2_len > threshold || x3_len > threshold) {
        detectAndRefineTemplate3<E>(
            std::get<0>(std::get<0>(maxIt->interval)),
            std::get<1>(std::get<0>(maxIt->interval)),
            std::get<0>(std::get<1>(maxIt->interval)),
            std::get<1>(std::get<1>(maxIt->interval)),
            std::get<0>(std::get<2>(maxIt->interval)),
            std::get<1>(std::get<2>(maxIt->interval)),
            dynamic_size, threshold, false);
    } else {
        detectAndRefineTemplate3<E>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                  dynamic_size, threshold, true);
    }
}

void detect3(double x1_l, double x1_r, double x2_l, double x2_r, double x3_l, double x3_r) {
    const double initial_max_error = performInitialDetection3(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r);
    const ErrorLevel level = determineErrorLevel3(initial_max_error);
    
    const int range1 = static_cast<int>(x1_r - x1_l);
    const int range2 = static_cast<int>(x2_r - x2_l);
    const int range3 = static_cast<int>(x3_r - x3_l);
    const int range = std::min({range1, range2, range3});
    const int SIZE = (range <= 10000) ? 5 : static_cast<int>(ceil(cbrt(range / 10)));
    
    total_points_tested3 = 0;
    constexpr double THRESHOLD = 1e-9;
    
    switch(level) {
        case ErrorLevel::MINIMAL:
            detectAndRefineTemplate3<ErrorLevel::MINIMAL>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                                        SIZE, THRESHOLD);
            break;
        case ErrorLevel::SMALL:
            detectAndRefineTemplate3<ErrorLevel::SMALL>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                                      SIZE, THRESHOLD);
            break;
        case ErrorLevel::MEDIUM:
            detectAndRefineTemplate3<ErrorLevel::MEDIUM>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                                       SIZE, THRESHOLD);
            break;
        case ErrorLevel::LARGE:
            detectAndRefineTemplate3<ErrorLevel::LARGE>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                                      SIZE, THRESHOLD);
            break;
        case ErrorLevel::CRITICAL:
            detectAndRefineTemplate3<ErrorLevel::CRITICAL>(x1_l, x1_r, x2_l, x2_r, x3_l, x3_r,
                                                         SIZE, THRESHOLD);
            break;
    }
}