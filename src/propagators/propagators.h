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

#include "propagators/clause_propagator.h"
#include "propagators/cardinality_propagator.h"
#include "propagators/integer_propagator.h"

#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/utility.hpp>


namespace cutsat {

/**
 * A collection of propagators
 */
class PropagatorCollection {

public:

    /** Info for doing repropagation */
    struct RepropagationInfo {
        /** Constraint that spawned the propagation */
        ConstraintRef constraint;
        /** Trail index at which this propagation happened */
        int trailIndex;
        /** Variable the got propagated */
        Variable var;
        /** Constructor */
        RepropagationInfo(ConstraintRef constraint = ConstraintManager::NullConstraint, int trailIndex = -1, Variable var = VariableNull)
        : constraint(constraint), trailIndex(trailIndex), var(var) {}
    };

private:

    /** Constraint that propagate something on assert (and might propagate something on backtrack) */
    std::vector<RepropagationInfo> d_repropagationList;

    /** Constraints that need to be checked for repropagation */
    std::vector<RepropagationInfo> d_toRepropagate;

    /** Constraint manager */
    ConstraintManager& d_constraintManager;

    /** Solver we're using */
    SolverState& d_solverState;

	/** All the propagators indexed by the constraint type */
	boost::fusion::vector<
		ClauseConstraintPropagator,
		CardinalityConstraintPropagator,
		IntegerConstraintPropagator
	> d_propagators;

public:

    PropagatorCollection(ConstraintManager& cm, SolverState& solverState)
    : d_constraintManager(cm),
      d_solverState(solverState),
      d_propagators(
    	ClauseConstraintPropagator(cm, solverState),
    	CardinalityConstraintPropagator(cm, solverState),
    	IntegerConstraintPropagator(cm, solverState)
      )
    {}

    struct add_variable {
    	/** Variable to add */
    	Variable var;
    	add_variable(Variable var) :var(var) {}
    	template<typename T>
    	void operator() (T& t) const {
    		t.addVariable(var);
    	}
    };

    /**
     * Adds a variable to all the propagators.
     * @param var
     */
    void addVariable(Variable var) {
    	boost::fusion::for_each(d_propagators, add_variable(var));
    }

    struct set_propagating_info {
        Variable var;
        set_propagating_info(Variable var) :var(var) {}
        template<typename T>
        void operator() (T& t) const {
            t.setPropagationVariable(var);
        }
    };

    void setPropagatingInfo(Variable var) {
        boost::fusion::for_each(d_propagators, set_propagating_info(var));
    }

    struct clean_all {
    	template<typename T>
    	void operator() (T& t) const {
    		t.cleanAll();
    	}
    };

    /**
     * Cleans all the removed constraints out of the database.
     */
    void cleanAll() {
    	boost::fusion::for_each(d_propagators, clean_all());
    	// Ne need to clean th repropagation list, as it is a reason for something and hence can't be deleted
    }

    struct realloc_all {
    	const std::map<ConstraintRef, ConstraintRef>& reallocMap;
    	realloc_all(const std::map<ConstraintRef, ConstraintRef>& reallocMap)
    	: reallocMap(reallocMap) {}
    	template<typename T>
    	void operator() (T& t) const {
    		t.gcUpdate(reallocMap);
    	}
    };

    /**
     * Updates all the constraints with their reallocated pointers.
     */
    void gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap);

    /**
     * Attaches a constraint to the appropriate propagator.
     * @param constraintRef the constraint to attach
     */
    template<ConstraintType constraintType>
    void attachConstraint(ConstraintRef constraintRef) {
        unsigned oldTrailSize = d_solverState.getTrailSize();
    	boost::fusion::at_c<constraintType>(d_propagators).attachConstraint(constraintRef);
        if (oldTrailSize < d_solverState.getTrailSize()) {
            d_repropagationList.push_back(RepropagationInfo(constraintRef, oldTrailSize, d_solverState.getTrail()[oldTrailSize].var));
        }
    }

    /**
     * Removes a constraint from the appropriate propagator.
     * @param constraintRef the constraint to remove
     */
    template<ConstraintType constraintType>
    void removeConstraint(ConstraintRef constraintRef) {
    	boost::fusion::at_c<constraintType>(d_propagators).removeConstraint(constraintRef);
    }

    struct bound_variable {
    	Variable var;
    	bound_variable(Variable var) : var(var) {}
        template<typename T>
        void operator() (T& t) const {
        	t.bound(var);
        }
    };

    /**
     * Bounds a variable wrt the current constraints.
     */
    void bound(Variable var) {
    	boost::fusion::for_each(d_propagators, bound_variable(var));
    }

    template<VariableModificationType eventType>
    struct propagate_event {
    	Variable triggerVar;
    	propagate_event(Variable triggerVar) : triggerVar(triggerVar) {}
    	template<typename T>
    	void operator() (T& t) const {
            CUTSAT_TRACE_FN("propagators");
    		t.propagateEvent(triggerVar, eventType);
    	}
    };

    /**
     * Calls the propagators on the specific event that happened on the triggerVar.
     */
    template<VariableModificationType eventType>
    void propagateEvent(Variable triggerVar) {
    	boost::fusion::for_each(d_propagators, propagate_event<eventType>(triggerVar));
    }

    /**
     * Repropagates the constraints that might still be propagating after a backtrack.
     */
    void repropagate();

    /**
     * Passes the constraint to the appropriate propagator for preprocessing.
     */
    template<ConstraintType constraintType>
    PreprocessStatus preprocess(std::vector< Literal<constraintType> >& lits, typename ConstraintTraits<constraintType>::constant_type& constant, int zeroLevelIndex) {
    	return boost::fusion::at_c<constraintType>(d_propagators).preprocess(lits, constant, zeroLevelIndex);
    }

    /**
     * Passes the constraint to the appropriate propagator for preprocessing.
     */
    template<ConstraintType constraintType>
    PreprocessStatus preprocess(std::vector< Literal<constraintType> >& lits, int zeroLevelIndex) {
    	return boost::fusion::at_c<constraintType>(d_propagators).preprocess(lits, zeroLevelIndex);
    }

    void cancelUntil(int trailIndex);

    /**
     * Print some information
     * @param out
     */
    void print(std::ostream& out) const;
};

inline std::ostream& operator << (std::ostream& out, const PropagatorCollection& p) {
	p.print(out);
	return out;
}

}



