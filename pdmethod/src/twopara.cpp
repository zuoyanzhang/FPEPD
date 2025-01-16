#include "function.h"
#include "getresult.h"

// ThreadPool implementation
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

// Error level definitions
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
std::mutex mtx2;
std::atomic<uint64_t> total_points_tested2{0};

double performInitialDetection2(double x1_l, double x1_r, double x2_l, double x2_r) {
    const double range = std::max(x1_r - x1_l, x2_r - x2_l);
    const int QUICK_SAMPLES = (range <= 1e3) ? 1024 :
                            (range <= 1e6) ? std::max(1024, std::min(static_cast<int>(range * 0.1), 50000)) :
                            std::max(5000, std::min(static_cast<int>(sqrt(range) * 10), 100000));
    
    double max_error = 0.0;
    std::mt19937 gen(FIXED_SEED);
    std::uniform_real_distribution<double> dis1(x1_l, x1_r);
    std::uniform_real_distribution<double> dis2(x2_l, x2_r);

    for (int i = 0; i < QUICK_SAMPLES; ++i) {
        const double x1 = dis1(gen);
        const double x2 = dis2(gen);
        const double origin = getDoubleOfOrigin2(x1, x2);
        max_error = std::max(max_error, getRelativeError2(x1, x2, origin));
    }
    total_points_tested2 += QUICK_SAMPLES;
    return max_error;
}

inline ErrorLevel determineErrorLevel2(double error) {
    if (error > 1e-1) return ErrorLevel::CRITICAL;
    if (error > 1e-5) return ErrorLevel::LARGE;
    if (error > 1e-10) return ErrorLevel::MEDIUM;
    if (error > 1e-15) return ErrorLevel::SMALL;
    return ErrorLevel::MINIMAL;
}

// Point generation functions with optimizations
template<ErrorLevel E>
std::vector<std::pair<double, double>> generateGradientPoints2(
    double x1, double x2, double interval_size1, double interval_size2, int steps) {
    std::vector<std::pair<double, double>> points;
    points.reserve(steps);
    
    const double min_step = std::pow(2.0, -52);
    const double max_step1 = interval_size1 * 0.1;
    const double max_step2 = interval_size2 * 0.1;
    double step_size1 = std::min(max_step1, std::max(min_step, interval_size1 * 0.01));
    double step_size2 = std::min(max_step2, std::max(min_step, interval_size2 * 0.01));
    
    double current1 = x1;
    double current2 = x2;
    
    for (int i = 0; i < steps; ++i) {
        const double h1 = std::max(std::abs(current1) * 1e-6, min_step);
        const double h2 = std::max(std::abs(current2) * 1e-6, min_step);
        
        const double f1 = getRelativeError2(current1 - h1, current2, 
                                          getDoubleOfOrigin2(current1 - h1, current2));
        const double f2 = getRelativeError2(current1 + h1, current2, 
                                          getDoubleOfOrigin2(current1 + h1, current2));
        const double grad1 = (f2 - f1) / (2 * h1);
        
        const double f3 = getRelativeError2(current1, current2 - h2, 
                                          getDoubleOfOrigin2(current1, current2 - h2));
        const double f4 = getRelativeError2(current1, current2 + h2, 
                                          getDoubleOfOrigin2(current1, current2 + h2));
        const double grad2 = (f4 - f3) / (2 * h2);
        
        if (i > 0) {
            const double prev_error = getRelativeError2(points.back().first, points.back().second,
                                                      getDoubleOfOrigin2(points.back().first, points.back().second));
            const double curr_error = getRelativeError2(current1, current2,
                                                      getDoubleOfOrigin2(current1, current2));
            
            if (curr_error > prev_error) {
                step_size1 = std::min(step_size1 * 1.2, max_step1);
                step_size2 = std::min(step_size2 * 1.2, max_step2);
            } else {
                step_size1 = std::max(step_size1 * 0.5, min_step);
                step_size2 = std::max(step_size2 * 0.5, min_step);
            }
        }
        
        current1 = std::clamp(current1 + step_size1 * grad1, 
                            x1 - interval_size1/2, x1 + interval_size1/2);
        current2 = std::clamp(current2 + step_size2 * grad2,
                            x2 - interval_size2/2, x2 + interval_size2/2);
        points.emplace_back(current1, current2);
    }
    return points;
}

