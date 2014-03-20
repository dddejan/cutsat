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

#include "propagators/cardinality_propagator.h"

using namespace cutsat;

TraceTag cardinalityPropagator("propagator::cardinality");

// Cardinality propagator doesn't care which bound got propagated, it should be on the right watch-list

void CardinalityConstraintPropagator::repropagate(ConstraintRef constraintRef) {
    CardinalityConstraint& constraint = d_constraintManager.get<ConstraintTypeCardinality>(constraintRef);

    CUTSAT_TRACE("propagator::cardinality") << d_propagationVariable << "," <<  constraint << std::endl;

    unsigned c = constraint.getConstant();
    for (unsigned i = c, i_end = constraint.getSize(); i < i_end; ++ i) {
        const CardinalityConstraintLiteral& literal = constraint.getLiteral(i);
        Variable litVariable = literal.getVariable();
        if (!d_solverState.isAssigned(litVariable) || d_solverState.getCurrentValue<ConstraintTypeCardinality>(literal) == 1) {
        	// We are not propagating
        	return;
        }
    }

    // We are propagating
	for (int i = c - 1; i >= 0 && !d_solverState.inConflict(); -- i) {
		const CardinalityConstraintLiteral& propagationLiteral = constraint.getLiteral(i);
		Variable propagationVariable = propagationLiteral.getVariable();
        if (propagationLiteral.isNegated()) {
        	if (d_solverState.getUpperBound<TypeInteger>(propagationVariable) == 1) {
        		d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(propagationVariable, 0, constraintRef);
        	}
        } else {
        	if (d_solverState.getLowerBound<TypeInteger>(propagationVariable) == 0) {
        		d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(propagationVariable, 1, constraintRef);
        	}
        }
	}
}

bool CardinalityConstraintPropagator::propagate(Variable var, ConstraintRef constraintRef, VariableModificationType eventType) {

    assert(d_solverState.isAssigned(var));
    assert(eventType == MODIFICATION_LOWER_BOUND_REFINE || eventType == MODIFICATION_UPPER_BOUND_REFINE);

    // Get the constraint
    CardinalityConstraint& constraint = d_constraintManager.get<ConstraintTypeCardinality>(constraintRef);

    CUTSAT_TRACE("propagator::cardinality") << var << "," <<  constraint << std::endl;

    // The number of literals we are watching
    unsigned c = constraint.getConstant();

    // Make sure that the propagating variable is at position c-1 and count the true literals
    for (unsigned i = 0, i_end = c-1; i <= i_end; ++ i) {
        CardinalityConstraintLiteral& currentLiteral = constraint.getLiteral(i);
        if (currentLiteral.getVariable() == var) {
            constraint.swapLiterals(i, c);
            break;
        }
    }
    assert(constraint.getLiteral(c).getVariable() == var);

    // The literal we are watching
    CardinalityConstraintLiteral& literal = constraint.getLiteral(c);

    // Try to find a new watch
    unsigned newWatch = 0;
    for (unsigned i = c + 1, i_end = constraint.getSize(); i < i_end; ++ i) {
        const CardinalityConstraintLiteral& literal = constraint.getLiteral(i);
        Variable litVariable = literal.getVariable();
        if (!d_solverState.isAssigned(litVariable) || d_solverState.getCurrentValue<ConstraintTypeCardinality>(literal) == 1) {
            newWatch = i;
            break;
        }
    }

    // If no watch has been found, we might propagate the unassigned literals to be true
    if (newWatch == 0) {
        for (unsigned i = 0; i < c; i ++) {
            CardinalityConstraintLiteral& propagationLiteral = constraint.getLiteral(i);
            Variable propagationVariable = propagationLiteral.getVariable();
            if (propagationLiteral.isNegated()) {
            	if (d_solverState.getUpperBound<TypeInteger>(propagationVariable) == 1) {
            		d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(propagationVariable, 0, constraintRef);
            	}
            } else {
            	if (d_solverState.getLowerBound<TypeInteger>(propagationVariable) == 0) {
            		d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(propagationVariable, 1, constraintRef);
            	}
            }
        }
    }
    // We found a new watch
    else {
        // Put the new watch on the spot
        constraint.swapLiterals(c, newWatch);
        // Add the watch to the watch list
        if (literal.isNegated()) {
            d_watchManager.getWatchList(literal.getVariable(), MODIFICATION_LOWER_BOUND_REFINE).push_back<false>(constraintRef);
        } else {
            d_watchManager.getWatchList(literal.getVariable(), MODIFICATION_UPPER_BOUND_REFINE).push_back<true>(constraintRef);
        }
        // Since we changed the watch we can erase this one
        return true;
    }

    // Default we just keep the watch
    return false;
}

