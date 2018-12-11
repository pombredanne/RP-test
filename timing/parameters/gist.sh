#!/usr/bin/env bash

MMAP=0
MRPT_SPARSITY=0.032
PARALLEL=0
N_AUTO=1000

MRPT_VOTING_N_TREES="25 50 75 100 125 150 200 250 300 350 400 500 600 700 800 900 1000"
MRPT_DEPTH="8 9 10 11 12 13"
MRPT_VOTES="1 2 3 4 5 8 10 12 15 20 25 30 40 50 60 70 80 90 100"
MRPT_CS_SIZES="20, 30, 40, 50, 60, 70, 80, 90, 100, 125, 150, 175, 200, 250,
               300, 350, 400, 500, 600, 750, 1000, 1250, 1500, 1750, 2000, 3000,
               4000, 5000, 7500, 10000, 12500, 15000, 17000, 20000, 25000,
               30000, 40000, 50000, 60000, 70000, 80000"


MRPT_AUTO_MIN_DEPTH=-1
MRPT_AUTO_MAX_DEPTH=-1
MRPT_AUTO_MAX_TREES=-1
MRPT_AUTO_MAX_VOTES=-1
