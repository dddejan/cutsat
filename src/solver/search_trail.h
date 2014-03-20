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
#include <boost/cstdint.hpp>

#include "util/config.h"
#include "solver/variable_info.h"
#include "propagators/events.h"
#include "constraints/constraint_manager.h"

namespace cutsat {

struct TrailElement {

	/** Is this the first modification of this property, i.e. initialization of the bound */
	unsigned init             : 1;
    /** The type of modification */
    unsigned modificationType : 31;
    /** The variable that is being changed */
    Variable var;

    TrailElement() {}

    TrailElement(VariableModificationType type, Variable var, bool init = false)
    : init(init ? 1 : 0), modificationType(type), var(var) {}
};

/**
 * A trail of changes to the state.
 */
class SearchTrail {

    /** The actual trail */
    std::vector<TrailElement> d_trail;

    /** Trail indices where decisions were made */
    std::vector<unsigned> d_decisions;

public:

    /** Cancels the trail up until the given level */
    template <typename Visitor>
    void cancelUntil(int trailIndex, Visitor& backtrackVisitor);

    template<VariableModificationType type>
    inline void push(Variable var, bool init);

    VariableModificationType getModificationTypeAt(unsigned trailIndex) const {
    	return (VariableModificationType) d_trail[trailIndex].modificationType;
    }

    Variable getVariableModifiedAt(unsigned trailIndex) const {
    	return d_trail[trailIndex].var;
    }

    size_t getSize() const { return d_trail.size(); }

    const TrailElement& operator[] (size_t index) const {
        return d_trail[index];
    }

    void newDecisionLevel() {
    	d_decisions.push_back(d_trail.size());
    }

    unsigned getLevelOfTrailIndex(unsigned trailIndex) const {
    	int i = 0, j = d_decisions.size();
    	while (i < j) {
    		int mid = (i + j) >> 1;
    		if (d_decisions[mid] <= trailIndex) {
    			i = mid + 1;
    		} else {
    			j = mid;
    		}
    	}
    	return i;
    }

    /** Returns the last index at given level */
    int getTrailIndexOfLevel(unsigned level) const {
    	assert(level <= d_decisions.size());
    	if (level == d_decisions.size()) {
    		return d_trail.size() - 1;
    	} else {
    		return d_decisions[level] - 1;
    	}
    }

    unsigned getDecisionLevel() const {
    	return d_decisions.size();
    }
};

template <>
inline void SearchTrail::push<MODIFICATION_LOWER_BOUND_REFINE>(Variable var, bool init) {
    CUTSAT_TRACE("trail") << var << "[" << MODIFICATION_LOWER_BOUND_REFINE << "]" << std::endl;
    d_trail.push_back(TrailElement(MODIFICATION_LOWER_BOUND_REFINE, var, init));
}

template <>
inline void SearchTrail::push<MODIFICATION_UPPER_BOUND_REFINE>(Variable var, bool init) {
    CUTSAT_TRACE("trail") << var << "[" << MODIFICATION_UPPER_BOUND_REFINE << "]" << std::endl;
    d_trail.push_back(TrailElement(MODIFICATION_UPPER_BOUND_REFINE, var, init));
}

template <typename Visitor>
void SearchTrail::cancelUntil(int trailIndex, Visitor& backtrackVisitor) {
    for(int i = d_trail.size() - 1; i > trailIndex; -- i) {
        backtrackVisitor(d_trail[i]);
        d_trail.pop_back();
        if (d_decisions.size() > 0 && d_decisions.back() == d_trail.size()) {
        	d_decisions.pop_back();
        }
    }
}

}

