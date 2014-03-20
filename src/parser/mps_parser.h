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

#include <vector>

#include "parser/parser.h"

namespace cutsat {

class MpsParser : public Parser {

    enum BoundType {
        LowerBound,
        UpperBound
    };

    void addConstraint(BoundType type, std::vector<Variable>& vars, std::vector<Integer> coeffNumerators,
            std::vector<Integer> coeffDenominators, Integer& cNumerator, Integer& cDenominator);

    void addIntegerBound(BoundType type, Variable var, Integer value);

public:

    MpsParser(Solver& solver)
    : Parser(solver) {}

    void parse() throw(ParserException);

};

}
