/**
 * Copyright 2010 Dejan Jovanovic.
 *
 * This file is part of cutsat.
 *
 * Cutsat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cutsat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cutsat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "util/exception.h"
#include "parser/cnf_parser.h"

using namespace std;
using namespace cutsat;

void CnfParser::makeVariables(size_t nVars)
{
    if (nVars > d_variables.size()) {
        std::vector<IntegerConstraintLiteral> literals;
        Integer one = -1;
        Integer zero = 0;
        literals.resize(1);
        char name[1000];
        for (unsigned i = d_variables.size(); i < nVars; ++ i) {
            sprintf(name, "x%d", i);
            Variable var = d_solver.newVariable(TypeInteger, name);
            d_variables.push_back(var);
            literals[0] = IntegerConstraintLiteral(1, var);
            d_solver.assertIntegerConstraint(literals, zero);
            literals[0] = IntegerConstraintLiteral(-1, var);
            d_solver.assertIntegerConstraint(literals, one);
        }
    }
}

void CnfParser::parse() throw(ParserException) {

    int lineNumber = 0;

    // Open the file
    FILE* input = fopen(d_filename.c_str(), "r");
    if (input == NULL) throw ParserException(lineNumber, "can't open " + d_filename);

    // Read lines of the file
    ssize_t read = 0;
    char* buffer = NULL;
    char* bufferP = NULL;
    size_t bufferSize = 0;
    while (!d_solver.inConflict() && (read = getline(&buffer, &bufferSize, input)) != -1)
    {
        // We are at the next line
        ++ lineNumber;

        // Clear the data
        d_constraintC.clear();
        d_constraintV.clear();

        // Skip the comments and empty lines
        if (buffer[0] == 'p' || buffer[0] == 'c' || buffer[0] == '\n') {
            continue;
        }

        // How many negative literals we haves
        int negativeLiterals = 0;

        // Read numbers until we hit 0
        bufferP  = buffer;
        while (true) {
            int literal = strtol(bufferP, &bufferP, 10);
            if (literal == 0) {
                break;
            }
            if (literal < 0) {
                negativeLiterals ++;
                d_constraintC.push_back(-1);
                literal = -literal;
            } else {
                d_constraintC.push_back(1);
            }
            if (literal >= (int)d_variables.size()) {
                makeVariables(literal + 1);
            }
            d_constraintV.push_back(d_variables[literal]);
        }

        // Add the constraint
        if (!d_constraintC.empty()) {
        	addClauseConstraint(d_constraintC, d_constraintV);
        }
    }

    // Free the buffer memory
    if (buffer != NULL) {
        free(buffer);
    }
}

