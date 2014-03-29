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

#include <ext/pb_ds/priority_queue.hpp>
#include <string>

#include "solver/search_trail.h"
#include "solver/variable_info.h"
#include "constraints/constraint_manager.h"

#include "util/enums.h"

namespace cutsat {

/**
 * Class encapsulating the state of the solver. This includes
 * the variable values, bounds and the heuristic information.
 */
class SolverState {

private:

	struct heuristic_info {
		unsigned hasLowerBound : 1;
		unsigned hasUpperBound : 1;
		double value;
		heuristic_info()
		: hasLowerBound(0), hasUpperBound(0), value(0) {}
	};
    typedef std::vector<heuristic_info> variable_heuristic;

    /** The map from variables to their heuristic */
    variable_heuristic d_variableHeuristic;

    /** The phases of variables, true = lower, false = upper */
    std::vector<bool> d_variablePhase;

    /** How much to increase the variable score per bump */
    double d_variableHeuristicIncrease;
    /** Decay factor for the variable scores */
    double d_variableHeuristicDecay;

    /** Is the state in conflict */
    bool d_inConflict;

    /** If in conflict, which variable is responsible */
    Variable d_conflictVariable;

    /** Names of variables for nice printout */
    std::vector<std::string> d_variableNames;

    /** Constraint manager */
    const ConstraintManager& d_cm;

public:

    /**
     * Returns the number of variables.
     */
    unsigned getVariablesCount() const {
        return d_variableNames.size();
    }

    /**
     * Returns true if the state is in conflict.
     * @return true if in conflict
     */
    bool inConflict() const {
        return d_inConflict;
    }

    /**
     * Returns the last variable x that is (or was) conflict, i.e. lb(x) > ub(x).
     * @return the conflict variable
     */
    Variable getConflictVariable() const {
    	return d_conflictVariable;
    }

    /**
     * Flips the phase of the variable
     * @param var
     */
    void setPhase(Variable var, bool phase) {
    	d_variablePhase[var.getId()] = phase;
    }

    /**
     * Compares two variables based on their activity heuristics.
     */
    class variable_compare_by_activity {
        const variable_heuristic& d_heuristic;
    public:
        variable_compare_by_activity(variable_heuristic& heuristic)
        : d_heuristic(heuristic) {}

        bool operator () (const Variable& v1, const Variable& v2) const {
            const heuristic_info& v1_info = d_heuristic[v1.getId()];
            const heuristic_info& v2_info = d_heuristic[v2.getId()];

            return v1_info.value < v2_info.value;
        }
    };

    /**
     * Compares two variables based on their ids
     */
    class variable_compare_linear {
    public:
        bool operator () (const Variable& v1, const Variable& v2) const {
            return v1.getId() > v2.getId();
        }
    };

    struct BacktrackVisitor {
        SolverState& d_state;
        BacktrackVisitor(SolverState& state): d_state(state) {}
        inline void operator() (const TrailElement& trailElement);
        inline void init() {}
    };

private:

    typedef __gnu_pbds::priority_queue<Variable, variable_compare_by_activity> dynamic_priority_queue;
    typedef dynamic_priority_queue::point_iterator dynamic_queue_pointer;

    variable_compare_by_activity d_variableCompare;

    /** The priority queue for dynamic variable selection */
    dynamic_priority_queue d_variableQueueDynamic;

    typedef __gnu_pbds::priority_queue<Variable, variable_compare_linear> linear_priority_queue;

    /** The priority queue for linear variable selection */
    linear_priority_queue d_variableQueueLinear;

    /** The position of a variable in the queue */
    std::vector<dynamic_queue_pointer> d_variableQueuePosition;

    /** Is the variable in the queue */
    std::vector<bool> d_variableInQueue;

    /** All the static data of a variable */
    std::vector<VariableInfo> d_variableInfo;

    /** The bounds of the integer variables */
    std::vector<Integer> d_boundsInteger;

    /** The bounds of the rational variables */
    std::vector<Rational> d_boundsRational;

    template<VariableType type>
    inline typename VariableTraits<type>::value_type const& getBound(unsigned i) const;

    template<VariableType type>
    inline unsigned addBound(typename VariableTraits<type>::value_type const& boundValue);

    /** The search trail */
    SearchTrail d_trail;

    /** Trail visitor on backtracks */
    BacktrackVisitor d_backtrackVisitor;

    /** Whether to use dynamic ordering */
    bool d_dynamicOrder;

