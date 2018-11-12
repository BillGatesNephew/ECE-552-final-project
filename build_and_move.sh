#!/bin/bash 

# Compile HyrdaScalar 
cd ./hydra_1.0c;
make clean;
make hydra; 

# Move Binary into Benchmarks
mv ./hydra ../working-benchmarks/BenchMarks_Little/hydra
