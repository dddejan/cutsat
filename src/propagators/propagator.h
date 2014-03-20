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

#include "constraints/constraint_manager.h"
#include "propagators/watch_list_manager.h"
#include "solver/solver_state.h"

#include <map>

namespace cutsat {

/**
 * Status of the preprocessing. Upcasting means for example upgrading a cardinality constraint
 * to a clause.
 */
enum PreprocessStatus {
    /** Preprocessing went fine, use the literals to assert the constraint */
    PREPROCESS_OK,
    /** Preprocessing determined that the constraint is unsatisfiable */
    PREPROCESS_INCONSISTENT,
    /** Preprocessing determined that the constraint is a tautology */
    PREPROCESS_TAUTOLOGY
};

/**
 * General propagator.
 */
template<ConstraintType type>
class Propagator {

protected:

    /** Constraint manager we are using */
    const ConstraintManager& d_constraintManager;

    /** Solver state we will communicate the propagations to */
    SolverState& d_solverState;

    /** The watch-list manager */
    WatchListManager d_watchManager;

    /** The possible propagtation variable for re-propagation */
    Variable d_propagationVariable;

public:

    typedef Literal<type> literal_type;
    typedef typename ConstraintTraits<type>::constant_type constant_type;

    Propagator(ConstraintManager& constraintManager, SolverState& solverState)
    : d_constraintManager(constraintManager), d_solverState(solverState), d_watchManager(constraintManager)
    {
    }

    /** Add a new variable to watch */
    inline void addVariable(Variable var) {
    	d_watchManager.addVariable(var);
    }

    inline void setPropagationVariable(Variable var) {
        d_propagationVariable = var;
    }

    inline void cleanAll() {
    	d_watchManager.cleanAll();
    }

    inline void gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap) {
        d_watchManager.gcUpdate(reallocMap);
    }

    void propagateEvent(Variable triggerVar, VariableModificationType eventType) {

    	// Get the watch list according to the event
        WatchList& watchList = d_watchManager.getWatchList(triggerVar, eventType);

        // Go through the watched constraints and call the appropriate propagator
        WatchList::iterator watch_i = watchList.begin();
        WatchList::iterator watch_j = watchList.begin();
        WatchList::iterator watch_i_end = watchList.end();
        while (watch_i != watch_i_end) {

            // The constraint that is watching this event
            ConstraintRef constraintRef = *watch_i;

            // Should we remove this watch
            bool removeWatch = propagate(triggerVar, constraintRef, eventType);

            // If we keep this watch go on
            if (!removeWatch) {
                *watch_j++ = *watch_i++;
            } else {
                watch_i ++;
            }

            // If in conflict
            if (d_solverState.inConflict()) {
                // Copy the remaining watches
                while (watch_i != watch_i_end) {
                    *watch_j++ = *watch_i++;
                }
                // Shrink the watchlist
                break;
            }
        }

        // Shrink the watch-list
        watchList.resize(watch_j);
    }

    // INTERFACE METHODS TO BE IMPLEMENTED IN SPECIALIZATIONS

    /**
     * Attaches a constraint to the appropriate watchlists.
     * @param constraintRef the constraint
     */
    virtual void attachConstraint(ConstraintRef constraintRef) = 0;

    /**
     * Attaches a constraint to the appropriate watchlists.
     * @param constraintRef the constraint
     */
    virtual void removeConstraint(ConstraintRef constraintRef) = 0;

    /**
     * Repropagates a constraint after a backtrack.
     * @param cref the constraint that propagated something at a higher level
     */
    virtual void repropagate(ConstraintRef cref) {
    	// Default do nothing
    }

    /**
     * Preprocess the constraint wrt the current assignment.
     * @param lits the literals
     * @param constant the constraint
     * @return the status of the preprocess
     */
    virtual PreprocessStatus preprocess(std::vector<literal_type>& lits, constant_type& constant, int zeroLevelIndex) = 0;

    /**
     * Bound the given variable.
     * @param var
     */
    virtual void bound(Variable var) {}

    /**
     * Propagate any value or bound changes to the solver state.
     *
     * @param var the variable that has been refined
     * @param constraintRef the constraint selected from the watch-list
     * @return true if this constraint is no longer useful in propagation and can be removed
     *         from the watch-list
     */
    virtual bool propagate(Variable var, ConstraintRef constraintRef, VariableModificationType eventType) {
    	return false;
    }
};

}
