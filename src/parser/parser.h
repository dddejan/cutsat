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

#pragma once

#include <boost/shared_ptr.hpp>

#include "util/config.h"
#include "solver/solver.h"

namespace cutsat {

/**
 * Input formats accepted by the parser.
 */
enum InputFormat { ILP, MPS, OPB, SMT, CNF };

class Parser;

typedef boost::shared_ptr<Parser> ParserRef;

class ParserException : public CutSatException {
public:
    ParserException(int lineNumber, std::string message);
};

class Parser {

protected:

    /** The solver we are parsing into */
    Solver& d_solver;

    /** The filename of the problems (or "-" for stdin) */
    std::string d_filename;

public:

    /** The default input format if not specified */
    static const InputFormat s_defaultFormat = ILP;

    Parser(Solver& solver)
    : d_solver(solver), d_filename("-") {}

    virtual ~Parser() {}

    /** Returns the solver we are parsing into */
    Solver& getSolver() {
        return d_solver;
    }

    /** Sets the filename of the solver */
    virtual void setFilename(const char* filename) {
        d_filename = filename;
    }

    /**
     * Parses the problem into the solver.
     */
    virtual void parse() throw(ParserException) = 0;

    /**
     * Returns the format given the filename.
     * @param filename the full name of the file
     * @return the input format associated with the file extension
     */
    static InputFormat getFormatFromFilename(const char* filename);

    /**
     * Creates a new parser that parses a given format.
     * @param format the format to parse
     * @param solver the solver to use
     * @return a new parser
     */
    static ParserRef newParser(InputFormat format, Solver& solver);

    /**
     * Creates a new parser that can parse the given filename.
     * @param filename the name of the file to parse
     * @param solver the solver to use
     * @return a new parser
     */
    static ParserRef newParser(const char* filename, Solver& solver);

    /**
     * Adds a parsed clause constraint to the solver.
     */
    void addClauseConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables);

    /**
     * Adds a parsed cardinality constraint to the solver.
     * @param coefficients the coefficients.
     * @param variables the variables
     * @param c the cardinality constant (l1 + ... ln >= c)
     */
    void addCardinalityConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables, unsigned c);

    /**
     * Adds a parsed integer constraint to the solver.
     */
    void addIntegerConstraint(const std::vector<Integer>& coefficients, const std::vector<Variable>& variables, Integer& rhs);

    /**
     * Adds a parserd integer dconstraint to the solver.
     */
    void addIntegerConstraint(const std::map<Variable, Integer>& coefficients, Integer& rhs);

};


}
