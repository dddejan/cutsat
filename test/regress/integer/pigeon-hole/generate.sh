#!/bin/bash
N=20
for i in `seq 2 1 $N`; do ./pigeon-hole $i 1 > pigeon-hole-$i.smt; done;
for i in `seq 2 1 $N`; do ./pigeon-hole $i 0 > pigeon-hole-$i.opb; done;