    /**
     * Resize the state so that it can accommodate the given number of variables.
     * @param size the size to ensure
     */
    void resize(size_t size);

    template<typename NumberType>
    struct reassert_info {
    	Variable variable;
    	VariableModificationType type;
    	NumberType value;
    	reassert_info(Variable variable, VariableModificationType type, const NumberType& value)
    	: variable(variable), type(type), value(value) {}
    };

    std::vector< reassert_info<Integer> > d_integerReassertList;
    std::vector< reassert_info<Rational> > d_rationalReassertList;

    template<VariableModificationType type>
    void addToUnitReassertList(Variable variable) {
    	switch (variable.getType()) {
    	case TypeInteger: {
    		Integer bound = type == MODIFICATION_LOWER_BOUND_REFINE ?
    				getLowerBound<TypeInteger>(variable) :
    				getUpperBound<TypeInteger>(variable) ;
    		d_integerReassertList.push_back(reassert_info<Integer>(variable, type, bound));
    		break;
    	}
    	case TypeRational: {
    		Rational bound = type == MODIFICATION_LOWER_BOUND_REFINE ?
    				getLowerBound<TypeRational>(variable) :
    				getUpperBound<TypeRational>(variable) ;
    		d_rationalReassertList.push_back(reassert_info<Rational>(variable, type, bound));
    		break;
    	}
    	default:
    		assert(false);
    	}
    }

public:

    SolverState(const ConstraintManager& cm)
    : d_variableHeuristicIncrease(1),
      d_variableHeuristicDecay(1.001),
      d_inConflict(false),
      d_conflictVariable(VariableNull),
      d_cm(cm),
      d_variableCompare(d_variableHeuristic),
      d_variableQueueDynamic(d_variableCompare),
      d_backtrackVisitor(*this),
      d_dynamicOrder(true)
      {}

    /**
     * Is the variable in the decision queue.
     */
    bool inQueue(Variable var) const {
        return d_variableInQueue[var.getId()];
    }


    /**
     * Returns the next unassigned variable in the variable order.
     * @return the next unassigned variable
     */
    Variable decideVariable();

    /**
     * Set to true to use dynamic order.
     * @param flag
     */
    void setDynamicOrder(bool flag) {
    	d_dynamicOrder = flag;
    }

    bool isDynamicOrderOn() const  {
    	return d_dynamicOrder;
    }

    /**
     * Returns the order of variables.
     * @param output the vector in order.
     */
    void getLinearOrder(std::vector<Variable>& output);

    /**
     * Returns the value to choose for the variable.
     * @param var the variable
     */
    void decideValue(Variable var);

    unsigned getTrailSize() const {
    	return d_trail.getSize();
    }

    /**
     * Adds a variable to the state.
     * @param var the variable to add
     */
    void newVariable(Variable var, const char* name, bool addToQueue = true);

    bool hasLowerBound(Variable var) const {
        return d_variableInfo[var.getId()].hasLowerBound();
    }

    bool hasLowerBound(Variable var, unsigned trailIndex) const {
        return d_variableInfo[var.getId()].hasLowerBound(trailIndex);
    }

    bool hasUpperBound(Variable var) const {
        return d_variableInfo[var.getId()].hasUpperBound();
    }

    bool hasUpperBound(Variable var, unsigned trailIndex) const {
        return d_variableInfo[var.getId()].hasUpperBound(trailIndex);
    }

    bool isDecided(Variable var) const {
        switch (d_variableInfo[var.getId()].getValueStatus()) {
            case ValueStatusAssignedToLower:
            case ValueStatusAssignedToUpper:
                return true;
            default:
                return false;
        }
    }

    bool isAssigned(Variable var) const {
        return d_variableInfo[var.getId()].getValueStatus() != ValueStatusUnassigned;
    }

    bool isAssigned(Variable var, unsigned trailIndex) const {
        return d_variableInfo[var.getId()].getValueStatus(trailIndex) != ValueStatusUnassigned;
    }

    int getAssignmentIndex(Variable var, unsigned trailIndex) const {
    	assert(isAssigned(var, trailIndex));
    	return d_variableInfo[var.getId()].getAssignmentIndex(trailIndex);
    }

    int getLowerBoundTrailIndex(Variable var) const {
    	assert(hasLowerBound(var));
    	return d_variableInfo[var.getId()].getLowerBoundTrailIndex();
    }

    int getLowerBoundTrailIndex(Variable var, unsigned trailIndex) const {
    	assert(hasLowerBound(var, trailIndex));
    	return d_variableInfo[var.getId()].getLowerBoundTrailIndex(trailIndex);
    }

