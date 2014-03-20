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

#include "solver/solver_state.h"

using namespace std;
using namespace cutsat;

TraceTag solverStateTag("solver::state");

Variable SolverState::decideVariable() {
	Variable decision;
    // Get the new variable
    if (d_dynamicOrder) {
    	while (!d_variableQueueDynamic.empty()) {
    		Variable var = d_variableQueueDynamic.top();
    		d_variableQueueDynamic.pop();
    		d_variableQueuePosition[var.getId()] = d_variableQueueDynamic.end();
    		if (!isDecided(var)) {
    			return var;
    		}
    	}
    } else {
    	while (!d_variableQueueLinear.empty()) {
    		Variable var = d_variableQueueLinear.top();
    	    d_variableQueueLinear.pop();
    	    if (!isDecided(var)) {
    	    	return var;
    	    }
    	}
    }
    // We haven't found any variables to decide on, we're done
    return VariableNull;
}

void SolverState::decideValue(Variable var) {
	// Get the current variable information
    VariableInfo& varInfo = d_variableInfo[var.getId()];
    // Pick a value for the variable
    assert(hasLowerBound(var) || hasUpperBound(var));
    assert(!isAssigned(var));
    // Mark the new decision level
    d_trail.newDecisionLevel();

    // Which bound to choose
    bool chooseLower;
    bool hasLower = hasLowerBound(var);
    bool hasUpper = hasUpperBound(var);
    if (hasLower && !hasUpper) {
    	// If we only have lower, pick the lower
    	chooseLower = true;
    } else if (!hasLower && hasUpper) {
    	// If we only have the upper, pick the upper
    	chooseLower = false;
    } else {
    	// If we have both, pick by phase
    	chooseLower = d_variablePhase[var.getId()];
    }

    // Choose the value
    if (chooseLower) {
        CUTSAT_TRACE("solver") << "assigning " << var << " to lower bound." << endl;
        // In effect we are forcing refinement of the upper bound
        varInfo.setValueStatus(ValueStatusAssignedToLower, d_trail.getSize());
        switch (var.getType()) {
            case TypeInteger:
                enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(var, getLowerBound<TypeInteger>(var), ConstraintManager::NullConstraint);
                break;
            default:
                assert(false);
        }
    }
    else {
        assert(hasUpperBound(var));
        CUTSAT_TRACE("solver") << "assigning " << var << " to upper bound." << endl;
        // In effect we are forcing rafinement of the lower bound
        varInfo.setValueStatus(ValueStatusAssignedToUpper, d_trail.getSize());
        switch (var.getType()) {
            case TypeInteger:
                enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(var, getUpperBound<TypeInteger>(var), ConstraintManager::NullConstraint);
                break;
            default:
                assert(false);
        }
    }
}

void SolverState::resize(size_t size) {
    if (size > d_variableInfo.size()) {
        d_variableHeuristic.resize(size);
        d_variableQueuePosition.resize(size);
        d_variableInfo.resize(size);
        d_variableNames.resize(size);
        d_variablePhase.resize(size, true);
    }
}

void SolverState::newVariable(Variable var, const char* varName, bool addToQueue) {

    // Get the id of the variable
    size_t varId = var.getId();
    // Ensure the state can accommodate this variable
    resize(varId + 1);

    // Remember the name of the variable
    d_variableNames[varId] = varName;

    if (addToQueue) {
    	// Add the variable to the queue
        d_variableHeuristic[varId].value = 1.0;
        d_variableQueuePosition[varId] = d_variableQueueDynamic.push(var);
        d_variableQueueLinear.push(var);
    }
}

void SolverState::printHeuristic(ostream& out) const {
	out << "Heuristic values:" << endl;
	for (unsigned i = 0; i < d_variableNames.size(); ++ i) {
		out << d_variableNames[i] << ":" << d_variableHeuristic[i].value << endl;
	}
}

