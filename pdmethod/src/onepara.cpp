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
std::mutex mtx1;
std::atomic<uint64_t> total_points_tested1{0};

inline double performInitialDetection(double x_l, double x_r) {
    const double range = x_r - x_l;
    const int QUICK_SAMPLES = 
        (range <= 1e3) ? 1024 :
        (range <= 1e6) ? std::max(1024, std::min(static_cast<int>(range * 0.1), 50000)) :
        std::max(5000, std::min(static_cast<int>(sqrt(range) * 10), 100000));
    
    double max_error = 0.0;
    std::mt19937 gen(FIXED_SEED);
    std::uniform_real_distribution<double> dis(x_l, x_r);

    for (int i = 0; i < QUICK_SAMPLES; ++i) {
        const double x = dis(gen);
        const double origin = getDoubleOfOrigin(x);
        max_error = std::max(max_error, getRelativeError(x, origin));
    }
    total_points_tested1 += QUICK_SAMPLES;
    return max_error;
}

inline ErrorLevel determineErrorLevel(double error) {
    if (error > 1e-1) return ErrorLevel::CRITICAL;
    if (error > 1e-5) return ErrorLevel::LARGE;
    if (error > 1e-10) return ErrorLevel::MEDIUM;
    if (error > 1e-15) return ErrorLevel::SMALL;
    return ErrorLevel::MINIMAL;
}

template<ErrorLevel E>
std::vector<double> generateGradientPoints(double x, double interval_size, int steps) {
    std::vector<double> points;
    points.reserve(steps);
    
    const double min_step = std::max(interval_size * 1e-10, std::pow(2.0, -52));
    const double max_step = interval_size * 0.1;
    double step_size = std::min(max_step, std::max(min_step, interval_size * 0.01));
    double current = x;
    
    for (int i = 0; i < steps; ++i) {
        const double h = std::max(std::abs(current) * 1e-6, min_step);
        const double f1 = getRelativeError(current - h, getDoubleOfOrigin(current - h));
        const double f2 = getRelativeError(current + h, getDoubleOfOrigin(current + h));
        const double grad = (f2 - f1) / (2 * h);
        
        if (i > 0) {
            const double prev_error = getRelativeError(points.back(), getDoubleOfOrigin(points.back()));
            const double curr_error = getRelativeError(current, getDoubleOfOrigin(current));
            
            if (curr_error > prev_error) {
                step_size = std::min(step_size * 1.2, max_step);
            } else {
                step_size = std::max(step_size * 0.5, min_step);
            }
        }
        
        current = std::clamp(current + step_size * grad,
                           x - interval_size/2,
                           x + interval_size/2);
        points.push_back(current);
    }
    return points;
}

template<ErrorLevel E>
std::vector<double> generateSpecialPoints(double x, int exp, int count) {
    std::vector<double> points;
    points.reserve(count);
    
    const double power_of_two = std::pow(2.0, exp);
    const double eps = std::pow(2.0, -52);
    
    points.push_back(power_of_two * (1.0 + eps));
    points.push_back(power_of_two * (1.0 - eps));
    
    if (std::abs(x) < 1.0) {
        points.push_back(std::nextafter(x, 0.0));
        points.push_back(std::nextafter(x, 1.0));
    } else {
        points.push_back(std::nextafter(x, x/2.0));
        points.push_back(std::nextafter(x, x*2.0));
    }
    
    std::mt19937 gen(static_cast<unsigned int>(exp));
    std::uniform_real_distribution<double> dis(0.95 * x, 1.05 * x);
    while (points.size() < count) {
        points.push_back(dis(gen));
    }
    
    return points;
}

template<ErrorLevel E>
std::vector<double> generateHeuristicPoints(double x, uint64_t original_mantissa, int count) {
    std::vector<double> points;
    points.reserve(count);
    
    const uint64_t pattern = original_mantissa & 0xF000000000000;
    const uint64_t mask = (*reinterpret_cast<const uint64_t*>(&x) & 0xFFF0000000000000);
    
    std::mt19937 gen(static_cast<unsigned int>(original_mantissa));
    std::uniform_int_distribution<uint64_t> dis(0, (1ULL << 52) - 1);
    
    for (int i = 0; i < count; ++i) {
        const uint64_t new_mantissa = pattern | (dis(gen) & 0x0FFFFFFFFFFFF);
        const uint64_t new_bits = mask | new_mantissa;
        points.push_back(*reinterpret_cast<const double*>(&new_bits));
    }
    
    return points;
}