template<ErrorLevel E>
std::vector<std::pair<double, double>> generateSpecialPoints2(
    double x1, double x2, int exp1, int exp2, int count) {
    std::vector<std::pair<double, double>> points;
    points.reserve(count);
    
    const double power_of_two1 = std::pow(2.0, exp1);
    const double power_of_two2 = std::pow(2.0, exp2);
    const double eps = std::pow(2.0, -52);
    
    points.emplace_back(power_of_two1 * (1.0 + eps), x2);
    points.emplace_back(power_of_two1 * (1.0 - eps), x2);
    points.emplace_back(x1, power_of_two2 * (1.0 + eps));
    points.emplace_back(x1, power_of_two2 * (1.0 - eps));
    
    if (std::abs(x1) < 1.0) {
        points.emplace_back(std::nextafter(x1, 0.0), x2);
        points.emplace_back(std::nextafter(x1, 1.0), x2);
    } else {
        points.emplace_back(std::nextafter(x1, x1/2.0), x2);
        points.emplace_back(std::nextafter(x1, x1*2.0), x2);
    }
    
    if (std::abs(x2) < 1.0) {
        points.emplace_back(x1, std::nextafter(x2, 0.0));
        points.emplace_back(x1, std::nextafter(x2, 1.0));
    } else {
        points.emplace_back(x1, std::nextafter(x2, x2/2.0));
        points.emplace_back(x1, std::nextafter(x2, x2*2.0));
    }
    
    std::mt19937 gen(static_cast<unsigned int>(exp1 + exp2));
    std::uniform_real_distribution<double> dis1(0.95 * x1, 1.05 * x1);
    std::uniform_real_distribution<double> dis2(0.95 * x2, 1.05 * x2);
    
    while (points.size() < count) {
        points.emplace_back(dis1(gen), dis2(gen));
    }
    
    return points;
}

template<ErrorLevel E>
std::vector<std::pair<double, double>> generateHeuristicPoints2(
    double x1, double x2, uint64_t mantissa1, uint64_t mantissa2, int count) {
    std::vector<std::pair<double, double>> points;
    points.reserve(count);
    
    const uint64_t pattern1 = mantissa1 & 0xF000000000000;
    const uint64_t pattern2 = mantissa2 & 0xF000000000000;
    std::mt19937 gen(static_cast<unsigned int>(mantissa1 + mantissa2));
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);
    
    const uint64_t mask1 = (*reinterpret_cast<const uint64_t*>(&x1) & 0xFFF0000000000000);
    const uint64_t mask2 = (*reinterpret_cast<const uint64_t*>(&x2) & 0xFFF0000000000000);
    
    for (int i = 0; i < count; ++i) {
        const uint64_t new_mantissa1 = (pattern1 | (dis(gen) & 0x0FFFFFFFFFFFF));
        const uint64_t new_mantissa2 = (pattern2 | (dis(gen) & 0x0FFFFFFFFFFFF));
        
        const uint64_t new_bits1 = mask1 | new_mantissa1;
        const uint64_t new_bits2 = mask2 | new_mantissa2;
        
        points.emplace_back(*reinterpret_cast<const double*>(&new_bits1),
                           *reinterpret_cast<const double*>(&new_bits2));
    }
    return points;
}