void SolverState::printTrail(std::ostream& out, bool useInternal) {
	for (unsigned i = 0, i_end = d_trail.getSize(); i < i_end; ++ i) {
		const TrailElement& trailElement = d_trail[i];
		Variable variable = trailElement.var;
		switch ((VariableModificationType)trailElement.modificationType) {
		case MODIFICATION_LOWER_BOUND_REFINE:
			if (getValueStatus(variable, i) == ValueStatusAssignedToUpper)  {
				out << std::endl << " || [" << i << ":";
				if (useInternal) {
					out << variable;
				} else {
					out << getVariableName(variable);
				}
				out << "=";
			} else {
				out << "[" << i << ":";
				if (useInternal) {
					out << variable;
				} else {
					out << getVariableName(variable);
				}
				out << ">=";
			}
			printLowerBound(out, variable, i);
			break;
		case MODIFICATION_UPPER_BOUND_REFINE:
			if (getValueStatus(variable, i) == ValueStatusAssignedToLower)  {
				out << std::endl << " || [" << i << ":";
				if (useInternal) {
					out << variable;
				} else {
					out << getVariableName(variable);
				}
				out << "=";
			} else {
				out << "[" << i << ":";
				if (useInternal) {
					out << variable;
				} else {
					out << getVariableName(variable);
				}
				out << "<=";
			}
			printUpperBound(out, variable, i);
			break;
		default:
			assert(false);
		}
		out << "]";
	}
	out << std::endl;
}

void SolverState::printLowerBound(std::ostream& out, Variable var, unsigned trailIndex) {
	switch (var.getType()) {
	case TypeInteger:
		out << getLowerBound<TypeInteger>(var, trailIndex);
		break;
	case TypeRational:
		out << getLowerBound<TypeRational>(var, trailIndex);
		break;
	default:
		assert(false);
	}
}

void SolverState::printUpperBound(std::ostream& out, Variable var, unsigned trailIndex) {
	switch (var.getType()) {
	case TypeInteger:
		out << getUpperBound<TypeInteger>(var, trailIndex);
		break;
	case TypeRational:
		out << getUpperBound<TypeRational>(var, trailIndex);
		break;
	default:
		assert(false);
	}
}

void SolverState::reassertUnitBounds() {
    // Reassert the integer bounds
    for(unsigned i = 0, i_end = d_integerReassertList.size(); i < i_end; ++ i) {
    	const reassert_info<Integer>& info = d_integerReassertList[i];
    	if (info.type == MODIFICATION_LOWER_BOUND_REFINE) {
    		if (!hasLowerBound(info.variable) || getLowerBound<TypeInteger>(info.variable) < info.value) {
    			enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(info.variable, info.value, ConstraintManager::NullConstraint);
    		}
    	} else {
    		if (!hasUpperBound(info.variable) || getUpperBound<TypeInteger>(info.variable) > info.value) {
    			enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(info.variable, info.value, ConstraintManager::NullConstraint);
    		}
    	}
    }
    d_integerReassertList.clear();
    // Reassert the rational bounds
    for(unsigned i = 0, i_end = d_rationalReassertList.size(); i < i_end; ++ i) {
    	const reassert_info<Rational>& info = d_rationalReassertList[i];
    	if (info.type == MODIFICATION_LOWER_BOUND_REFINE) {
    		if (!hasLowerBound(info.variable) || getLowerBound<TypeRational>(info.variable) < info.value) {
    			enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeRational>(info.variable, info.value, ConstraintManager::NullConstraint);
    		}
    	} else {
    		if (!hasUpperBound(info.variable) || getUpperBound<TypeRational>(info.variable) > info.value) {
    			enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeRational>(info.variable, info.value, ConstraintManager::NullConstraint);
    		}
    	}
    }
    d_rationalReassertList.clear();
}

void SolverState::getLinearOrder(std::vector<Variable>& output) {
	output.clear();
	while (!d_variableQueueLinear.empty()) {
		Variable var = d_variableQueueLinear.top();
		d_variableQueueLinear.pop();
		output.push_back(var);
	}
	for(unsigned i = 0, i_end = output.size(); i < i_end; ++ i) {
		d_variableQueueLinear.push(output[i]);
	}
}

void SolverState::gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap) {
	for (unsigned i = 0, i_end = d_variableInfo.size(); i < i_end; ++ i) {
		d_variableInfo[i].gcUpdate(reallocMap);
	}
}
