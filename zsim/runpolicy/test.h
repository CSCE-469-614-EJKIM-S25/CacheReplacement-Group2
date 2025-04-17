#ifndef TEST_H_
#define TEST_H_

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include <iomanip>

#include "benchmark_suite.h"

class Test
{
private:
    const char *repl_policy_; // std:string view? --> look into this
    int max_running_processes_;

    static inline const std::set<std::string> repl_policies = {"LRU", "LFU", "SRRIP"};

    std::vector<pid_t> child_pids_;
    std::vector<BenchmarkSuite> benchmark_suites_;
    std::unordered_map<pid_t, const char *> pid_benchmark_map_;

    bool isValidMaxProcesses() { return (max_running_processes_ >= 1); }
    bool isValidReplPolicy() { return (repl_policies.find(repl_policy_) != repl_policies.end()); }

public:
    Test() : repl_policy_(""), max_running_processes_(1) {}
    Test(char *repl_policy, int max_running_processes) : repl_policy_(repl_policy), max_running_processes_(max_running_processes) {}

    const char *getReplPolicy() { return repl_policy_; }
    const int getMaxProcesses() { return max_running_processes_; }

    void addBenchmarkSuite(const BenchmarkSuite &benchmark_suite) { benchmark_suites_.push_back(benchmark_suite); }

    /**
     * @brief Sets test arguments replacement policy and max running processes
     * from command line.
     *
     * @param argc Number of command line arguments.
     * @param arvg Command line arguments.
     *
     * @return Returns 1 if the arguments were set correctly, 0 otherwise.
     */
    bool initializeArguments(int argc, char **argv);

    /**
     * @brief Synchronizes benchmark processes by starting
     * the benchmark, recording the process ID, then stopping
     * the benchmark so that the benchmarks can be run in a
     * controlled manner
     */
    void synchronizeTests();

    /**
     * @brief Runs benchmarks in a controlled manner such that
     * the total number of running benchmarks does not exceed
     * max_running_processes_
     */
    void runTests();
};

#endif // TEST_H_