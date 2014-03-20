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

#include "propagators/clause_propagator.h"

#include <algorithm>

using namespace cutsat;

TraceTag clausePropagator("propagator::clause");

void ClauseConstraintPropagator::removeConstraint(ConstraintRef constraintRef) {

    const ClauseConstraint& constraint = d_constraintManager.get<ConstraintTypeClause>(constraintRef);
    assert(!constraint.inUse());
    for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
        const ClauseConstraintLiteral& literal = constraint.getLiteral(i);
        Variable variable = literal.getVariable();
        // Remove from the appears list
        if (literal.isNegated()) {
        	d_watchManager.needsCleanup<MODIFICATION_LOWER_BOUND_REFINE>(variable);
        } else {
        	d_watchManager.needsCleanup<MODIFICATION_UPPER_BOUND_REFINE>(variable);
        }
    }
}

void ClauseConstraintPropagator::repropagate(ConstraintRef constraintRef) {
    // Get the constraint
    ClauseConstraint& clause = d_constraintManager.get<ConstraintTypeClause>(constraintRef);
    CUTSAT_TRACE_FN("propagator::clause") << d_propagationVariable << " with " << clause << std::endl;

    // Get the first literal (this one should be unchanged..., the propagation should not happen before this
    const ClauseConstraintLiteral& l0 = clause.getLiteral(0);
    if (d_propagationVariable != l0.getVariable() || d_solverState.isAssigned(d_propagationVariable)) {
    	return;
    }

    // Check that the other ones are false (it's not enough to check just the second one, as many
    // reassertions might have happened with learned unit constraints.
    for (unsigned i = 1; i < clause.getSize(); ++ i) {
    	const ClauseConstraintLiteral& literal = clause.getLiteral(i);
    	if (!d_solverState.isAssigned(literal.getVariable()) || d_solverState.getCurrentValue(literal) == 1) {
    		return;
    	}
    }

   	// Propagate
    if (l0.isNegated()) {
    	// Propagate upper bound, i.e. var <= 0
        d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(d_propagationVariable, 0, constraintRef);
    } else {
    	// Propagate lower bound, i.e. var >= 1
        d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(d_propagationVariable, 1, constraintRef);
    }
}


// Clause propagator doesn't care which bound got propagated, it should be on the right watch-list
bool ClauseConstraintPropagator::propagate(Variable var, ConstraintRef constraintRef, VariableModificationType eventType) {

    // Get the constraint
    ClauseConstraint& clause = d_constraintManager.get<ConstraintTypeClause>(constraintRef);
    CUTSAT_TRACE_FN("propagator::clause") << var << "[" << eventType << "]" << "," << clause << std::endl;

    // Get the literals (REFERENCES! hence always first and second
    ClauseConstraintLiteral& firstLiteral = clause.getLiteral(0);
    ClauseConstraintLiteral& secondLiteral = clause.getLiteral(1);

    // Make sure that the propagating variable is at position 1
    if (firstLiteral.getVariable() == var) {
        clause.swapLiterals(0, 1);
    } else {
        assert(secondLiteral.getVariable() == var);
    }

    CUTSAT_TRACE("propagator::clause") << var << "[" << eventType << "]" << "," << clause << std::endl;

    // If a clause and 0th watch is true, then clause is already satisfied.
    if (d_solverState.isAssigned(firstLiteral.getVariable()) && d_solverState.getCurrentValue<ConstraintTypeClause>(firstLiteral) == 1) {
        CUTSAT_TRACE("propagator::clause") << "First literal already assigned to true!" << std::endl;
        return false;
    }

    // Try to find a new watch
    unsigned newWatch = 0;
    for (unsigned i = 2, i_end = clause.getSize(); i < i_end; ++ i) {
        const ClauseConstraintLiteral& literal = clause.getLiteral(i);
        Variable litVariable = literal.getVariable();
        if (!d_solverState.isAssigned(litVariable)) {
            newWatch = i;
            break;
        } else if (d_solverState.getCurrentValue<ConstraintTypeClause>(literal) == 1) {
            // This is a clause and we are already satisfied
            CUTSAT_TRACE("propagator::clause") << "Clause already satisfied!" << std::endl;
            return false;
        }
    }

    // If no watch has been found
    if (newWatch == 0) {
        Variable propagationVariable = firstLiteral.getVariable();
        if (firstLiteral.isNegated()) {
            // Propagate upper bound, i.e. var <= 0
            d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(propagationVariable , 0, constraintRef);
        } else {
            // Propagate lower bound, i.e. var >= 1
            d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(propagationVariable , 1, constraintRef);
        }
    }
    // We found a new watch
    else {
        CUTSAT_TRACE("propagator::clause") << "Found a new watch at position " << newWatch << std::endl;
        // Put the new watch on the spot
        clause.swapLiterals(1, newWatch);
        // Add the watch to the watch list
        CUTSAT_TRACE("propagator::clause") << "attaching to literal " << secondLiteral << " for clause " << clause << std::endl;
        if (secondLiteral.isNegated()) {
            d_watchManager.getWatchList(secondLiteral.getVariable(), MODIFICATION_LOWER_BOUND_REFINE).push_back<false>(constraintRef);
        } else {
            d_watchManager.getWatchList(secondLiteral.getVariable(), MODIFICATION_UPPER_BOUND_REFINE).push_back<true>(constraintRef);
        }
        // Since we checnged the watch we can erase this one
        return true;
    }

    // Default we just keep the watch
    return false;
}