template<ErrorLevel E>
std::vector<double> enhancedSearch(double x, double interval_size) {
    std::vector<double> candidates;
    candidates.reserve(SamplingStrategy<E>::VARIANT_SAMPLES);
    
    const uint64_t bits = *reinterpret_cast<const uint64_t*>(&x);
    const int exp = ((bits >> 52) & 0x7FF) - 1023;
    
    auto gradient_points = generateGradientPoints<E>(x, interval_size,
                                                   SamplingStrategy<E>::VARIANT_SAMPLES / 3);
    candidates.insert(candidates.end(),
                     std::make_move_iterator(gradient_points.begin()),
                     std::make_move_iterator(gradient_points.end()));
    
    auto special_points = generateSpecialPoints<E>(x, exp,
                                                 SamplingStrategy<E>::VARIANT_SAMPLES / 6);
    candidates.insert(candidates.end(),
                     std::make_move_iterator(special_points.begin()),
                     std::make_move_iterator(special_points.end()));
    
    const uint64_t original_mantissa = bits & 0x000FFFFFFFFFFFFF;
    auto heuristic_points = generateHeuristicPoints<E>(x, original_mantissa,
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
inline unsigned int generateSeed(double a, double b) {
    uint64_t hash = std::hash<double>{}(a) + std::hash<double>{}(b);
    return static_cast<unsigned int>(hash);
}

template<ErrorLevel E>
void processIntervalTemplate(double a, double b, vector<IntervalResult>& results) {
    std::vector<std::pair<double, double>> errors;
    errors.reserve(SamplingStrategy<E>::INITIAL_SAMPLES);
    
    const unsigned int seed = static_cast<unsigned int>(a * 1000 + b * 100);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(a, b);

    double quick_max_error = 0.0;
    double quick_max_point = 0.0;
    total_points_tested1 += SamplingStrategy<E>::INITIAL_SAMPLES;

    for (int i = 0; i < SamplingStrategy<E>::INITIAL_SAMPLES; ++i) {
        const double x = dis(gen);
        const double origin = getDoubleOfOrigin(x);
        const double error = getRelativeError(x, origin);
        if (error > quick_max_error) {
            quick_max_error = error;
            quick_max_point = x;
        }
        errors.emplace_back(x, error);
    }

    std::partial_sort(errors.begin(), 
                     errors.begin() + SamplingStrategy<E>::TOP_K,
                     errors.end(),
                     [](const auto& lhs, const auto& rhs) {
                         return lhs.second > rhs.second;
                     });

    double totalUlp = 0.0;
    double maxUlp = quick_max_error;
    double maxUlpPoint = quick_max_point;
    const double interval_size = b - a;

    for (int i = 0; i < SamplingStrategy<E>::TOP_K && i < errors.size(); ++i) {
        auto variants = enhancedSearch<E>(errors[i].first, interval_size);
        total_points_tested1 += variants.size();
        
        for (const auto& variant : variants) {
            const double origin = getDoubleOfOrigin(variant);
            const double error = getRelativeError(variant, origin);
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
        std::scoped_lock lock(mtx1);
        results.push_back({avgUlp, maxUlp, maxUlpPoint, {a, b}});
    }
}

template<ErrorLevel E>
void detectAndRefineTemplate(double x_l, double x_r, int SIZE, double threshold, bool isFinalIteration = false) {
    vector<IntervalResult> results;
    results.reserve(SIZE);
    
    static ThreadPool pool(std::thread::hardware_concurrency());
    std::atomic<int> completed_tasks{0};
    std::mutex completion_mutex;
    std::condition_variable completion_cv;
    
    const int total_tasks = SIZE;
    
    for (int i = 0; i < SIZE; ++i) {
        const double sub_a = x_l + (x_r - x_l) * i / static_cast<double>(SIZE);
        const double sub_b = x_l + (x_r - x_l) * (i + 1) / static_cast<double>(SIZE);

        pool.enqueue([&, sub_a, sub_b] {
            processIntervalTemplate<E>(sub_a, sub_b, ref(results));
            int completed = ++completed_tasks;
            if (completed == total_tasks) {
                completion_cv.notify_one();
            }
        });
    }
    
    {
        std::unique_lock<std::mutex> lock(completion_mutex);
        completion_cv.wait(lock, [&] { return completed_tasks == total_tasks; });
    }
    
    auto maxIt = max_element(results.begin(), results.end(), 
        [](const IntervalResult& lhs, const IntervalResult& rhs) {
            return lhs.avgUlp < rhs.avgUlp;
        });

    if (isFinalIteration) {
        printf("Final Iteration - Maximum Relative Error: %e, at X: %.16lf\n", 
               maxIt->maxUlp, maxIt->maxUlpPoint);
        printf("Total points tested: %llu\n", total_points_tested1.load());
        return;
    }

    const double x_len = fabs(maxIt->interval.second - maxIt->interval.first);
    
    int dynamic_size = SIZE;
    if (x_len < threshold * 10) {
        dynamic_size = std::max(2, SIZE / 2);
    }

    if (x_len > threshold) {
        detectAndRefineTemplate<E>(maxIt->interval.first, maxIt->interval.second, 
                                 dynamic_size, threshold, false);
    } else {
        detectAndRefineTemplate<E>(x_l, x_r, dynamic_size, threshold, true);
    }
}

void detect1(double x1_l, double x1_r) {
    const double initial_max_error = performInitialDetection(x1_l, x1_r);
    const ErrorLevel level = determineErrorLevel(initial_max_error);
    
    const int range = static_cast<int>(x1_r - x1_l);
    const int SIZE = (range <= 10000) ? 100 : (100 + static_cast<int>(ceil(log2(range))));
    
    total_points_tested1 = 0;
    constexpr double THRESHOLD = 1e-9;

    switch(level) {
        case ErrorLevel::MINIMAL:
            detectAndRefineTemplate<ErrorLevel::MINIMAL>(x1_l, x1_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::SMALL:
            detectAndRefineTemplate<ErrorLevel::SMALL>(x1_l, x1_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::MEDIUM:
            detectAndRefineTemplate<ErrorLevel::MEDIUM>(x1_l, x1_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::LARGE:
            detectAndRefineTemplate<ErrorLevel::LARGE>(x1_l, x1_r, SIZE, THRESHOLD);
            break;
        case ErrorLevel::CRITICAL:
            detectAndRefineTemplate<ErrorLevel::CRITICAL>(x1_l, x1_r, SIZE, THRESHOLD);
            break;
    }
}