    int getUpperBoundTrailIndex(Variable var) const {
    	assert(hasUpperBound(var));
    	return d_variableInfo[var.getId()].getUpperBoundTrailIndex();
    }

    int getUpperBoundTrailIndex(Variable var, unsigned trailIndex) const {
    	assert(hasUpperBound(var, trailIndex));
    	return d_variableInfo[var.getId()].getUpperBoundTrailIndex(trailIndex);
    }

    template<bool includeAssignment>
    int getLastModificationTrailIndex(Variable var, unsigned trailIndex) const {
    	return d_variableInfo[var.getId()].getLastModificationTrailIndex<includeAssignment>(trailIndex);
    }

    template<bool includeAssignment>
    int getLastModificationTrailIndex(Variable var) const {
    	return d_variableInfo[var.getId()].getLastModificationTrailIndex<includeAssignment>();
    }

    ConstraintRef getLowerBoundConstraint(Variable var) const {
        assert(hasLowerBound(var));
        return d_variableInfo[var.getId()].getLowerBoundConstraint();
    }

    ConstraintRef getLowerBoundConstraint(Variable var, unsigned trailIndex) const {
        assert(hasLowerBound(var));
        return d_variableInfo[var.getId()].getLowerBoundConstraint(trailIndex);
    }

    ConstraintRef getUpperBoundConstraint(Variable var) const {
        assert(hasUpperBound(var));
        return d_variableInfo[var.getId()].getUpperBoundConstraint();
    }

    ConstraintRef getUpperBoundConstraint(Variable var, unsigned trailIndex) const {
        assert(hasUpperBound(var));
        return d_variableInfo[var.getId()].getUpperBoundConstraint(trailIndex);
    }

    template<VariableType type>
    void setLowerBound(Variable var, const typename VariableTraits<type>::value_type& value, ConstraintRef reason, unsigned trailIndex) {
    	assert(!hasLowerBound(var, trailIndex) || (value > getLowerBound<type>(var, trailIndex) && (int) trailIndex > getLowerBoundTrailIndex(var)));
        VariableInfo& varInfo = d_variableInfo[var.getId()];
        varInfo.setLowerBoundInfo(addBound<type>(value), reason, trailIndex);
        assert(hasLowerBound(var));
    }

