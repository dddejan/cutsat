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
#include <vector>

#include "util/exception.h"
#include "parser/pbo_parser.h"

using namespace std;
using namespace cutsat;

void PboParser::makeVariables(size_t nVars) {
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

void PboParser::parse() throw(ParserException) {

    int lineNumber = 0;

    // Open the file
    FILE* input = fopen(d_filename.c_str(), "r");
    if (input == NULL) throw ParserException(lineNumber, "can't open " + d_filename);

    // Read lines of the file
    ssize_t read = 0;
    char* buffer = NULL;
    char* bufferP = NULL;
    char* bufferP_end = NULL;
    const char* error = NULL;
    size_t bufferSize = 0;
    while ((read = getline(&buffer, &bufferSize, input)) != -1) {
        // We are at the next line
        ++ lineNumber;

        // Clear the data
        d_constraintC.clear();
        d_constraintV.clear();

        // Skip the comments
        if (buffer[0] == '*') continue;
        // Skip the empty lines
        if (buffer[0] == '\n') continue;
        // Is it the objective function
        if (buffer[0] == 'm') {
            // Skip the 'min:'
            bufferP = buffer + 4;
            // Skip the space
            while(*bufferP && *bufferP == ' ') ++ bufferP;
            // Read terms until hitting ';'
            while (bufferP[0] != ';')
            {
                // Find the end of the number (find the next space)
                bufferP_end = bufferP;
                while (*bufferP_end && *bufferP_end != ' ') ++ bufferP_end;
                *bufferP_end = 0;

                // Read the number
                d_constraintC.push_back(NumberUtils<Integer>::read(bufferP, &error));
                if (*error != 0) throw ParserException(lineNumber, "expected a number");

                // Skip the space
                bufferP = bufferP_end + 1;
                while (*bufferP_end && *bufferP_end == ' ') ++ bufferP_end;

                // Parse the variable
                if (*(bufferP ++) != 'x') throw ParserException(lineNumber, "expected a variable");
                int var_id = NumberUtils<int>::read(bufferP, &error);
                if (var_id <= 0) throw ParserException(lineNumber, "expected a variable");
                bufferP = const_cast<char*>(error);

                // Get the variable
                makeVariables(var_id + 1);
                d_constraintV.push_back(d_variables[var_id]);

                // Skip the space
                while(*bufferP && *bufferP == ' ') ++ bufferP;
            }
        }
        // Otherwise it's a constraint
        else {
            bufferP = buffer;
            // Is this a cardinality constraint
            bool isCardinality = true;
            int negativeLiterals = 0;
            // Skip the space
            while(*bufferP && *bufferP == ' ') ++ bufferP;
            // Read terms until hitting '>=' or '='
            while (*bufferP != '>' && *bufferP != '=')
            {
                // Find the end of the number (find the next space)
                bufferP_end = bufferP;
                while (*bufferP_end && *bufferP_end != ' ') ++ bufferP_end;
                *bufferP_end = 0;

                // Read the number
                Integer coefficient = NumberUtils<Integer>::read(bufferP, &error);
                if (*error != 0) throw ParserException(lineNumber, "expected a number");
                d_constraintC.push_back(coefficient);

                // Add up the negative coefficients
                if (coefficient < 0) {
                    negativeLiterals ++;
                }

                // Check if this is still a cardinality constraint
                if (isCardinality) {
                    if (NumberUtils<Integer>::abs(coefficient) != 1) {
                        isCardinality = false;
                    }
                }

                // Skip the space
                bufferP = bufferP_end + 1;
                while (*bufferP_end && *bufferP_end == ' ') ++ bufferP_end;

                // Parse the variable
                if (*(bufferP ++) != 'x') throw ParserException(lineNumber, "expected a variable");
                int var_id = NumberUtils<int>::read(bufferP, &error);
                if (var_id <= 0) throw ParserException(lineNumber, "expected a variable");
                bufferP = const_cast<char*>(error);

                // Get the variable
                makeVariables(var_id + 1);
                d_constraintV.push_back(d_variables[var_id]);

                // Skip the space
                while(*bufferP && *bufferP == ' ') ++ bufferP;
            }

            // Parse the relation symbol
            switch(*(bufferP++)) {
            case '>':
                if (*(bufferP++) != '=') throw ParserException(lineNumber, "expected a relation symbol");
                break;
            default:
                throw ParserException(lineNumber, "expected a relation symbol");
            }

            // Skip space
            while(*bufferP && *bufferP == ' ') ++ bufferP;

            // Find the end of the number (find the next space)
            bufferP_end = bufferP;
            while (*bufferP_end && *bufferP_end != ' ') ++ bufferP_end;
            *bufferP_end = 0;

            // Read the constant
            d_constraintRHS = NumberUtils<Integer>::read(bufferP, &error);
            if (*error != 0) throw ParserException(lineNumber, "expected a number");

            // Skip space
            bufferP = bufferP_end + 1;
            while(*bufferP && *bufferP == ' ') ++ bufferP;

            // We must end with ';'
            if (*(bufferP++) != ';') throw ParserException(lineNumber, "expected end of constraint (;)");

            // If a clause add it specially
            if (isCardinality) {
            	if (1 - negativeLiterals == d_constraintRHS) {
            		addClauseConstraint(d_constraintC, d_constraintV);
                	continue;
            	} else {
            		addCardinalityConstraint(d_constraintC, d_constraintV, NumberUtils<Integer>::toInt(d_constraintRHS) + negativeLiterals);
            	}
            } else {
            	// Otherwise just add the integer constraint
            	addIntegerConstraint(d_constraintC, d_constraintV, d_constraintRHS);
            }
        }
    }

    // Free the buffer memory
    if (buffer != NULL) {
        free(buffer);
    }
}