// Sort the literals as:
// * unassigned first
// * true sorted descending trail index
// * false sorted descending trail index
struct cardinality_sort {
	const SolverState& d_state;
	cardinality_sort(const SolverState& state)
	: d_state(state) {}

	bool operator () (const CardinalityConstraintLiteral& l1, const CardinalityConstraintLiteral& l2) {
        // Whatever is unassigned goes first
		if (!d_state.isAssigned(l1.getVariable())) {
            return d_state.isAssigned(l2.getVariable()) || l1.getVariable() < l2.getVariable();
        }
		if (!d_state.isAssigned(l2.getVariable())) {
			return false;
		}

		// If they are of opposite signs, the true one goes first
		bool l1_true = d_state.getCurrentValue(l1) == 1;
		bool l2_true = d_state.getCurrentValue(l2) == 1;
		if (l1_true && !l2_true) {
			return true;
		}
		if (l2_true && !l1_true) {
			return false;
		}

		// Otherwise the higher index goes first
        return d_state.getLastModificationTrailIndex<true>(l1.getVariable()) > d_state.getLastModificationTrailIndex<true>(l2.getVariable());
    }
};

void CardinalityConstraintPropagator::attachConstraint(ConstraintRef constraintRef) {
    CUTSAT_TRACE_FN("propagator::cardinality");

    // Get the constraint
    CardinalityConstraint& constraint = d_constraintManager.get<ConstraintTypeCardinality>(constraintRef);
    CUTSAT_TRACE("propagator::cardinality") << "attaching: " << constraint << std::endl;

    // Sort the literals in order to attach
    cardinality_sort sorter(d_solverState);
    constraint.sort(sorter);

    CUTSAT_TRACE("propagator::cardinality") << "attaching: " << constraint << std::endl;

    // Attach the first (c + 1) literals -- if you hit a false literal it's propagation time
    bool propagate = false;
    for (unsigned i = 0, i_end = constraint.getConstant(); i <= i_end; ++ i) {
    	CardinalityConstraintLiteral& lit = constraint.getLiteral(i);
        if (lit.isNegated()) {
            // We want to know when l0 becomes false, i.e the variable becomes true, so we watch the lower bound
            d_watchManager.getWatchList(lit.getVariable(), MODIFICATION_LOWER_BOUND_REFINE).push_back<false>(constraintRef);
        } else {
            // We want to know when l0 becomes false, i.e the variable becomes false, so we watch the upper bound
            d_watchManager.getWatchList(lit.getVariable(), MODIFICATION_UPPER_BOUND_REFINE).push_back<true>(constraintRef);
        }
        if (d_solverState.isAssigned(lit.getVariable()) && d_solverState.getCurrentValue(lit) == 0) {
        	propagate = true;
        }
    }

    if (propagate) {
		CUTSAT_TRACE("propagator::cardinality") << "constraint propagates"<< std::endl;
    	for (int i = constraint.getConstant() - 1; i >= 0 && !d_solverState.inConflict(); -- i) {
    		const CardinalityConstraintLiteral& propagationLiteral = constraint.getLiteral(i);
    		Variable propagationVariable = propagationLiteral.getVariable();
            if (propagationLiteral.isNegated()) {
            	if (d_solverState.getUpperBound<TypeInteger>(propagationVariable) == 1) {
            		d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(propagationVariable, 0, constraintRef);
            	}
            } else {
            	if (d_solverState.getLowerBound<TypeInteger>(propagationVariable) == 0) {
            		d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(propagationVariable, 1, constraintRef);
            	}
            }
    	}
    }
}

