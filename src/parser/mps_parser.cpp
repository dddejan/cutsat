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

#include "parser/mps_parser.h"

#include <vector>
#include <coin/CoinMpsIO.hpp>

using namespace std;
using namespace cutsat;
using namespace boost::numeric;

void MpsParser::parse() throw(ParserException) {

    // Map from coin variables to our variables
    vector<Variable> coinToVariable;

    // Parse the problem using the coin parser
    CoinMpsIO coinParser;
    coinParser.messageHandler()->setLogLevel(0);
    coinParser.readMps(d_filename.c_str(), "mps");

    // Infinity the coin parser is using
    double inf = coinParser.getInfinity();

    // Get all the variables
    unsigned nVariables = coinParser.getNumCols();
    coinToVariable.resize(nVariables);
    for(unsigned var_i = 0; var_i < nVariables; ++ var_i) {

        bool        isInt    = coinParser.isInteger    (var_i);
        const char* varName  = coinParser.columnName   (var_i);
        double      varLower = coinParser.getColLower()[var_i];
        double      varUpper = coinParser.getColUpper()[var_i];

        if (!isInt) {
            cerr << "Warning: variable " << varName << " is not integer" << endl;
        }

        bool hasLower = (varLower != -inf);
        bool hasUpper = (varUpper != +inf);

        // Figure out the type of the variable
        VariableType type = isInt ? TypeInteger : TypeRational;

        // Create a new variable
        Variable var = d_solver.newVariable(type, varName);
        coinToVariable[var_i] = var;

        // Add the bound constraints
        switch(type) {
        case TypeInteger:
            if (hasLower) addIntegerBound(LowerBound, var, NumberUtils<Integer>::ceil(varLower));
            if (hasUpper) addIntegerBound(UpperBound, var, NumberUtils<Integer>::floor(varUpper));
            break;
        default:
            assert(false);
        }
    }

    // Get all the constraints
    const CoinPackedMatrix* matrix = coinParser.getMatrixByRow();
    unsigned nRows = matrix->getNumRows();
    for (unsigned i = 0; i < nRows; ++ i) {
        vector<Integer> coeffNumerators;
        vector<Integer> coeffDenominators;
        vector<Variable> vars;

        // Get the row
        CoinShallowPackedVector row = matrix->getVector(i);
        const double* coeffs = row.getElements();
        const int* varIndices = row.getIndices();
        // Go through the row elements
        unsigned nElements = row.getNumElements();
        for (unsigned j = 0; j < nElements; ++ j) {
            // Get the coefficient
            Rational coeff = NumberUtils<Rational>::floor(coeffs[j]);
            coeffNumerators.push_back(NumberUtils<Rational>::getNumerator(coeff));
            coeffDenominators.push_back(NumberUtils<Rational>::getDenominator(coeff));
            // Get the variable
            Variable var = coinToVariable[varIndices[j]];
            vars.push_back(var);
        }

        // Add the constraints
        double rowLowerBound = coinParser.getRowLower()[i];
        if (rowLowerBound != -inf) {
            Rational bound = NumberUtils<Rational>::floor(rowLowerBound);
            Integer boundN = NumberUtils<Rational>::getNumerator(bound);
            Integer boundD = NumberUtils<Rational>::getDenominator(bound);
            addConstraint(LowerBound, vars, coeffNumerators, coeffDenominators, boundN, boundD);
        }
        double rowUpperBound = coinParser.getRowUpper()[i];
        if (rowUpperBound != inf) {
            Rational bound = NumberUtils<Rational>::floor(rowUpperBound);
            Integer boundN = NumberUtils<Rational>::getNumerator(bound);
            Integer boundD = NumberUtils<Rational>::getDenominator(bound);
            addConstraint(UpperBound, vars, coeffNumerators, coeffDenominators, boundN, boundD);
        }
    }
}

void MpsParser::addConstraint(BoundType type, std::vector<Variable>& vars, std::vector<Integer> coeffNumerators,
        std::vector<Integer> coeffDenominators, Integer& cNumerator, Integer& cDenominator) {

    bool hasRational = false; // Does the constraint have rational variables
    bool isCardinalityConstraint = true; // Are we dealing with a cardinality constraint
    int negativeCount = 0; // How many negative coefficients are there

    Integer denominatorLCM = cln::abs(cDenominator); // Least common multiple of the denominators

    // Compute the LCM of the coefficients
    unsigned nLiterals = vars.size();
    for (unsigned i = 0; i < nLiterals; ++ i) {
        // If there is a rational variable, this is a rational constraint
        if (vars[i].getType() == TypeRational) hasRational = true;
        // Accumulate the LCM of the denominators
        denominatorLCM = NumberUtils<Integer>::lcm(denominatorLCM, coeffDenominators[i]);
    }

    // If the bound is an upper bound, we convert it to the lower bound
    if (type == UpperBound) denominatorLCM = -denominatorLCM;

    // Normalize the constraint
    if (denominatorLCM != 1) {
        cNumerator *= denominatorLCM;
        cDenominator = 1;
        for (unsigned i = 0; i < nLiterals; ++ i) {
            Integer& coeff = coeffNumerators[i];
            coeff *= denominatorLCM;
            if (coeff < 0) {
                ++ negativeCount;
                if (coeff != -1) isCardinalityConstraint = false;
            }
            else if (coeff != 1) {
                isCardinalityConstraint = false;
            }
            coeffDenominators[i] = 1;
        }
    }

    assert(!hasRational);
    
    if (isCardinalityConstraint) {
    	if (cNumerator == 1 - negativeCount) {
    		// It's a clause
    		addClauseConstraint(coeffNumerators, vars);
    	}
    }

    addIntegerConstraint(coeffNumerators, vars, cNumerator);
}

void MpsParser::addIntegerBound(MpsParser::BoundType type, Variable var, Integer value)
{
    vector<IntegerConstraintLiteral> literals;
    switch (type) {
        case LowerBound: {
            Integer one = 1;
            literals.push_back(IntegerConstraintLiteral(one, var));
            d_solver.assertIntegerConstraint(literals, value);
            break;
        }
        case UpperBound: {
            Integer one = -1;
            value = -value;
            literals.push_back(IntegerConstraintLiteral(one, var));
            d_solver.assertIntegerConstraint(literals, value);
            break;
        }
        default:
            assert(false);
    }
}

