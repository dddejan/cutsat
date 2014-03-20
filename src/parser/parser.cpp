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

#include <cstring>
#include <sstream>
#include "parser/parser.h"
#include "parser/ilp_parser.h"
#include "parser/mps_parser.h"
#include "parser/pbo_parser.h"
#include "parser/smt_parser.h"
#include "parser/cnf_parser.h"

using namespace std;
using namespace cutsat;

ParserException::ParserException(int lineNumber, string message) {
    stringstream ss;
    ss << "Passe error at line " << lineNumber << ": " << message << ".";
    d_message = ss.str();
}

InputFormat Parser::getFormatFromFilename(const char* filename) {
    // Find the extension
    const char* ext = strrchr(filename, '.');
    if (ext) {
        ext ++;
        if (strcmp(ext, "ilp") == 0) return ILP;
        if (strcmp(ext, "mps") == 0) return MPS;
        if (strcmp(ext, "opb") == 0) return OPB;
        if (strcmp(ext, "smt") == 0) return SMT;
        if (strcmp(ext, "cnf") == 0) return CNF;
    }
    // Return the default input format
    return s_defaultFormat;
}

ParserRef Parser::newParser(InputFormat format, Solver& solver) {
    ParserRef parser;
    switch (format) {
    case ILP: parser = ParserRef(new IlpParser(solver)); break;
    case MPS: parser = ParserRef(new MpsParser(solver)); break;
    case OPB: parser = ParserRef(new PboParser(solver)); break;
    case SMT: parser = ParserRef(new SmtParser(solver)); break;
    case CNF: parser = ParserRef(new CnfParser(solver)); break;
    default:
        assert(false);
    }
    return parser;
}

ParserRef Parser::newParser(const char* filename, Solver& solver) {
    InputFormat format = getFormatFromFilename(filename);
    ParserRef parser = newParser(format, solver);
    parser->setFilename(filename);
    return parser;
}

void Parser::addClauseConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables) {
    std::vector<ClauseConstraintLiteral> literals;
    for (unsigned i = 0; i < variables.size(); ++ i) {
        literals.push_back(ClauseConstraintLiteral(variables[i], coefficients[i] < 0));
    }
    d_solver.assertClauseConstraint(literals);
}

void Parser::addCardinalityConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables, unsigned c) {
    std::vector<CardinalityConstraintLiteral> literals;
    for (unsigned i = 0; i < variables.size(); ++ i) {
        literals.push_back(CardinalityConstraintLiteral(variables[i], coefficients[i] < 0));
    }
    d_solver.assertCardinalityConstraint(literals, c);
}

void Parser::addIntegerConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables, Integer& rhs) {
    std::vector<IntegerConstraintLiteral> literals;
    for (unsigned i = 0; i < variables.size(); ++ i) {
        literals.push_back(IntegerConstraintLiteral(coefficients[i], variables[i]));
    }
    d_solver.assertIntegerConstraint(literals, rhs);
}

void Parser::addIntegerConstraint(const std::map<Variable, Integer>& coefficients, Integer& rhs) {
    std::vector<IntegerConstraintLiteral> literals;
    std::map<Variable, Integer>::const_iterator it = coefficients.begin();
    std::map<Variable, Integer>::const_iterator it_end = coefficients.end();

    bool isBoolean = true;
    bool isCardinality = true;
    int negativeCoefficients = 0;
    Integer maxCoefficient = 0;
    for (; it != it_end; ++ it) {
        if (it->second != 0) {
        	literals.push_back(IntegerConstraintLiteral(it->second, it->first));
        }
        if (!d_solver.hasLowerBound(it->first) ||
        	!d_solver.hasUpperBound(it->first) ||
        	d_solver.getLowerBound<TypeInteger>(it->first) < 0 ||
        	d_solver.getUpperBound<TypeInteger>(it->first) > 1) {
        	isBoolean = false;
        	isCardinality = false;
        }
        if (isBoolean && (it->second > 1 || it->second < -1)) {
        	isCardinality = false;
        }
        if (isBoolean && isCardinality) {
        	if (NumberUtils<Integer>::abs(it->second) > maxCoefficient) {
        		maxCoefficient = NumberUtils<Integer>::abs(it->second);
        	}
        	if (it->second < 0) {
        		negativeCoefficients ++;
        	}
        }
    }

    // Check if it's a clause or a cardinality
    if (isCardinality) {
        if (rhs == 1 - negativeCoefficients) {
            // Clause
            std::vector<ClauseConstraintLiteral> clauseLiterals;
            for(unsigned i = 0; i < literals.size(); ++ i) {
                clauseLiterals.push_back(ClauseConstraintLiteral(literals[i].getVariable(), literals[i].isNegated()));
            }
            d_solver.assertClauseConstraint(clauseLiterals);
            return;
        } else {
        	// Clause
        	std::vector<CardinalityConstraintLiteral> cardinalityLiterals;
        	for(unsigned i = 0; i < literals.size(); ++ i) {
        		cardinalityLiterals.push_back(CardinalityConstraintLiteral(literals[i].getVariable(), literals[i].isNegated()));
        	}
        	// x - y >= 1 is equivalent to x + (1-y) >= 2
        	unsigned c = NumberUtils<Integer>::toInt(rhs) + negativeCoefficients;
        	d_solver.assertCardinalityConstraint(cardinalityLiterals, c);
        	return;
        }
    }

    d_solver.assertIntegerConstraint(literals, rhs);
}