template<ErrorLevel E>
std::vector<std::pair<double, double>> enhancedSearch2(
    double x1, double x2, double interval_size1, double interval_size2) {
    std::vector<std::pair<double, double>> candidates;
    candidates.reserve(SamplingStrategy<E>::VARIANT_SAMPLES);
    
    const uint64_t bits1 = *reinterpret_cast<const uint64_t*>(&x1);
    const uint64_t bits2 = *reinterpret_cast<const uint64_t*>(&x2);
    const int exp1 = ((bits1 >> 52) & 0x7FF) - 1023;
    const int exp2 = ((bits2 >> 52) & 0x7FF) - 1023;
    
    auto gradient_points = generateGradientPoints2<E>(x1, x2, interval_size1, interval_size2,
                                                    SamplingStrategy<E>::VARIANT_SAMPLES / 3);
    candidates.insert(candidates.end(),
                     std::make_move_iterator(gradient_points.begin()),
                     std::make_move_iterator(gradient_points.end()));
    
    auto special_points = generateSpecialPoints2<E>(x1, x2, exp1, exp2,
                                                  SamplingStrategy<E>::VARIANT_SAMPLES / 6);
    candidates.insert(candidates.end(),
                     std::make_move_iterator(special_points.begin()),
                     std::make_move_iterator(special_points.end()));
    
    const uint64_t mantissa1 = bits1 & 0x000FFFFFFFFFFFFF;
    const uint64_t mantissa2 = bits2 & 0x000FFFFFFFFFFFFF;
    auto heuristic_points = generateHeuristicPoints2<E>(x1, x2, mantissa1, mantissa2,
                                                       SamplingStrategy<E>::VARIANT_SAMPLES / 2);
    candidates.insert(candidates.end(),
                     std::make_move_iterator(heuristic_points.begin()),
                     std::make_move_iterator(heuristic_points.end()));
    
    if (candidates.size() > SamplingStrategy<E>::VARIANT_SAMPLES) {
        std::nth_element(candidates.begin(),
                        candidates.begin() + SamplingStrategy<E>::VARIANT_SAMPLES,
                        candidates.end());
        candidates.resize(SamplingStrategy<E>::VARIANT_SAMPLES);
    }
    
    return candidates;
}

template<ErrorLevel E>
void processSubIntervalTemplate(double a, double b, double c, double d,
                              vector<SubIntervalResult>& results) {
    std::vector<std::tuple<double, double, double>> errors;
    errors.reserve(SamplingStrategy<E>::INITIAL_SAMPLES);
    
    const unsigned int seed = static_cast<unsigned int>(a * 1000 + b * 100 + c * 10 + d);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis1(a, b);
    std::uniform_real_distribution<double> dis2(c, d);

    double quick_max_error = 0.0;
    std::pair<double, double> quick_max_point;
    total_points_tested2 += SamplingStrategy<E>::INITIAL_SAMPLES;
for (int i = 0; i < SamplingStrategy<E>::INITIAL_SAMPLES; ++i) {
        const double x1 = dis1(gen);
        const double x2 = dis2(gen);
        const double origin = getDoubleOfOrigin2(x1, x2);
        const double error = getRelativeError2(x1, x2, origin);
        if (error > quick_max_error) {
            quick_max_error = error;
            quick_max_point = {x1, x2};
        }
        errors.emplace_back(x1, x2, error);
    }

    std::partial_sort(errors.begin(), 
                     errors.begin() + SamplingStrategy<E>::TOP_K,
                     errors.end(),
                     [](const auto& lhs, const auto& rhs) {
                         return std::get<2>(lhs) > std::get<2>(rhs);
                     });

    double totalUlp = 0.0;
    double maxUlp = quick_max_error;
    std::pair<double, double> maxUlpPoint = quick_max_point;

    const double interval_size1 = b - a;
    const double interval_size2 = d - c;

    for (int i = 0; i < SamplingStrategy<E>::TOP_K && i < errors.size(); ++i) {
        auto variants = enhancedSearch2<E>(std::get<0>(errors[i]), std::get<1>(errors[i]),
                                         interval_size1, interval_size2);
        total_points_tested2 += variants.size();
        
        for (const auto& variant : variants) {
            const double origin = getDoubleOfOrigin2(variant.first, variant.second);
            const double ulp = getRelativeError2(variant.first, variant.second, origin);
            totalUlp += ulp;
            if (ulp > maxUlp) {
                maxUlp = ulp;
                maxUlpPoint = variant;
            }
        }
    }
    
    const double avgUlp = totalUlp / (SamplingStrategy<E>::TOP_K * 
                                     static_cast<double>(SamplingStrategy<E>::VARIANT_SAMPLES));
    {
        std::scoped_lock lock(mtx2);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {{a, b}, {c, d}}});
    }
}

