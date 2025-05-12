#!/bin/bash

# Compile
mpicc -o sudoku_mpi sudoku_mpi.c sudoku_utils.c

# Run with 4 processes
mpirun -np 4 ./sudoku_mpi
