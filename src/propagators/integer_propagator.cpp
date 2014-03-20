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

#include "propagators/integer_propagator.h"

using namespace cutsat;

TraceTag integerPropagator("propagator::integer");

PreprocessStatus IntegerConstraintPropagator::preprocess(std::vector<IntegerConstraintLiteral>& literals, Integer& constant, int zeroLevelIndex) {

    CUTSAT_TRACE_FN("propagator::integer") << "preprocessing: " << literals << " >= " << constant << std::endl;

    // We only check for substitutions and obvious tautologies and compute the gcd
    unsigned i = 0, j = 0, i_end = literals.size();
    Integer gcd = 0;
    while (i < i_end) {
    	IntegerConstraintLiteral& literal = literals[i];
    	Variable literalVariable = literal.getVariable();
    	if (zeroLevelIndex >= 0 && d_solverState.isAssigned(literalVariable, zeroLevelIndex)) {
    		constant -= d_solverState.getValue(literal, zeroLevelIndex);
    		i ++;
    	} else {
    		// Compute the gcd
    		if (gcd > 0) {
    			gcd = NumberUtils<Integer>::gcd(gcd, NumberUtils<Integer>::abs(literal.getCoefficient()));
    		} else {
    			gcd = NumberUtils<Integer>::abs(literal.getCoefficient());
    		}
    		// Keep the literal
    		literals[j++] = literals[i++];
    	}
    }

    // Trim the literals
    literals.resize(j);

    // Divide by the gcd
    if (gcd > 1) {
    	for (i = 0, i_end = literals.size(); i < i_end; ++ i) {
    		Integer& coefficient = literals[i].getCoefficient();
    		coefficient = NumberUtils<Integer>::divideUp(coefficient, gcd);
    	}
    	constant = NumberUtils<Integer>::divideUp(constant, gcd);
    }

    CUTSAT_TRACE("propagators::integer") << "preprocessing: " << literals << " >= " << constant << std::endl;

    if (literals.size() == 0) {
    	if (constant > 0) {
    		return PREPROCESS_INCONSISTENT;
    	} else {
    		return PREPROCESS_TAUTOLOGY;
    	}
    }

	return PREPROCESS_OK;
}

void IntegerConstraintPropagator::attachConstraint(ConstraintRef constraintRef) {

    // Get the constraint
    IntegerConstraint& constraint = d_constraintManager.get<ConstraintTypeInteger>(constraintRef);
    CUTSAT_TRACE_FN("propagator::integer") << constraint << std::endl;

    // Attach to all the variables in the constraint and see if one can propagae something
    for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
    	// Get the variable
    	const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
    	Variable var = literal.getVariable();
    	// Attach to the watchlists
    	if (literal.getCoefficient() > 0) {
            // Attach to the any modification watch list
            d_watchManager.getWatchList(var, MODIFICATION_ANY).push_back<true>(constraintRef);
    	} else {
            // Attach to the any modification watch list
            d_watchManager.getWatchList(var, MODIFICATION_ANY).push_back<false>(constraintRef);
    	}
    }

    if (constraint.isLearnt()) {
    	// Propagate something
    	Integer sum = 0;
    	Integer propagatingVarCoefficient;
    	for (unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
    		const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
        	Variable var = literal.getVariable();
        	if (var == d_propagationVariable) {
        		propagatingVarCoefficient = literal.getCoefficient();
        		continue;
        	}
        	if (literal.getCoefficient() > 0) {
        		sum += d_solverState.getUpperBound<TypeInteger>(var) * literal.getCoefficient();
        	} else {
        		sum += d_solverState.getLowerBound<TypeInteger>(var) * literal.getCoefficient();
        	}

    	}
    	// We must be able to propagate something
    	if (propagatingVarCoefficient > 0) {
        	Integer bound = NumberUtils<Integer>::divideUp(constraint.getConstant() - sum, propagatingVarCoefficient);
			d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(d_propagationVariable, bound, constraintRef);
		} else {
	    	Integer bound = NumberUtils<Integer>::divideDown(constraint.getConstant() - sum, propagatingVarCoefficient);
			d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(d_propagationVariable, bound, constraintRef);
		}
    }
}


void IntegerConstraintPropagator::repropagate(ConstraintRef constraintRef) {

	const IntegerConstraint& constraint = d_constraintManager.get<ConstraintTypeInteger>(constraintRef);

	// Propagate something
	Integer sum = 0;
	Integer propagatingVarCoefficient;
	for (unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
		const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
    	Variable var = literal.getVariable();
    	if (var == d_propagationVariable) {
    		propagatingVarCoefficient = literal.getCoefficient();
    		continue;
    	}
    	if (literal.getCoefficient() > 0) {
    		if (d_solverState.hasUpperBound(var)) {
    			sum += d_solverState.getUpperBound<TypeInteger>(var) * literal.getCoefficient();
    		} else {
    			// Propagation not possible
    			return;
    		}
    	} else {
    		if (d_solverState.hasLowerBound(var)) {
    			sum += d_solverState.getLowerBound<TypeInteger>(var) * literal.getCoefficient();
    		} else {
    			// Propagation not possible
    			return;
    		}
    	}
	}

	// Try and propagate something
	if (propagatingVarCoefficient > 0) {
    	Integer bound = NumberUtils<Integer>::divideUp(constraint.getConstant() - sum, propagatingVarCoefficient);
		if (!d_solverState.hasLowerBound(d_propagationVariable) || d_solverState.getLowerBound<TypeInteger>(d_propagationVariable) < bound) {
			d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(d_propagationVariable, bound, constraintRef);
		}
	} else {
    	Integer bound = NumberUtils<Integer>::divideDown(constraint.getConstant() - sum, propagatingVarCoefficient);
		if (!d_solverState.hasUpperBound(d_propagationVariable) || d_solverState.getUpperBound<TypeInteger>(d_propagationVariable) > bound) {
			d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(d_propagationVariable, bound, constraintRef);
		}
	}
}

