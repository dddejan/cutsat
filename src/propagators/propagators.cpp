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

#include "propagators/propagators.h"
#include "propagators/clause_propagator.h"
#include "propagators/integer_propagator.h"

using namespace cutsat;

TraceTag propagators("propagators");

void PropagatorCollection::cancelUntil(int trailIndex) {
	while (!d_repropagationList.empty() && d_repropagationList.back().trailIndex > trailIndex) {
		d_toRepropagate.push_back(d_repropagationList.back());
		d_repropagationList.pop_back();
	}
}

void PropagatorCollection::repropagate()
{
    CUTSAT_TRACE_FN("propagators") << *this << std::endl;

    // First, re-propagate unit bounds
    d_solverState.reassertUnitBounds();

	// Try to repropagate the constraints
	while (!d_toRepropagate.empty() && !d_solverState.inConflict()) {
		unsigned oldTrailSize = d_solverState.getTrailSize();
		RepropagationInfo& current = d_toRepropagate.back();
		switch (ConstraintManager::getType(current.constraint)) {
			case ConstraintTypeClause:
				boost::fusion::at_c<ConstraintTypeClause>(d_propagators).setPropagationVariable(current.var);
				boost::fusion::at_c<ConstraintTypeClause>(d_propagators).repropagate(current.constraint);
				break;
			case ConstraintTypeCardinality:
				boost::fusion::at_c<ConstraintTypeClause>(d_propagators).setPropagationVariable(current.var);
				boost::fusion::at_c<ConstraintTypeClause>(d_propagators).repropagate(current.constraint);
				break;
			case ConstraintTypeInteger:
				boost::fusion::at_c<ConstraintTypeInteger>(d_propagators).setPropagationVariable(current.var);
				boost::fusion::at_c<ConstraintTypeInteger>(d_propagators).repropagate(current.constraint);
				break;
			default:
				assert(false);
		}
		// It propagated something again
		if (oldTrailSize < d_solverState.getTrailSize()) {
			d_repropagationList.push_back(RepropagationInfo(current.constraint, oldTrailSize, d_solverState.getTrail()[oldTrailSize].var));
		}
		// Remove from the list
		d_toRepropagate.pop_back();
	}

    // If got into a conflict we have to try later again
    while (!d_toRepropagate.empty()) {
        RepropagationInfo& current = d_toRepropagate.back();
        d_repropagationList.push_back(RepropagationInfo(current.constraint, d_solverState.getTrailSize()-1, current.var));
        d_toRepropagate.pop_back();
    }

	CUTSAT_TRACE("propagators") << *this << std::endl;
}

void PropagatorCollection::print(std::ostream& out) const {
	d_solverState.printTrail(out);
	out << "Propagation List" << std::endl;
	for (unsigned i = 0; i < d_repropagationList.size(); ++ i) {
		switch (ConstraintManager::getType(d_repropagationList[i].constraint)) {
			case ConstraintTypeClause:
				out << d_repropagationList[i].trailIndex << ": " << d_constraintManager.get<ConstraintTypeClause>(d_repropagationList[i].constraint) << std::endl;
				break;
			case ConstraintTypeInteger:
				out << d_repropagationList[i].trailIndex << ": " << d_constraintManager.get<ConstraintTypeInteger>(d_repropagationList[i].constraint) << std::endl;
				break;
			default:
				assert(false);
		}
	}
	out << "To-Repropagatte List" << std::endl;
	for (unsigned i = 0; i < d_toRepropagate.size(); ++ i) {
		switch (ConstraintManager::getType(d_toRepropagate[i].constraint)) {
			case ConstraintTypeClause:
				out << d_toRepropagate[i].trailIndex << ": " << d_constraintManager.get<ConstraintTypeClause>(d_toRepropagate[i].constraint) << std::endl;
				break;
			case ConstraintTypeInteger:
				out << d_toRepropagate[i].trailIndex << ": " << d_constraintManager.get<ConstraintTypeInteger>(d_toRepropagate[i].constraint) << std::endl;
				break;
			default:
				assert(false);
		}
	}
}

void PropagatorCollection::gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap) {
	// Update the propagators
	boost::fusion::for_each(d_propagators, realloc_all(reallocMap));
	// Update the repropagation constraints
	for (unsigned i = 0; i < d_repropagationList.size(); ++ i) {
		ConstraintRef& cref = d_repropagationList[i].constraint;
		if (ConstraintManager::getFlag(cref)) {
			cref = ConstraintManager::unsetFlag(cref);
			assert(reallocMap.find(cref) != reallocMap.end());
			cref = ConstraintManager::setFlag(reallocMap.find(cref)->second);
		} else {
			assert(reallocMap.find(cref) != reallocMap.end());
			cref = reallocMap.find(cref)->second;
		}
	}
	for (unsigned i = 0; i < d_toRepropagate.size(); ++ i) {
		ConstraintRef& cref = d_toRepropagate[i].constraint;
		if (ConstraintManager::getFlag(cref)) {
			cref = ConstraintManager::unsetFlag(cref);
			assert(reallocMap.find(cref) != reallocMap.end());
			cref = ConstraintManager::setFlag(reallocMap.find(cref)->second);
		} else {
			assert(reallocMap.find(cref) != reallocMap.end());
			cref = reallocMap.find(cref)->second;
		}
	}

}
