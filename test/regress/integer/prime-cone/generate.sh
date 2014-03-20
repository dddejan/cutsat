#!/bin/bash
N=20
for i in `seq 2 1 $N`; do ./primes $i 1 1 > prime-cone-sat-$i.smt; done;
for i in `seq 2 1 $N`; do ./primes $i 0 1 > prime-cone-unsat-$i.smt; done;
for i in `seq 2 1 $N`; do ./primes $i 1 0 prime-cone-sat-$i.mps; done;
for i in `seq 2 1 $N`; do ./primes $i 0 0 prime-cone-unsat-$i.mps; done;
