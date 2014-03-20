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

#include "propagators/propagator.h"

namespace cutsat {

class IntegerConstraintPropagator : public Propagator<ConstraintTypeInteger> {
public:
	IntegerConstraintPropagator(ConstraintManager& constraintManager, SolverState& solverState)
	: Propagator<ConstraintTypeInteger>(constraintManager, solverState) { }
	PreprocessStatus preprocess(std::vector<literal_type>& literals, Integer& constant, int zeroLevelIndex);
    void attachConstraint(ConstraintRef constraintRef);
    void removeConstraint(ConstraintRef constraintRef);
    void repropagate(ConstraintRef constraintRef);
    void bound(Variable var);
};

}
