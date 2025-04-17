#include "benchmark_suite.h"

BenchmarkSuite::BenchmarkSuite() : suite_name_(""), benchmarks_(), num_benchmarks_(0) {}

BenchmarkSuite::BenchmarkSuite(const char *suite_name, const std::vector<const char *> &benchmarks)
    : suite_name_(suite_name), benchmarks_(benchmarks), num_benchmarks_(benchmarks.size()) {}

void BenchmarkSuite::setBenchmarks(const std::vector<const char *> &benchmarks)
{
    benchmarks_ = benchmarks;
    num_benchmarks_ = benchmarks_.size();
}

const char *BenchmarkSuite::getBenchmark(int i)
{
    return benchmarks_.at(i);
}

void BenchmarkSuite::setSuiteName(const char *suite_name)
{
    suite_name_ = suite_name;
}

const char *BenchmarkSuite::getSuiteName()
{
    return suite_name_;
}

const size_t BenchmarkSuite::getSuiteSize()
{
    return num_benchmarks_;
}
