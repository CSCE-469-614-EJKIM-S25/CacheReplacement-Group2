#include "benchmark_suite.h"
#include "test.h"

int main(int argc, char **argv)
{

    Test test;

    bool args_correct = test.initializeArguments(argc, argv);

    if (args_correct == false)
        exit(1);

    /*
     * The PARSEC suite contains benchmark programs that are meant to test parallel
     * processors and multi-core architectures. Further details on PARSEC can be found below.
     * https://www.cs.princeton.edu/techreports/2008/811.pdf
     *
     **/
    BenchmarkSuite PARSEC(const_cast<char *>("PARSEC"), {"blackscholes", "bodytrack", "canneal", "dedup",
                                                         "fluidanimate", "freqmine", "streamcluster",
                                                         "swaptions", "x264"});

    /*
     * The SPEC suite contains benchmark programs that test the performance of processors.
     * Further details on the SPEC benchmark suite can be found below.
     * https://www.spec.org/cpu2006/CINT2006/
     *
     **/
    BenchmarkSuite SPEC(const_cast<char *>("SPEC"), {"bzip2", "gcc", "mcf", "hmmer", "sjeng", "libquantum",
                                                     "xalancbmk", "milc", "cactusADM", "leslie3d", "namd",
                                                     "soplex", "calculix", "lbm"});

    test.addBenchmarkSuite(PARSEC);
    test.addBenchmarkSuite(SPEC);

    std::cout << "--------------- Synchronizing " << test.getReplPolicy() << " child processes -----------------" << std::endl;
    std::cout << std::endl;

    test.synchronizeTests();

    std::cout << "--------------- " << test.getReplPolicy() << " child processes SUCCESSFULLY synchronized -----------------" << std::endl;
    std::cout << std::endl;

    std::cout << "--------------- STARTING " << test.getReplPolicy() << " tests!  |";
    std::cout << " Will run, at maximum, " << test.getMaxProcesses() << " at a time -----------------" << std::endl;
    std::cout << std::endl;

    test.runTests();

    std::cout << std::endl;
    std::cout << "--------------- FINISHED " << test.getReplPolicy() << " child processes -----------------" << std::endl;

    return 0;
}