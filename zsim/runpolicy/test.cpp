#include "test.h"

bool Test::initializeArguments(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "r:n:")) != -1)
    {
        switch (opt)
        {
        case 'r':
            repl_policy_ = optarg;
            break;
        case 'n':
            max_running_processes_ = std::atoi(optarg);
            break;
        }
    }

    if (!isValidMaxProcesses() || !isValidReplPolicy())
    {
        std::cerr << "Usage: " << argv[0] << " -r <repl_policy> [-n <max_running_processes>]\n";
        std::cerr << "    repl_policy: LRU LFU SRRIP\n";
        std::cerr << "    (optional) max_running_processes: >= 1 (set to 1 by default)\n";

        return false;
    }

    return true;
}

void Test::synchronizeTests()
{
    int child_status;

    for (size_t benchmark_suite_idx = 0; benchmark_suite_idx < benchmark_suites_.size(); ++benchmark_suite_idx)
    {
        BenchmarkSuite curr_suite = benchmark_suites_.at(benchmark_suite_idx);

        for (size_t benchmark_idx = 0; benchmark_idx < curr_suite.getSuiteSize(); ++benchmark_idx)
        {
            int pid = fork();

            if (pid != 0)
            { // If it is the parent, we want to collect the child pid and wait until it hits sigstop.
                child_pids_.push_back(pid);
                pid_benchmark_map_[pid] = curr_suite.getBenchmark(benchmark_idx);
                waitpid(pid, &child_status, WUNTRACED);
            }
            else
            { // If it is the child, we want to stop the process because we only want the processes in the queue to run.
                std::cout << "Synchronizing process for: " << curr_suite.getBenchmark(benchmark_idx) << ", with pid: " << getpid() << std::endl;
                raise(SIGSTOP);
                char *args[] = {
                    const_cast<char *>("./../hw4runscript"),
                    const_cast<char *>(curr_suite.getSuiteName()),
                    const_cast<char *>(curr_suite.getBenchmark(benchmark_idx)),
                    const_cast<char *>(repl_policy_),
                    NULL};
                execvp(args[0], args);
            }
        }
    }
}

void Test::runTests()
{
    std::set<pid_t> running_pids;
    int child_status;

    const int pad_width = 18; // only used for cout statements

    while (!child_pids_.empty() || !running_pids.empty())
    {
        while (!child_pids_.empty() && (running_pids.size() < max_running_processes_))
        {
            pid_t stopped_pid = child_pids_.back();
            child_pids_.pop_back();

            kill(stopped_pid, SIGCONT); // restart the child process

            std::cout << "▶ Running:   "
                      << std::left << std::setw(pad_width) << pid_benchmark_map_[stopped_pid]
                      << " [PID: " << stopped_pid << "]" << std::endl;

            running_pids.insert(stopped_pid);
        }

        pid_t finished_pid = waitpid(-1, &child_status, 0);
        if (finished_pid > 0)
        {
            std::cout << "✔ Finished:  "
                      << std::left << std::setw(pad_width) << pid_benchmark_map_[finished_pid]
                      << " [PID: " << finished_pid << "]" << std::endl;

            running_pids.erase(finished_pid);
        }
    }
}