template<ErrorLevel E>
void detectAndRefineTemplate2(double x1_l, double x1_r, double x2_l, double x2_r, 
                            int SIZE, double threshold, bool isFinalIteration = false) {
    vector<SubIntervalResult> results;
    results.reserve(SIZE * SIZE);
    
    static ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> completed_tasks{0};
    std::mutex completion_mutex;
    std::condition_variable completion_cv;
    
    const int total_tasks = SIZE * SIZE;
    
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            const double sub_a = x1_l + (x1_r - x1_l) * i / static_cast<double>(SIZE);
            const double sub_b = x1_l + (x1_r - x1_l) * (i + 1) / static_cast<double>(SIZE);
            const double sub_c = x2_l + (x2_r - x2_l) * j / static_cast<double>(SIZE);
            const double sub_d = x2_l + (x2_r - x2_l) * (j + 1) / static_cast<double>(SIZE);

            pool.enqueue([&, sub_a, sub_b, sub_c, sub_d] {
                processSubIntervalTemplate<E>(sub_a, sub_b, sub_c, sub_d, ref(results));
                
                int completed = ++completed_tasks;
                if (completed == total_tasks) {
                    completion_cv.notify_one();
                }
            });
        }
    }
    
    {
        std::unique_lock<std::mutex> lock(completion_mutex);
        completion_cv.wait(lock, [&] { return completed_tasks == total_tasks; });
    }
    
    auto maxIt = max_element(results.begin(), results.end(), 
        [](const SubIntervalResult& lhs, const SubIntervalResult& rhs) {
            return lhs.avgUlp < rhs.avgUlp;
        });

    if (isFinalIteration) {
        printf("Final Iteration - Maximum Relative Error: %e, at X1: %.16lf, at X2: %.16lf\n", 
               maxIt->maxUlp, maxIt->maxUlpPoint.first, maxIt->maxUlpPoint.second);
        printf("Total points tested: %llu\n", total_points_tested2.load());
        return;
    }

    const double x1_len = fabs(maxIt->interval.first.second - maxIt->interval.first.first);
    const double x2_len = fabs(maxIt->interval.second.second - maxIt->interval.second.first);
    
    int dynamic_size = SIZE;
    if (x1_len < threshold * 10 || x2_len < threshold * 10) {
        dynamic_size = std::max(2, SIZE / 2);
    }

    if (x1_len > threshold || x2_len > threshold) {
        detectAndRefineTemplate2<E>(maxIt->interval.first.first, maxIt->interval.first.second,
                                  maxIt->interval.second.first, maxIt->interval.second.second,
                                  dynamic_size, threshold, false);
    } else {
        detectAndRefineTemplate2<E>(x1_l, x1_r, x2_l, x2_r, dynamic_size, threshold, true);
    }
}

void detect2(double x1_l, double x1_r, double x2_l, double x2_r) {
    const double initial_max_error = performInitialDetection2(x1_l, x1_r, x2_l, x2_r);
    const ErrorLevel level = determineErrorLevel2(initial_max_error);
    
    const int range1 = static_cast<int>(x1_r - x1_l);
    const int range2 = static_cast<int>(x2_r - x2_l);
    const int range = std::min(range1, range2);
    // const int SIZE = (range <= 10000) ? 10 : (10 + static_cast<int>(ceil(log2(range))));
    const int SIZE = (range <= 10000) ? 10 : (10 + static_cast<int>(ceil(log2(range))));
    
    total_points_tested2 = 0;
    constexpr double THRESHOLD = 1e-9;
    
    switch(level) {
        case ErrorLevel::MINIMAL:
            detectAndRefineTemplate2<ErrorLevel::MINIMAL>(x1_l, x1_r, x2_l, x2_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::SMALL:
            detectAndRefineTemplate2<ErrorLevel::SMALL>(x1_l, x1_r, x2_l, x2_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::MEDIUM:
            detectAndRefineTemplate2<ErrorLevel::MEDIUM>(x1_l, x1_r, x2_l, x2_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::LARGE:
            detectAndRefineTemplate2<ErrorLevel::LARGE>(x1_l, x1_r, x2_l, x2_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::CRITICAL:
            detectAndRefineTemplate2<ErrorLevel::CRITICAL>(x1_l, x1_r, x2_l, x2_r, SIZE, THRESHOLD);
            break;
    }
}
