default : benchmarks

.PHONY : benchmarks

pigeon-hole : pigeon-hole.cpp
	g++ -g -o pigeon-hole pigeon-hole.cpp -lCoinUtils

benchmarks : pigeon-hole
	chmod a+x generate.sh
	./generate.sh

clean : 
	rm -Rf *.smt
	rm -Rf *.mps
	rm -Rf *.opb

	