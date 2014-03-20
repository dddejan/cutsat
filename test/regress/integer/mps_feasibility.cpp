#include <coin/CoinMpsIO.hpp>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		abort();
	}
	const char* input = argv[1];
	const char* output = argv[2];

	CoinMpsIO parser;
	parser.setConvertObjective(true);
	parser.readMps(input, "mps");

	const char* colnames[parser.getNumCols()];
	for (unsigned i = 0; i < parser.getNumCols(); ++ i) {
		colnames[i] = parser.columnName(i);
	}
	const char* rownames[parser.getNumRows()];
	for (unsigned i = 0; i < parser.getNumRows(); ++ i) {
		rownames[i] = parser.rowName(i);
	}

	double* objective = (double*)calloc(parser.getNumCols(), sizeof(double));

    CoinMpsIO newModel;
    newModel.setMpsData(
    		*parser.getMatrixByRow(),
    		parser.getInfinity(),
    		parser.getColLower(),
    		parser.getColUpper(),
    		objective,
    		parser.integerColumns(),
    		parser.getRowLower(),
    		parser.getRowUpper(),
    		colnames,
    		rownames);
    newModel.setProblemName(parser.getProblemName());
    newModel.writeMps(output);
}
