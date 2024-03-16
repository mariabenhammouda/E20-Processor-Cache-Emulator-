# E20-Processor-Cache-Emulator-
## Overview
This repository contains a cache simulator implemented in C++. The simulator is capable of simulating both single-level and two-level caches and can execute machine code stored in a binary file. The cache simulator supports various cache configurations, including cache size, associativity, and block size.

## Usage
To use the cache simulator, follow these steps:

Clone the repository to your local machine.
Compile the source code using a C++ compiler.
Run the compiled executable with the following command-line arguments:
bash
Copy code
./cache_simulator [options] filename
Options
-h, --help: Show the help message and exit.
--cache CACHE: Specify the cache configuration in the format size,associativity,blocksize for a single cache, or size,associativity,blocksize,size,associativity,blocksize for two caches.

I've added test cases, including edge cases, to thoroughly test the cache simulator.

## Example
I use Anubis LMS to execute the program:
g++ -Wall -o sim sim_cache.cpp
./sim input.bin --cache 4,1,1

[0] % g++ -Wall -o sim sim_cache.cpp

anubis@anubis-ide : ~
[0] % ./sim write-through.bin --cache 4,1,1


Cache L1 has size 4, associativity 1, blocksize 1, rows 4

L1 MISS  pc:    1       addr:   50      row:   2

L1 SW    pc:    2       addr:   50      row:   2

L1 HIT   pc:    3       addr:   50      row:   2


## File Structure
sim_cache.cpp: The main source code file containing the cache simulator implementation.
program.bin: Example binary file containing machine code to be executed by the simulator.

## Cache Configuration
The cache configuration is specified using the --cache option followed by the cache parameters:
size: The total size of the cache in bytes.
associativity: The associativity of the cache.
blocksize: The size of each cache block in bytes.
For a two-level cache, provide the parameters for both L1 and L2 caches separated by commas.

