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

#include <string>
#include <vector>

#include "parser/parser.h"

namespace cutsat {

class IlpParser : public Parser {

    /** The variables we are using */
    std::vector<Variable> d_variables;

    /**
     * Create integer variables x0, ... xn-1.
     * @param nVars the number of variables to create.
     */
    void makeVariables(size_t nVars);

    /** Constraint coefficients (in order) */
    std::vector<Integer>  d_constraintC;
    /** Constraint variables (in order) */
    std::vector<Variable> d_constraintV;
    /** Constraint right-hand-side */
    Integer d_constraintRHS;

public:

    IlpParser(Solver& solver)
    : Parser(solver) {}

    void parse() throw(ParserException);
};

}

