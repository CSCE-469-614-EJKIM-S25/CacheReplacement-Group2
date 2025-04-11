#include <benchmark_suite.h>
#include <test.h>

int main(int argc, char **argv)
{

    Test Test;

    bool args_correct = Test.initializeArguments(argc, argv);

    if (args_correct == false)
        exit(1);

    // define benchmark suites
    BenchmarkSuite PARSEC(const_cast<char *>("PARSEC"), {"blackscholes", "bodytrack", "canneal", "dedup",
                                                         "fluidanimate", "freqmine", "streamcluster",
                                                         "swaptions", "x264"});
    BenchmarkSuite SPEC(const_cast<char *>("SPEC"), {"bzip2", "gcc", "mcf", "hmmer", "sjeng", "libquantum",
                                                     "xalancbmk", "milc", "cactusADM", "leslie3d", "namd",
                                                     "soplex", "calculix", "lbm"});

    // add benchmark suites to the test
    Test.addBenchmarkSuite(PARSEC);
    Test.addBenchmarkSuite(SPEC);

    // synchronize benchmark
    std::cout << "--------------- Synchronizing " << Test.getReplPolicy() << " child processes -----------------" << std::endl;
    std::cout << std::endl;

    Test.synchronizeTests();

    std::cout << "--------------- " << Test.getReplPolicy() << " child processes SUCCESSFULLY synchronized -----------------" << std::endl;
    std::cout << std::endl;

    // run benchmarks
    std::cout << "--------------- STARTING " << Test.getReplPolicy() << " tests!  |";
    std::cout << " Will run, at maximum, " << Test.getMaxProcesses() << " at a time -----------------" << std::endl;
    std::cout << std::endl;

    Test.runTests();

    std::cout << std::endl;
    std::cout << "--------------- FINISHED " << Test.getReplPolicy() << " child processes -----------------" << std::endl;

    return 0;
}