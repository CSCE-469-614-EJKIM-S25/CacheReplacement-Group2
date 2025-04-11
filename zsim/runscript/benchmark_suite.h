#ifndef BENCHMARK_SUITE_H_
#define BENCHMARK_SUITE_H_

#include <vector>

#include <cstddef>

class BenchmarkSuite
{
private:
    const char *suite_name_;
    std::vector<const char *> benchmarks_;
    size_t num_benchmarks_;

public:
    BenchmarkSuite();
    BenchmarkSuite(const char *suite_name, const std::vector<const char *> &benchmarks);

    void setBenchmarks(const std::vector<const char *> &benchmarks);
    const char *getBenchmark(int i);

    void setSuiteName(const char *suite_name);
    const char *getSuiteName();

    const size_t getSuiteSize();
};

#endif // BENCHMARK_SUITE_H_