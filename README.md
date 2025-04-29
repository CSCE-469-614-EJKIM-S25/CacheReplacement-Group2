# casim
Computer Architecture Simulation Infrastructure for CSCE 614 Computer Architecture

## Description
The goal of this project is to design the best-performing cache replacement policy for a last-level cache using the ZSim simulator. 

Our team decided to pursue and implement two different cache replacement policies:
- SHiP (Signature-based Hit Predictor for High Performance Caching)
- EHC (Expected Hit Count)
A further description of these cache replacement policies is below.

In order to implement these cache replacement policies, we edited/created the following files:
- init.cpp
- repl_ehc.h
- repl_ship.h

#### SHiP Description
The SHiP (Signature-based Hit Predictor) cache replacement policy improves on traditional policies by using program context (signatures) to predict a block’s likelihood of reuse. Each cache access updates a re-reference prediction value (RRPV) based on the signature, which encodes some attribute of the instruction that brought the block into the cache. This signature is unique to that instruction and can be derived from a few different instruction attributes, including: the memory region of the address being referenced, the PC of the instruction that calls for the memory, and the instruction sequence history for the memory reference. SHiP learns which instructions produce cache-friendly data and prioritizes retaining those blocks, reducing cache misses more effectively than LRU.

For the team's implementation, we decided to create the sigature using the memory region of the address being referenced. 

#### EHC Description
The EHC (Expected Hit Count) cache replacement policy is a predictive algorithm that replaces the cache block with the lowest expected number of future hits. Instead of relying solely on traditional metrics like recency or frequency, EHC uses past access patterns to estimate how many times each block is likely to be reused before eviction, aiming to retain the most valuable data in the cache.

