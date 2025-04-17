# casim
Computer Architecture Simulation Infrastructure for CSCE 614 Computer Architecture


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

### 3. Compile zsim

```
$ cd zsim
$ scons -j4
```

You need to compile the code each time you make a change. However, keep in mind that the build scripts detailed below will rebuild zsim each time you run them.

### 4. Running Benchmarks

There are two different programs that allow you to run the benchmarks: a shell script titled `hw4runscript` and a .cpp program titled `run_policy.cpp` (located in `runpolicy/`). Instructions on how to run these are detailed below.

#### 4a. hw4runscript 

The `hw4runscript` is a shell script that tests a single replacement policy on a single benchmark. The correct usage of the file is as follows:
```
$ ./hw4runscript <suite> <benchmark> <repl_policy> &
```
where
```
(suite) benchmarks: 
    -- (SPEC) bzip2 gcc mcf hmmer sjeng libquantum xalancbmk milc cactusADM leslie3d namd soplex calculix lbm
    -- (PARSEC) blackscholes bodytrack canneal dedup fluidanimate freqmine streamcluster swaptions x264
repl_policy: LRU LFU SRRIP
```
The `&` is used so that the script can be run as a background process and you can continue to use the terminal. However, it is recommended to use `tmux`, a terminal multiplexer, to run the benchmarks. More details are below in 4c.

#### 4b. run_policy.cpp & runpolicy.o

The `run_policy.cpp` is a .cpp program that tests a single replacement policy on all the benchmarks required for this homework. This file is located in the `run_policy/` folder in `zsim`. To create the executable from the Makefile, one can run the `make` command in the zsim folder. The Makefile will compile the files `benchmark_suite.cpp`, `test.cpp`, and `run_policy.cpp`, create the executable `runpolicy.o`, and place the executable in the `zsim` folder. The correct usage of the Makefile and the executable is as follows:
```
$ make clean 
$ make
$ ./runpolicy.o -r <repl_policy> [-n <max_running_processes>] &
```
where
```
repl_policy: LRU LFU SRRIP
(optional) max_running_processes: >= 1 (set to 1 by default)
```
Once again, the `&` is used so the script can be run as a background process. 

**IMPORTANT NOTES**:
- One thing to note is that the `<max_running_processes>` should be changed with EXTREME CAUTION. Ensure that you check with your system administrator to ensure you do not take up too many resources.
- Furthermore, be mindful of when you are `make`-ing; ensure that you do not overwrite the binary while you are currently running a replcement policy with the executable. More information can be found below in 4c.

#### 4c. Using tmux

It is recommended that you use `tmux`, a terminal multiplexer, to run the benchmarks so that the benchmarks continue to run even if you are disconnected from your session.

Examples of how you would use `tmux` for running both the `hw4runscript` and the `runpolicy.cpp` are shown below. Keep in mind that the naming conventions used in the examples are optional; however, make sure you don't recompile and accidentally overwrite an existing executable that is running.

##### hw4runscript with tmux
```
$ tmux new -s run_<repl_policy>_<benchmark>
$ ./hw4runscript <suite> <benchmark> <repl_policy> &
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
tmux ls
```

To check on the benchmarks running in a `tmux` session, simply reconnect to the window of your choosing
```
tmux attach-session -t <session_name>
```

###### For more information, check `zsim/README.md`