PreprocessStatus CardinalityConstraintPropagator::preprocess(
		std::vector<CardinalityConstraintLiteral>& literals, constant_type& constant, int zeroLevelIndex) {

    CUTSAT_TRACE_FN("propagator::cardinality") << "preprocessing: " << literals << " >= " << constant << std::endl;

    assert(literals.size() > 0);

    // If the constant is 0, we are done
    if (constant == 0) {
        return PREPROCESS_TAUTOLOGY;
    }

    // Sort the literals
    std::sort(literals.begin(), literals.end());

    CUTSAT_TRACE("propagator::cardinality") << "preprocessing: " << literals << " >= " << constant << std::endl;

    // Go through the literals, eliminate duplicates and check for obvious inconsistencies
    int i = -1; // Last literal we are done with
    int j = 0; // The literal we are currently considering
    int j_end = literals.size();

    // Number of true literals
    unsigned trueLiteralCount = 0;

    do {
        // Check for semantic stuff
        if (d_solverState.isAssigned(literals[j].getVariable(), zeroLevelIndex)) {
            // If the literal is true, we are satisfied
            if (d_solverState.getValue<ConstraintTypeCardinality>(literals[j], zeroLevelIndex)) {
                // It the number of true literals is enough, this constraint is satisfied
                if (++ trueLiteralCount >= constant) {
                    return PREPROCESS_TAUTOLOGY;
                }
            }
            j ++;
            continue;
        }

        // Check for the syntactic stuff
        if (i >= 0 && literals[j].getVariable() == literals[i].getVariable()) {
            // Literal on the same variable, we don't allow this in the cardinality case
            assert(false);
        } else {
            // Not on the same variable, just copy
            literals[++ i] = literals[j ++];
        }
    } while (j < j_end);

    if (i >= 0) {
        // Trim the clause
        literals.resize(i + 1);

        // Update the constant
        constant = constant - trueLiteralCount;

        // If there is less literals than the constant, we are inconsistent
        if (i + 1 < (int)constant) {
            CUTSAT_TRACE("propagator::cardinality") << "Inconsistent!" << std::endl;
            return PREPROCESS_INCONSISTENT;
        }

        // If the number of literals is equal to the constant they must all be true
        if (i + 1 == (int) constant) {
            CUTSAT_TRACE("propagator::cardinality") << "Propagating!" << std::endl;
        	for (int k = 0; k <= i; ++ k) {
        		if (literals[k].isNegated()) {
        			// 1-x >= 1 => x <= 0
        			if (d_solverState.getUpperBound<TypeInteger>(literals[k].getVariable()) == 1) {
        				d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(literals[k].getVariable(), 0, ConstraintManager::NullConstraint);
        			}
        		} else {
        			// x >= 1
        			if (d_solverState.getLowerBound<TypeInteger>(literals[k].getVariable()) == 0) {
        				d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(literals[k].getVariable(), 1, ConstraintManager::NullConstraint);
        			}
        		}
        	}
        	return PREPROCESS_TAUTOLOGY;
        }

        CUTSAT_TRACE("propagator::cardinality") << "preprocessing => " << literals << " >= " << constant << std::endl;

        // Everything fine now
        return PREPROCESS_OK;
    } else {
        // No literals, we are inconsistent
        return PREPROCESS_INCONSISTENT;
    }
}

void CardinalityConstraintPropagator::removeConstraint(ConstraintRef constraintRef) {
    const CardinalityConstraint& constraint = d_constraintManager.get<ConstraintTypeCardinality>(constraintRef);
    assert(!constraint.inUse());
    for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
        const CardinalityConstraintLiteral& literal = constraint.getLiteral(i);
        Variable variable = literal.getVariable();
        // Remove from the appears list
        if (literal.isNegated()) {
        	d_watchManager.needsCleanup<MODIFICATION_LOWER_BOUND_REFINE>(variable);
        } else {
        	d_watchManager.needsCleanup<MODIFICATION_UPPER_BOUND_REFINE>(variable);
        }
    }
}