void ClauseConstraintPropagator::attachConstraint(ConstraintRef constraintRef) {
    CUTSAT_TRACE_FN("propagator::clause");

    // Get the constraint
    ClauseConstraint& clause = d_constraintManager.get<ConstraintTypeClause>(constraintRef);
    CUTSAT_TRACE("propagator::clause") << "attaching: " << clause << std::endl;

    // Find the unassigned literal to put in the first two spots
    unsigned j = 0;
    for(unsigned i = 0; i < clause.getSize() && j < 2; ++ i) {
    	if (!d_solverState.isAssigned(clause.getLiteral(i).getVariable())) {
    		clause.swapLiterals(i, j++);
    	}
    }

    // Attach the first literal
    ClauseConstraintLiteral& l0 = clause.getLiteral(0);
    if (l0.isNegated()) {
        // We want to know when l0 becomes false, i.e the variable becomes true, so we watch the lower bound
        d_watchManager.getWatchList(l0.getVariable(), MODIFICATION_LOWER_BOUND_REFINE).push_back<false>(constraintRef);
    } else {
        // We want to know when l0 becomes false, i.e the variable becomes false, so we watch the upper bound
        d_watchManager.getWatchList(l0.getVariable(), MODIFICATION_UPPER_BOUND_REFINE).push_back<true>(constraintRef);
    }

    // If there is only one unassigned literal, we propagate it
    assert(j > 0);
    ClauseConstraintLiteral& l1 = clause.getLiteral(1);
    if (j == 1) {
        CUTSAT_TRACE("propagator::clause") << "propagates at attachment: " << clause << std::endl;
        assert(l0.getVariable() == d_propagationVariable);
    	// Propagate
    	if (l0.isNegated()) {
    		// Propagate upper bound, i.e. var <= 0
    	    d_solverState.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(l0.getVariable(), 0, constraintRef);
    	} else {
    		// Propagate lower bound, i.e. var >= 1
    	    d_solverState.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(l0.getVariable(), 1, constraintRef);
    	}
    	// Now, put the second highest literal to the second place
    	for (unsigned i = 2; i < clause.getSize(); ++ i) {
    		const ClauseConstraintLiteral& current = clause.getLiteral(i);
    		if (d_solverState.getLastModificationTrailIndex<true>(l1.getVariable()) < d_solverState.getLastModificationTrailIndex<true>(current.getVariable())) {
    			clause.swapLiterals(1, i);
    		}
    	}
    }

    // Attach the second literal
    if (l1.isNegated()) {
        // We want to know when l1 becomes false, i.e the variable becomes true, so we watch the lower bound
        d_watchManager.getWatchList(l1.getVariable(), MODIFICATION_LOWER_BOUND_REFINE).push_back<false>(constraintRef);
    } else {
        // We want to know when l1 becomes false, i.e the variable becomes false, so we watch the upper bound
        d_watchManager.getWatchList(l1.getVariable(), MODIFICATION_UPPER_BOUND_REFINE).push_back<true>(constraintRef);
    }
}

PreprocessStatus ClauseConstraintPropagator::preprocess(std::vector<literal_type>& literals, constant_type& c, int zeroLevelIndex) {

    assert(literals.size() > 0);

    CUTSAT_TRACE("propagator::clause") << "preprocessing: " << literals << std::endl;

    // Sort the literals
    std::sort(literals.begin(), literals.end());

    CUTSAT_TRACE("propagator::clause") << "preprocessing: " << literals << std::endl;

    // Go through the literals, eliminate duplicates and check for obvious inconsistencies
    int i = -1; // Last literal we are done with
    int j = 0; // The literal we are currently considering
    int j_end = literals.size();

    do {
        // Check for semantic stuff
        if (d_solverState.isAssigned(literals[j].getVariable(), zeroLevelIndex)) {
            // If the literal is true, we are satisfied
            if (d_solverState.getValue<ConstraintTypeClause>(literals[j], zeroLevelIndex) > 0) {
                return PREPROCESS_TAUTOLOGY;
            } else {
                // Literal is false so we can just skip it
                if (literals[j].isNegated()) {
                	c ++;
                }
                j ++;
                continue;
            }
        }

        // Check for the syntactic stuff
        if (i >= 0 && literals[j].getVariable() == literals[i].getVariable()) {
            // Literal on the same variable, check
            if (literals[j].isNegated() == literals[i].isNegated()) {
                // Same literal, just skip
                if (literals[j].isNegated()) {
                	c ++;
                }
                j ++;
                continue;
            } else {
                // Negated literal, tautology
                return PREPROCESS_TAUTOLOGY;
            }
        } else {
            // Not on the same variable, just copy
            literals[++ i] = literals[j ++];
        }
    } while (j < j_end);

    if (i >= 0) {
        // Trim the clause
        literals.resize(i + 1);

        // Everything fine now
        return PREPROCESS_OK;
    } else {
        // No literals, we are inconsistent
        return PREPROCESS_INCONSISTENT;
    }
}