### References
- C. -J. Wu, A. Jaleel, W. Hasenplaugh, M. Martonosi, S. C. Steely and J. Emer, "SHiP: Signature-based Hit Predictor for high performance caching," 2011 44th Annual IEEE/ACM International Symposium on Microarchitecture (MICRO), Porto Alegre, Brazil, 2011, pp. 430-441. keywords: {Marine vehicles;Proposals;Radiation detectors;Hardware;History;Servers;Art;Replacement;Reuse Distance Prediction;Shared Cache}
- A. Vakil-Ghahani, S. Mahdizadeh-Shahri, M. -R. Lotfi-Namin, M. Bakhshalipour, P. Lotfi-Kamran and H. Sarbazi-Azad, "Cache Replacement Policy Based on Expected Hit Count," in IEEE Computer Architecture Letters, vol. 17, no. 1, pp. 64-67, 1 Jan.-June 2018, doi: 10.1109/LCA.2017.2762660. keywords: {Radiation detectors;Proposals;Correlation;History;Multicore processing;Prefetching;Memory system;memory-intensive workload;last-level cache;replacement policy;Belady's MIN}

## Structure
The project structure is very similar to the repositories that were given to us for homework 2 and 4. An overview of the file structure can be found at the bottom of the `README.md`.

The root directory contains the infrastructuer build zsim and test the benchmarks. These folders include:
- `benchmarks/` folder -> contains the benchmark programs and their inputs
- `tools/` folder -> contains the intel pin tool, which is needed to trace the benchmarks and their programs
- `venv/` folder -> the python virtual environment needed to build zsim
- `zsim/` folder -> the directory actually contains the zsim simulator

The `zsim/` directory contains several notable folders needed for actually running zsim. These folders include:
- `configs/` folder -> this folder contains the different configs for running a replacement policy with a benchmark
- `outputs/` folder -> this folder contains the actual outputs for each replacement policy; each replacement policy contains results for all the benchmarks available
- `src/` folder -> this folder contains the source code for the zsim simulator; one is able to edit how the zsim simulator functions by editing this source code, and this is the folder that contains most of the files our team needed to edit in order to implement EHC and SHiP

However, some differences do exist between the repositories in homework 2 and 4 and the term project.

The first difference is how tests are run. The folder `runpolicy/` was created in `zsim/` to automate the running of the benchmarks on the cache replacement policies. The running of each replacement policy is still manual, but the new testing automation will automatically run all the benchmarks for a specific policy. Details on how to work this new testing infrastructure is detailed in a later section.

Furthermore, there is now a new file, titled `term_project_runscript` that was created to place all test results in the appropriate term project results folder. This file virtually performs the same function as the `hw4runscript`, but it has minor edits that allow for the automated testing infrastructure to work. 

## Environment Setup

### 1. Unzip benchmarks files

```
zip -F benchmarks.zip --out single-benchmark.zip && unzip single-benchmark.zip && mkdir benchmarks/parsec-2.1/inputs/streamcluster
```

### 2. Environemnt setup

To set up the Python environment for the first time, run the following commands.

```
$ python -m venv venv
$ source venv/bin/activate
$ pip install scons
```

Everytime you want to build or run zsim, you need to setup the environment variables first.

```
$ source venv/bin/activate
$ source setup_env
```

## Compilation Instructiosn

### Compile zsim

After setting up the virtual environment and the environment variables (above instructions), navigate from the root directory to the `zsim/` directory. In the `zsim/` directory, run the following command.
```
$ scons -j4
```

You need to compile the code each time you make a change. However, keep in mind that the build scripts detailed below will rebuild zsim each time you run them.

## Run Commands

### 1. Running Benchmarks

There are two different programs that allow you to run the benchmarks: a shell script titled `term_project_runscript` and a .cpp program titled `run_policy.cpp` (located in `runpolicy/`). Instructions on how to run these are detailed below.

Before running any benchmarks, ensure that the environment is setup properly and zsim is built. For instructions on those tasks, please look above.

#### 1a. term_project_runscript 

The `term_project_runscript` is a shell script that tests a single replacement policy on a single benchmark. The correct usage of the file is as follows:
```
$ ./term_project_runscript <suite> <benchmark> <repl_policy> &
```
where
```
(suite) benchmarks: 
    -- (SPEC) bzip2 gcc mcf hmmer sjeng libquantum xalancbmk milc cactusADM leslie3d namd soplex calculix lbm
    -- (PARSEC) blackscholes bodytrack canneal dedup fluidanimate freqmine streamcluster swaptions x264
repl_policy: LRU LFU SRRIP TreeLRU Rand NRU EHC SHiP
```
The `&` is used so that the script can be run as a background process and you can continue to use the terminal. However, it is recommended to use `tmux`, a terminal multiplexer, to run the benchmarks. More details are below in 2.

#### 1b. run_policy.cpp & runpolicy.o

The `run_policy.cpp` is a .cpp program that tests a single replacement policy on all the benchmarks required for this homework. This file is located in the `run_policy/` folder in `zsim`. To create the executable from the Makefile, one can run the `make` command in the zsim folder. The Makefile will compile the files `benchmark_suite.cpp`, `test.cpp`, and `run_policy.cpp`, create the executable `runpolicy.o`, and place the executable in the `zsim` folder. The correct usage of the Makefile and the executable is as follows:
```
$ make clean 
$ make
$ ./runpolicy.o -r <repl_policy> [-n <max_running_processes>] &
```
where
```
repl_policy: LRU LFU SRRIP TreeLRU Rand NRU EHC SHiP
(optional) max_running_processes: >= 1 (set to 1 by default)
```
Once again, the `&` is used so the script can be run as a background process. 

**IMPORTANT NOTES**:
- One thing to note is that the `<max_running_processes>` should be changed with EXTREME CAUTION. Ensure that you check with your system administrator to ensure you do not take up too many resources.
- Furthermore, be mindful of when you are `make`-ing; ensure that you do not overwrite the binary while you are currently running a replcement policy with the executable. More information can be found below in 4c.

### 2. Using tmux

It is recommended that you use `tmux`, a terminal multiplexer, to run the benchmarks so that the benchmarks continue to run even if you are disconnected from your session.

Examples of how you would use `tmux` for running both the `term_project_runscript` and the `runpolicy.cpp` are shown below. Keep in mind that the naming conventions used in the examples are optional; however, make sure you don't recompile and accidentally overwrite an existing executable that is running.

##### term_project_runscript with tmux
```
$ tmux new -s run_<repl_policy>_<benchmark>
$ ./term_project_runscript <suite> <benchmark> <repl_policy> &
```

##### run_policy.cpp & runpolicy.o with tmux
**NOTE**: If you `make` the `runpolicy.o` executable while you are currently testing a replacement policy with the `runpolicy.o`, it is likely that you will overwrite the current binary and the program execution will fail. If the `runpolicy.o` executable is already created, then there is no need to re`make` the executable; simply run the executable with the desired replacement policy. 
```
$ tmux new -s run_<repl_policy>
$ ./runpolicy.o -r <repl_policy> -n <max_running_processes> &
```
To exit out of the `tmux` session: <kbd>Ctrl</kbd> + <kbd>b</kbd>, <kbd>d</kbd>

To see the `tmux` sessions you have up, simply do:
```
$ tmux ls
```

To check on the benchmarks running in a `tmux` session, simply reconnect to the window of your choosing
```
$ tmux attach-session -t <session_name>
```

## Output File Format
The output files are all located in the following file path:
```
/zsim/outputs/term_project/<replacement_policy>/<benchmark>/zsim.out
```

The output file format will depend on the benchmark suite that is run (SPEC or PARSEC).

### SPEC
The SPEC benchmark suite has programs that are ran single threaded, which means there is only one shared L3 cache. The format of the output that is relevant to us is how the CPU and L3 cache performs. 

The core stats are below. We specifically care about the `cycles`, `cCycles`, and `instrs` because we want to compute the approximate IPC (instructions per cycle) the CPU achieves while running a benchmark with a specific cache replacement policy. 
```
 westmere: # Core stats
  westmere-0: # Core stats
   cycles: 363332375 # Simulated unhalted cycles
   cCycles: 66514946 # Cycles due to contention stalls
   instrs: 100001101 # Simulated instructions
   uops: 114163730 # Retired micro-ops
   bbls: 2330424 # Basic blocks
   approxInstrs: 553218 # Instrs with approx uop decoding
   mispredBranches: 3146 # Mispredicted branches
   condBranches: 916781 # conditional branches
```
The cache states are below. We care about the `hGETS`, `hGETX`, `mGETS`, and `mGETXIM`, as this allows us to get the MPKI (misses per 1000 instructions) and the total misses.
```
l3: # Cache stats
  l3-0: # Cache stats
   hGETS: 1 # GETS hits
   hGETX: 23 # GETX hits
   mGETS: 2579024 # GETS misses
   mGETXIM: 4178269 # GETX I->M misses
   mGETXSM: 0 # GETX S->M misses (upgrade misses)
   PUTS: 59845 # Clean evictions (from lower level)
   PUTX: 122309 # Dirty evictions (from lower level)
   INV: 0 # Invalidates (from upper level)
   INVX: 0 # Downgrades (from upper level)
   FWD: 0 # Forwards (from upper level)
   latGETnl: 608156370 # GET request latency on next level
   latGETnet: 0 # GET request latency on network to next level
```

### PARSEC
The PARSEC benchmark suite has programs that are ran multi-threaded, which means there is a core and shared L3 cache per thread. This means that we need to sum all the core and L3 stats for every thread to create our needed evaluation statistics. One again, the format of the output that is relevant to us is how the CPU and L3 cache performs. 

The core stats are below. We specifically care about the `cycles`, `cCycles`, and `instrs` because we want to compute the approximate IPC (instructions per cycle) the CPU achieves while running a benchmark with a specific cache replacement policy. In order to get the overall IPC, we will sum all the core `cycles` and `cCycles` and all the core `instrs`.
```
 westmere: # Core stats
  westmere-0: # Core stats
   cycles: 1592389818 # Simulated unhalted cycles
   cCycles: 42264119 # Cycles due to contention stalls
   instrs: 2282096573 # Simulated instructions
   uops: 3297268918 # Retired micro-ops
   bbls: 610425754 # Basic blocks
   approxInstrs: 162199 # Instrs with approx uop decoding
   mispredBranches: 9707278 # Mispredicted branches
   condBranches: 530240438 # conditional branches
  westmere-1: # Core stats
   cycles: 316312199 # Simulated unhalted cycles
   cCycles: 13670232 # Cycles due to contention stalls
   instrs: 403993086 # Simulated instructions
   uops: 486437249 # Retired micro-ops
   bbls: 125070440 # Basic blocks
   approxInstrs: 175814 # Instrs with approx uop decoding
   mispredBranches: 1778994 # Mispredicted branches
   condBranches: 124687808 # conditional branches
  westmere-2: # Core stats
   cycles: 301930758 # Simulated unhalted cycles
   cCycles: 13340451 # Cycles due to contention stalls
   instrs: 380556183 # Simulated instructions
   uops: 457190260 # Retired micro-ops
   bbls: 117910585 # Basic blocks
   approxInstrs: 160836 # Instrs with approx uop decoding
   mispredBranches: 1611753 # Mispredicted branches
   condBranches: 117587886 # conditional branches
  westmere-3: # Core stats
   cycles: 306985411 # Simulated unhalted cycles
   cCycles: 13355264 # Cycles due to contention stalls
   instrs: 389587182 # Simulated instructions
   uops: 467799601 # Retired micro-ops
   bbls: 120789260 # Basic blocks
   approxInstrs: 165241 # Instrs with approx uop decoding
   mispredBranches: 1683758 # Mispredicted branches
   condBranches: 120399567 # conditional branches
  westmere-4: # Core stats
    .... (continued)
```

The cache states are below. We care about the `hGETS`, `hGETX`, `mGETS`, and `mGETXIM`, as this allows us to get the MPKI (misses per 1000 instructions) and the total misses. In order to get the overall total misses, we will need to sum all the `mGETS` and `mGETXIM`, and to get the total MPKI, we will also need to sum up all the `instrs`.
```
l3: # Cache stats
  l3-0b0: # Cache stats
   hGETS: 512684 # GETS hits
   hGETX: 18665 # GETX hits
   mGETS: 147997 # GETS misses
   mGETXIM: 361411 # GETX I->M misses
   mGETXSM: 0 # GETX S->M misses (upgrade misses)
   PUTS: 577716 # Clean evictions (from lower level)
   PUTX: 456517 # Dirty evictions (from lower level)
   INV: 0 # Invalidates (from upper level)
   INVX: 0 # Downgrades (from upper level)
   FWD: 0 # Forwards (from upper level)
   latGETnl: 45846720 # GET request latency on next level
   latGETnet: 0 # GET request latency on network to next level
  l3-0b1: # Cache stats
   hGETS: 517744 # GETS hits
   hGETX: 18879 # GETX hits
   mGETS: 148484 # GETS misses
   mGETXIM: 361631 # GETX I->M misses
   mGETXSM: 0 # GETX S->M misses (upgrade misses)
   PUTS: 582335 # Clean evictions (from lower level)
   PUTX: 457281 # Dirty evictions (from lower level)
   INV: 0 # Invalidates (from upper level)
   INVX: 0 # Downgrades (from upper level)
   FWD: 0 # Forwards (from upper level)
   latGETnl: 45910350 # GET request latency on next level
   latGETnet: 0 # GET request latency on network to next level
  l3-0b2: # Cache stats
   hGETS: 525403 # GETS hits
   hGETX: 19008 # GETX hits
   mGETS: 148404 # GETS misses
   mGETXIM: 361730 # GETX I->M misses
   mGETXSM: 0 # GETX S->M misses (upgrade misses)
   PUTS: 589692 # Clean evictions (from lower level)
   PUTX: 458422 # Dirty evictions (from lower level)
   INV: 0 # Invalidates (from upper level)
   INVX: 0 # Downgrades (from upper level)
   FWD: 0 # Forwards (from upper level)
   latGETnl: 45912060 # GET request latency on next level
   latGETnet: 0 # GET request latency on network to next level
  l3-0b3: # Cache stats
   hGETS: 538520 # GETS hits
   hGETX: 19052 # GETX hits
   mGETS: 148720 # GETS misses
   mGETXIM: 361651 # GETX I->M misses
   mGETXSM: 0 # GETX S->M misses (upgrade misses)
   PUTS: 604900 # Clean evictions (from lower level)
   PUTX: 456501 # Dirty evictions (from lower level)
   INV: 0 # Invalidates (from upper level)
   INVX: 0 # Downgrades (from upper level)
   FWD: 0 # Forwards (from upper level)
   latGETnl: 45933390 # GET request latency on next level
   latGETnet: 0 # GET request latency on network to next level
  l3-0b4: # Cache stats
    .... (continued)
```

## Folder Paths

The complete folder paths for the project are below.
```
.
├── README.md
├── benchmarks.z01
├── benchmarks.z02
├── benchmarks.z03
├── benchmarks.zip
├── cse_server.patch
├── setup_env
├── tools
│   └── ... (contains intel pin tool and other simulation tools)
├── venv
│   └── ... (python virtual environment)
└── zsim
    ├── LICENSE
    ├── Makefile
    ├── README.md
    ├── README.stats
    ├── SConstruct
    ├── configs
    │   └── term_project
    │       ├── EHC
    │       │   ├── blackscholes_8c_simlarge.cfg
    │       │   ├── bodytrack_8c_simlarge.cfg
    │       │   ├── bzip2.cfg
    │       │   ├── cactusADM.cfg
    │       │   ├── calculix.cfg
    │       │   ├── canneal_8c_simlarge.cfg
    │       │   ├── dedup_8c_simlarge.cfg
    │       │   ├── fluidanimate_8c_simlarge.cfg
    │       │   ├── freqmine_8c_simlarge.cfg
    │       │   ├── gcc.cfg
    │       │   ├── hmmer.cfg
    │       │   ├── lbm.cfg
    │       │   ├── leslie3d.cfg
    │       │   ├── libquantum.cfg
    │       │   ├── mcf.cfg
    │       │   ├── milc.cfg
    │       │   ├── namd.cfg
    │       │   ├── sjeng.cfg
    │       │   ├── soplex.cfg
    │       │   ├── streamcluster_8c_simlarge.cfg
    │       │   ├── swaptions_8c_simlarge.cfg
    │       │   ├── x264_8c_simlarge.cfg
    │       │   └── xalan.cfg
    │       ├── LFU
    │       │   └── ...
    │       ├── LRU
    │       │   └── ...
    │       ├── NRU
    │       │   └── ...
    │       ├── Rand
    │       │   └── ...
    │       ├── SHiP
    │       │   └── ...
    │       ├── SRRIP
    │       │   └── ...
    │       └── TreeLRU
    │           └── ...
    ├── outputs
    │   └── term_project
    │       ├── EHC
    │       │   ├── blackscholes_8c_simlarge
    │       │   │   ├── bodytrack.log
    │       │   │   ├── heartbeat
    │       │   │   ├── out.cfg
    │       │   │   ├── zsim-cmp.h5
    │       │   │   ├── zsim-ev.h5
    │       │   │   └── xalan.cfg
    │       │   ├── bodytrack_8c_simlarge
    │       │   │   └── ...
    │       │   ├── cactusADM
    │       │   │   └── ...
    │       │   ├── calculix
    │       │   │   └── ...
    │       │   ├── canneal_8c_simlarge
    │       │   │   └── ...
    │       │   ├── dedup_8c_simlarge
    │       │   │   └── ...
    │       │   ├── fluidanimate_8c_simlarge
    │       │   │   └── ...
    │       │   ├── freqmine_8c_simlarge
    │       │   │   └── ...
    │       │   ├── gcc
    │       │   │   └── ...
    │       │   ├── hmmer
    │       │   │   └── ...
    │       │   ├── lbm
    │       │   │   └── ...
    │       │   ├── leslie3d
    │       │   │   └── ...
    │       │   ├── libquantum
    │       │   │   └── ...
    │       │   ├── mcf
    │       │   │   └── ...
    │       │   ├── milc
    │       │   │   └── ...
    │       │   ├── namd
    │       │   │   └── ...
    │       │   ├── sjeng
    │       │   │   └── ...
    │       │   ├── soplex
    │       │   │   └── ...
    │       │   ├── streamcluster_8c_simlarge
    │       │   │   └── ...
    │       │   ├── swaptions_8c_simlarge
    │       │   │   └── ...
    │       │   ├── x264_8c_simlarge
    │       │   │   └── ...
    │       │   └── xalan
    │       │       └── ...
    │       ├── LFU
    │       │   └── ...
    │       ├── LRU
    │       │   └── ...
    │       ├── NRU
    │       │   └── ...
    │       ├── Rand
    │       │   └── ...
    │       ├── SHiP
    │       │   └── ...
    │       ├── SRRIP
    │       │   └── ...
    │       └── TreeLRU
    │           └── ...
    ├── hw2runscript
    ├── hw4runscript
    ├── misc
    │   └── ...
    ├── runpolicy
    │   ├── benchmark_suite.cpp
    │   ├── benchmark_suite.h
    │   ├── run_policy.cpp
    │   ├── test.cpp
    │   └── test.h
    ├── src
	│   ├── ... (other zsim source files)
    │   ├── repl_ehc.h
    │   ├── repl_policies.h
    │   ├── repl_rrip.h 
    │   ├── repl_ship.h 
	│   └── ... (other zsim source files)
    ├── term_project_runscript
    └── tests
        └── ...
```

###### For more information, check `zsim/README.md`