    template<VariableType type>
    void setUpperBound(Variable var, const typename VariableTraits<type>::value_type& value, ConstraintRef reason, unsigned trailIndex) {
    	assert(!hasUpperBound(var, trailIndex) || (value < getUpperBound<type>(var, trailIndex) && (int) trailIndex > getUpperBoundTrailIndex(var)));
        VariableInfo& varInfo = d_variableInfo[var.getId()];
        varInfo.setUpperBoundInfo(addBound<type>(value), reason, trailIndex);
        assert(hasUpperBound(var));
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getLowerBound(Variable var) const {
    	assert(hasLowerBound(var));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	return getBound<type>(variableInfo.getLowerBoundIndex());
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getLowerBound(Variable var, unsigned trailIndex) const {
    	assert(hasLowerBound(var, trailIndex));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	return getBound<type>(variableInfo.getLowerBoundIndex(trailIndex));
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getUpperBound(Variable var) const {
    	assert(hasUpperBound(var));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	return getBound<type>(variableInfo.getUpperBoundIndex());
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getUpperBound(Variable var, unsigned level) const {
    	assert(hasUpperBound(var, level));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	return getBound<type>(variableInfo.getUpperBoundIndex(level));
    }

    ValueStatus getCurrentValueStatus(Variable var) const {
    	return d_variableInfo[var.getId()].getValueStatus();
    }

    ValueStatus getValueStatus(Variable var, unsigned trailIndex) const {
    	return d_variableInfo[var.getId()].getValueStatus(trailIndex);
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getCurrentValue(Variable var) const {
    	assert(isAssigned(var));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	if (variableInfo.getValueStatus() == ValueStatusAssignedToLower) {
    		return getBound<type>(variableInfo.getLowerBoundIndex());
    	} else {
    		return getBound<type>(variableInfo.getUpperBoundIndex());
    	}
    }

    template<VariableType type>
    typename VariableTraits<type>::value_type const& getValue(Variable var, unsigned trailIndex) const {
    	assert(isAssigned(var, trailIndex));
    	const VariableInfo& variableInfo = d_variableInfo[var.getId()];
    	if (variableInfo.getValueStatus(trailIndex) == ValueStatusAssignedToLower) {
    		return getBound<type>(variableInfo.getLowerBoundIndex(trailIndex));
    	} else {
    		return getBound<type>(variableInfo.getUpperBoundIndex(trailIndex));
    	}
    }

    template<ConstraintType type>
    typename ConstraintTraits<type>::literal_value_type getCurrentValue(const Literal<type>& literal) const {
    	return literal.getValue(getCurrentValue< ConstraintTraits<type>::variableType >(literal.getVariable()));
    }

    template<ConstraintType type>
    typename ConstraintTraits<type>::literal_value_type getValue(const Literal<type>& literal, unsigned trailIndex) const {
    	return literal.getValue(getValue< ConstraintTraits<type>::variableType >(literal.getVariable(), trailIndex));
    }

    /**
     * Pushes the variable to the assignment queue and remove a variable from the assignment ordering. Since the
     * variables are enqueued in backward trail order, we will remove the same variables from the ordering.
     * @param var the variable
     */
    void enqueueVariable(Variable var) {
        assert(!inQueue(var));
        if (d_dynamicOrder) {
        	d_variableQueuePosition[var.getId()] = d_variableQueueDynamic.push(var);
        } else {
        	d_variableQueueLinear.push(var);
        }
        d_variableInQueue[var.getId()] = true;
    }

    template<VariableModificationType type, bool set>
    void changeVariableHeuristicBound(Variable var) {
    	if (d_dynamicOrder) {
            if (inQueue(var)) {
                d_variableQueueDynamic.erase(d_variableQueuePosition[var.getId()]);
    			if (type == MODIFICATION_LOWER_BOUND_REFINE) {
    				d_variableHeuristic[var.getId()].hasLowerBound = set ? 1 : 0;
    			} else {
    				d_variableHeuristic[var.getId()].hasUpperBound = set ? 1 : 0;
    			}
    			d_variableQueuePosition[var.getId()] = d_variableQueueDynamic.push(var);
    		} else {
    			if (type == MODIFICATION_LOWER_BOUND_REFINE) {
    				d_variableHeuristic[var.getId()].hasLowerBound = set ? 1 : 0;
    			} else {
    				d_variableHeuristic[var.getId()].hasUpperBound = set ? 1 : 0;
    			}
    		}
    	}
    }

    void bumpVariable(Variable var, double times = 1) {
    	if (d_dynamicOrder) {
    		double newValue = d_variableHeuristic[var.getId()].value + d_variableHeuristicIncrease*times;
			if (inQueue(var)) {
				d_variableQueueDynamic.erase(d_variableQueuePosition[var.getId()]);
				d_variableHeuristic[var.getId()].value = newValue;
				d_variableQueuePosition[var.getId()] = d_variableQueueDynamic.push(var);
			} else {
				d_variableHeuristic[var.getId()].value = newValue;
			}
    		if (newValue > 1e100) {
    			// This preserves the order, we're fine
    			for (unsigned i = 0, i_end = d_variableHeuristic.size(); i < i_end; ++ i) {
    				d_variableHeuristic[i].value *= 1e-100;
    			}
    			d_variableHeuristicIncrease *= 1e-100;
    		}
    	}
    }

    void decayActivities() {
        d_variableHeuristicIncrease *= d_variableHeuristicDecay;
    }

    VariableInfo& operator [] (Variable var) {
        return d_variableInfo[var.getId()];
    }

    void cancelUntil(int index) {
    	d_backtrackVisitor.init();
        d_trail.cancelUntil(index, d_backtrackVisitor);
        if (d_inConflict) {
            if (!inQueue(d_conflictVariable)) {
              enqueueVariable(d_conflictVariable);
            }
            d_inConflict = false;
        }
    }

    void reassertUnitBounds();

    /**
     * Get the last index at level 0.
     * @return
     */
    int getSafeIndex() const {
    	return d_trail.getTrailIndexOfLevel(0);
    }

    /**
     * We are safe if we made no decisions.
     * @return
     */
    int isSafe() const {
    	return d_trail.getDecisionLevel() == 0;
    }

    const SearchTrail& getTrail() const {
        return d_trail;
    }

    template <VariableModificationType eventType, VariableType variableType>
    void enqueueEvent(Variable var, typename VariableTraits<variableType>::value_type newValue, ConstraintRef reason);

    void gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap);

    std::string getVariableName(Variable var) const {
    	if (var == VariableNull) {
    		return "null";
    	}
    	return d_variableNames[var.getId()];
    }

    void printTrail(std::ostream& out, bool useIntrnale = false);
    void printLowerBound(std::ostream& out, Variable var, unsigned trailIndex);
    void printUpperBound(std::ostream& out, Variable var, unsigned trailIndex);
    void printHeuristic(std::ostream& out) const;

    template<ConstraintType type>
    void printConstraint(const TypedConstraint<type>& constraint, std::ostream& out, OutputFormat format) const {
    	constraint.print(out, *this, format);
    }

};

template<>
inline const Integer& SolverState::getBound<TypeInteger>(unsigned i) const {
	return d_boundsInteger[i];
}

template<>
inline const Rational& SolverState::getBound<TypeRational>(unsigned i) const {
	return d_boundsRational[i];
}

template<>
inline unsigned SolverState::addBound<TypeInteger>(const Integer& boundValue) {
	unsigned index = d_boundsInteger.size();
	d_boundsInteger.push_back(boundValue);
	return index;
}

template<>
inline unsigned SolverState::addBound<TypeRational>(const Rational& boundValue) {
	unsigned index = d_boundsRational.size();
	d_boundsRational.push_back(boundValue);
	return index;
}

template<VariableType type>
inline unsigned addBound(typename VariableTraits<type>::value_type const& boundValue);

inline void SolverState::BacktrackVisitor::operator ()(const TrailElement& trailElement) {

    // Get the state of the variable
    Variable variable = trailElement.var;
    VariableInfo& variableInfo = d_state.d_variableInfo[variable.getId()];


    // Restore the bound info
    switch((VariableModificationType)trailElement.modificationType) {
      case MODIFICATION_LOWER_BOUND_REFINE: {
          // Remove the usage
          ConstraintRef reason = variableInfo.getLowerBoundConstraint();
          // Remove the user from the constraint
          if (reason != ConstraintManager::NullConstraint) {
            IntegerConstraint& constraint = d_state.d_cm.get<ConstraintTypeInteger>(reason);
            constraint.removeUser();
          }
          // If any bound on the variable is gone, inform the propagator later
    	  if (trailElement.init) {
    		  d_state.changeVariableHeuristicBound<MODIFICATION_LOWER_BOUND_REFINE, false>(variable);
    	  }
    	  // If the variable was assigned, it becomes unassigned, so we enqueue it
    	  if (variableInfo.getValueStatus() != ValueStatusUnassigned) {
    		  bool justAssigned = variableInfo.getAssignmentIndex() == variableInfo.getLowerBoundTrailIndex();
    		  // Also, if it's a global unit propagation, we will repropagate it
    		  if (reason == ConstraintManager::NullConstraint) {
    			  // It must not be a decision though
    			  if (variableInfo.getValueStatus() != ValueStatusAssignedToUpper || !justAssigned) {
    				  // Global unit bound, reassert after backtrack
    				  d_state.addToUnitReassertList<MODIFICATION_LOWER_BOUND_REFINE>(variable);
    			  }
    		  }
    	  	  if (justAssigned) {
        		  if (!d_state.inQueue(variable)) {
                      d_state.enqueueVariable(variable);
                  }
    	  		  variableInfo.setValueStatus(ValueStatusUnassigned, -1);
    	  	  }
    	  } else {
    		  // This is not a decision for sure, so we repropagate if global
    		  if (variableInfo.getLowerBoundConstraint() == ConstraintManager::NullConstraint) {
    			  d_state.addToUnitReassertList<MODIFICATION_LOWER_BOUND_REFINE>(variable);
    		  }
    	  }
    	  // Restore the old data
          variableInfo.popLowerBoundInfo();
          break;
      }
      case MODIFICATION_UPPER_BOUND_REFINE: {
          ConstraintRef reason = variableInfo.getUpperBoundConstraint();
    	  // Remove the user from the constraint
          if (reason != ConstraintManager::NullConstraint) {
            IntegerConstraint& constraint = d_state.d_cm.get<ConstraintTypeInteger>(reason);
            constraint.removeUser();
          }
          // If any bound on the variable is gone, inform the propagator later
    	  if (trailElement.init) {
    		  d_state.changeVariableHeuristicBound<MODIFICATION_UPPER_BOUND_REFINE, false>(variable);
    	  }
    	  // If the variable was assigned, it becomes unassigned, so we enqueue it
    	  if (variableInfo.getValueStatus() != ValueStatusUnassigned) {
    		  // Also, if it's a global unit propagation, we will re-propagate it
    		  bool justAssigned = variableInfo.getAssignmentIndex() == variableInfo.getUpperBoundTrailIndex();
    		  if (reason == ConstraintManager::NullConstraint) {
    			  // It must not be a decision though
    			  if (variableInfo.getValueStatus() != ValueStatusAssignedToLower || !justAssigned) {
    				  // Global unit bound, reassert after backtrack
    				  d_state.addToUnitReassertList<MODIFICATION_UPPER_BOUND_REFINE>(variable);
    			  }
    		  }
    		  if (justAssigned) {
                  if (!d_state.inQueue(variable)) {
                      d_state.enqueueVariable(variable);
                  }
        	  	  variableInfo.setValueStatus(ValueStatusUnassigned, -1);
    		  }
    	  } else {
    		  // This is not a decision for sure, so we re-propagate if global
    		  if (variableInfo.getUpperBoundConstraint() == ConstraintManager::NullConstraint) {
    			  d_state.addToUnitReassertList<MODIFICATION_UPPER_BOUND_REFINE>(variable);
    		  }
    	  }
          // Restore the old data
          variableInfo.popUpperBoundInfo();
          break;
      }
      default:
          assert(false);
    }

    switch (trailElement.var.getType()) {
    case TypeInteger:
    	d_state.d_boundsInteger.pop_back();
    	break;
    case TypeRational:
    	d_state.d_boundsRational.pop_back();
    	break;
    default:
    	break;
    }
}

template <VariableModificationType eventType, VariableType variableType>
void SolverState::enqueueEvent(Variable var, typename VariableTraits<variableType>::value_type newValue, ConstraintRef reason) {

    CUTSAT_TRACE("solver::state") << eventType << ": " << var << " to " << newValue << std::endl;

    // If we are already in conflict, just return
    if (d_inConflict) return;

    // Get the variable info
    VariableInfo& variableInfo = d_variableInfo[var.getId()];

    // Index of the next trail element
	unsigned trailIndex = getTrailSize();

    if (reason != ConstraintManager::NullConstraint) {
        // Add the user
        IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(reason);
        constraint.addUser();
    }

    switch(eventType) {
    case MODIFICATION_LOWER_BOUND_REFINE:
        // We are modifying the lower bound to the new bound
    	d_trail.push<MODIFICATION_LOWER_BOUND_REFINE>(var, !variableInfo.hasLowerBound());
		if (!variableInfo.hasLowerBound()) {
			changeVariableHeuristicBound<MODIFICATION_LOWER_BOUND_REFINE, true>(var);
		}
        // Actually set the bound
        setLowerBound<variableType>(var, newValue, reason, trailIndex);
        // Check for conflicts and assignment
        if (variableInfo.hasUpperBound()) {
        	typename VariableTraits<variableType>::value_type const& upperBound = getUpperBound<variableType>(var);
			if (newValue == upperBound && variableInfo.getValueStatus() == ValueStatusUnassigned) {
				// We have equal bound, its an assignment
				variableInfo.setValueStatus(ValueStatusAssigned, trailIndex);
				// Also set the phase
				d_variablePhase[var.getId()] = false;
			} else if (newValue > upperBound) {
				// Lower bound is bigger than the upper bound, conflict it is
				d_inConflict = true;
				d_conflictVariable = var;
			}
        }
        break;
    case MODIFICATION_UPPER_BOUND_REFINE:
        // We are modifying the upper bound to the new bound
    	d_trail.push<MODIFICATION_UPPER_BOUND_REFINE>(var, !variableInfo.hasUpperBound());
    	if (!variableInfo.hasUpperBound()) {
    		changeVariableHeuristicBound<MODIFICATION_UPPER_BOUND_REFINE, true>(var);
    	}
    	// Actually set the bound
        setUpperBound<variableType>(var, newValue, reason, trailIndex);
        // Check for conflicts and assignment
        if (variableInfo.hasLowerBound()) {
        	typename VariableTraits<variableType>::value_type const& lowerBound = getLowerBound<variableType>(var);
			if (newValue == lowerBound && variableInfo.getValueStatus() == ValueStatusUnassigned) {
				// We have equal bound, its an assignment
				variableInfo.setValueStatus(ValueStatusAssigned, trailIndex);
				// Also set the phase
				d_variablePhase[var.getId()] = true;
			} else if (newValue < lowerBound) {
				// Upper bound is smaller than the lower bound, conflict it is
				d_inConflict = true;
				d_conflictVariable = var;
			}
        }
        break;
    default:
        assert(false);
    }
}

}

