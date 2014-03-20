default : benchmarks

.PHUNY : benchmarks

primes : primes.cpp
	g++ -g -o primes primes.cpp -lCoinUtils

benchmarks : primes
	./generate.sh

clean : 
	rm -Rf primes *.smt *.mps
	