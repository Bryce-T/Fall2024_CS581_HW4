#!/bin/bash
source /apps/profiles/modules_asax.sh.dyn
module load openmpi/4.1.4-gcc11
mpicc - -Wall -O -o homework4 homework4.c
mpirun -n 8 hw4 5000 5000 scratch/$USER/
