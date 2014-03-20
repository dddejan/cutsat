#include <iostream>
#include <cstdlib>

#include <coin/CoinMpsIO.hpp>

using namespace std;

void opb(int n) {

		// Variable p_i_j means pidgeon i in hole j
		// We have n+1 pidgeons and n holes
		int v[n+1][n];
		unsigned n_vars = 0;
		for(unsigned i = 0; i < n+1; ++ i) {
			for(unsigned j = 0; j < n; ++ j) {
				v[i][j] = ++ n_vars;
			}
		}

		cout << "* #variable= " << n_vars << " #constraint= " << 2*n+1 << std::endl;

		// Each pidgeon goes to at least one hole
		// sum_j p_ij >= 1
		for (unsigned i = 0; i < n+1; ++ i) {
			for (unsigned j = 0; j < n; ++ j) {\
				cout << "+1 x" << v[i][j] << " ";
			}
			cout << ">=  1 ;" << endl;
		}

		// Each hole has at most one pidgeon
		// sum_i p_ij <= 1
		for (unsigned j = 0; j < n; ++ j) {
			for (unsigned i = 0; i < n+1; ++ i) {
				cout << "-1 x" << v[i][j] << " ";
			}
			cout << ">= -1 ;" << endl;
		}
}

void smt(int n) {
	cout << "(benchmark pigeonhole" << endl;
	cout << ":status unsat" << endl;
	cout << ":category { crafted }" << endl;
	cout << ":logic QF_LIA" << endl;

	// Variable p_i_j means pidgeon i in hole j
	// We have n+1 pidgeons and n holes
	cout << ":extrafuns (";
	for(unsigned i = 0; i < n+1; ++ i) {
		for(unsigned j = 0; j < n; ++ j) {
			cout << "(p_" << i << "_" << j << " Int) ";
		}
	}
	cout << ")" << endl;

	// Constrain the variables
	for (unsigned i = 0; i < n+1; ++ i) {
		for(unsigned j = 0; j < n; ++ j) {
			cout << ":assumption (>= p_" << i << "_" << j << " 0)" << endl;
			cout << ":assumption (<= p_" << i << "_" << j << " 1)" << endl;
		}
	}

	// Each pidgeon goes to at least one hole
	// sum_j p_ij >= 1
	for (unsigned i = 0; i < n+1; ++ i) {
		cout << ":assumption (>= (+ ";
		for (unsigned j = 0; j < n; ++ j) {\
			cout << "p_" << i << "_" << j << " ";
		}
		cout << ") 1)" << endl;
	}

	// Each hole has at most one pidgeon
	// sum_i p_ij <= 1
	for (unsigned j = 0; j < n; ++ j) {
		cout << ":assumption (<= (+ ";
		for (unsigned i = 0; i < n+1; ++ i) {
			cout << "p_" << i << "_" << j << " ";
		}
		cout << ") 1)" << endl;
	}

	// Finish the benchmark
	cout << ":formula true" << endl << ")" << endl;
}

void mps(int n, const char* filename) {
}

int main(int argc, char* argv[]) {

	const int n = atoi(argv[1]);
	const int smt_format = atoi(argv[2]);

	if (smt_format) {
		smt(n);
	} else {
		opb(n);
	}
}