void IntegerConstraintPropagator::removeConstraint(ConstraintRef constraintRef) {

    const IntegerConstraint& constraint = d_constraintManager.get<ConstraintTypeInteger>(constraintRef);
    assert(!constraint.inUse());
    for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
        const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
        Variable variable = literal.getVariable();
        // Remove from the appears list
        d_watchManager.needsCleanup<MODIFICATION_ANY>(variable);
    }
}

void IntegerConstraintPropagator::bound(Variable var) {

	CUTSAT_TRACE_FN("propagator::integer") << var << std::endl;

	WatchList& constraintList = d_watchManager.getWatchList(var, MODIFICATION_ANY);

    bool lowerBoundSet = false;
    bool UpperBoundSet = false;
    Integer bestLowerBound;
	Integer bestUpperBound;
	ConstraintRef bestLowerBoundConstraint = ConstraintManager::NullConstraint;
	ConstraintRef bestUpperBoundConstraint = ConstraintManager::NullConstraint;

    WatchList::iterator it = constraintList.begin(), it_copy = it;
    WatchList::iterator it_end = constraintList.end();
    for(; it != it_end; ++ it) {

        ConstraintRef constraintRef = *it;
		IntegerConstraint& constraint = d_constraintManager.get<ConstraintTypeInteger>(constraintRef);

        if (constraint.isDeleted()) continue;
        *it_copy++ = constraintRef;

		CUTSAT_TRACE_FN("propagator::integer") << constraint << std::endl;

		// ax >= c - S, where S is the biggest estimate
		// Compute the upper bound of the sum of other variables
		Integer sum = 0;
		bool doBounding = true;
		int varIndex = -1;
		Integer varCoefficient = 0;
		for(unsigned lit = 0, i_end = constraint.getSize(); lit < i_end; ++ lit) {
            const IntegerConstraintLiteral& literal = constraint.getLiteral(lit);
			Variable literalVariable = literal.getVariable();
			const Integer& coefficient = literal.getCoefficient();
			if (literalVariable != var) {
				if (coefficient > 0) {
					// ax >= c - by, where b > 0 => we need to get upper bound
					if (d_solverState.hasUpperBound(literalVariable)) {
						sum += coefficient * d_solverState.getUpperBound<TypeInteger>(literalVariable);
					} else {
						constraint.swapLiterals(0, lit);
						doBounding = false;
                        break;
                    }
				} else {
					// ax >= c + by, where b > 0 =? we need to get lower boud
					if (d_solverState.hasLowerBound(literalVariable)) {
						sum += coefficient * d_solverState.getLowerBound<TypeInteger>(literalVariable);
					} else {
						constraint.swapLiterals(0, lit);
						doBounding = false;
                        break;
					}
				}
			} else {
				varIndex = lit;
				varCoefficient = literal.getCoefficient();
			}
		}
		assert(!doBounding || varIndex >= 0);

        if (doBounding) {
			// We have (1) ax >= c - sum or (2) -ax >= c - sum
			// (1) x >= ceil((c - sum) / a)
			// (2) x <= floor((c - sum) / -a)
        	if (varCoefficient > 0) {
				Integer bound = NumberUtils<Integer>::divideUp(constraint.getConstant() - sum, varCoefficient);
				if (!lowerBoundSet || bound > bestLowerBound) {
                    lowerBoundSet = true;
					bestLowerBound = bound;
					bestLowerBoundConstraint = constraintRef;
				}
			} else {
				Integer bound = NumberUtils<Integer>::divideDown(constraint.getConstant() - sum, varCoefficient);
				if (!UpperBoundSet || bound < bestUpperBound) {
                    UpperBoundSet = true;
					bestUpperBound = bound;
					bestUpperBoundConstraint = constraintRef;
				}
			}
		}
	}

    // Shrink the watch-list
    constraintList.resize(it_copy);


	if (lowerBoundSet) {
		if (!d_solverState.hasLowerBound(var) || bestLowerBound > d_solverState.getLowerBound<TypeInteger>(var)) {
			CUTSAT_TRACE("propagator::integer") << var << " >= " << bestLowerBound << std::endl;
			d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(var, bestLowerBound, bestLowerBoundConstraint);
		}
	}
	if (UpperBoundSet) {
		if (!d_solverState.hasUpperBound(var) || bestUpperBound < d_solverState.getUpperBound<TypeInteger>(var)) {
			CUTSAT_TRACE("propagator::integer") << var << " <= " << bestUpperBound << std::endl;
			d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(var, bestUpperBound, bestUpperBoundConstraint);
		}
	}
}

