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

#include <string>
#include <vector>
#include <boost/static_assert.hpp>
#include <algorithm>

#include "util/config.h"
#include "constraints/variable.h"
#include "constraints/constraint_manager.h"

namespace cutsat {

/**
 * Status of the variable value.
 */
enum ValueStatus {
    /** Value of the variable is still unassigned */
    ValueStatusUnassigned = 0,
    /** Value of tre variable is assigned to its lower bound = upper bound */
    ValueStatusAssigned,
    /** Value of the variable is assigned to its lower bound */
    ValueStatusAssignedToLower,
    /** Value fo the variable is assigned to its upper bound */
    ValueStatusAssignedToUpper
};

/**
 * Status of the variable bound
 */
enum BoundStatus {
    /** Bound has still not been established */
    BoundStatusUnassigned,
    /** Current bound is a global bound (no propagation) */
    BoundStatusGlobal,
    /** Current bound was established at some point of the search */
    BoundStatusLocal
};

/**
 * Information about a bound.
 */
struct VariableBoundInfo {

    /** The index of the bound in the bounds table */
    unsigned boundIndex;
    /** The level at which this bound was introduced */
    unsigned trailIndex;

    /** Constraint imposing the lower bound */
    ConstraintRef boundConstraint;

    VariableBoundInfo(unsigned boundIndex, ConstraintRef boundContraint, unsigned trailIndex)
    : boundIndex(boundIndex), trailIndex(trailIndex), boundConstraint(boundContraint) {}
};

/** Information about the variables */
class VariableInfo {

private:

	static int findIndex(const std::vector<VariableBoundInfo>& info, unsigned trailIndex) {

		int left = 0;
		int right = info.size();

		while (left < right) {
		   int middle = (left + right) >> 1;
           if (trailIndex < info[middle].trailIndex) {
			   right = middle;
           } else {
        	   left = middle + 1;
           }
		}

		return left - 1;
	}

	static inline VariableBoundInfo& find(std::vector<VariableBoundInfo>& info, unsigned trailIndex) {
		return info[findIndex(info, trailIndex)];
	}

	static inline const VariableBoundInfo& find(const std::vector<VariableBoundInfo>& info, unsigned trailIndex) {
		return info[findIndex(info, trailIndex)];
	}

	/** The status of the variable assignment */
    ValueStatus d_valueStatus;
    /** If assigned, this is the trail index responsible */
    unsigned d_valueStatusTrailIndex;
    /** The lower bound information assignment */
    std::vector<VariableBoundInfo> d_lowerBoundInfo;
    /** The status of the upper bound assignment (BoundStatus) */
    std::vector<VariableBoundInfo> d_upperBoundInfo;

public:

    VariableInfo()
    : d_valueStatus(ValueStatusUnassigned), d_valueStatusTrailIndex(0) {}

    inline void setValueStatus(ValueStatus status, unsigned trailIndex) {
    	d_valueStatus = status;
    	if (status == ValueStatusUnassigned) {
    		d_valueStatusTrailIndex = -1;
    	} else {
    		d_valueStatusTrailIndex = trailIndex;
    	}
    }

    inline const ValueStatus getValueStatus() const {
    	return d_valueStatus;
    }

    inline const ValueStatus getValueStatus(unsigned trailIndex) const {
    	if (trailIndex < d_valueStatusTrailIndex) {
    		return ValueStatusUnassigned;
    	} else {
    		return d_valueStatus;
    	}
    }

    inline const int getAssignmentIndex(unsigned trailIndex) const {
    	if (trailIndex < d_valueStatusTrailIndex) {
    		return -1;
    	} else {
    		return d_valueStatusTrailIndex;
    	}
    }

    inline const int getAssignmentIndex() const {
    	return d_valueStatusTrailIndex;
    }

    inline const VariableBoundInfo& getLowerBoundInfo() const {
    	assert(hasLowerBound());
    	return d_lowerBoundInfo.back();
    }

    inline const VariableBoundInfo& getLowerBoundInfo(unsigned trailIndex) const {
    	assert(hasLowerBound(trailIndex));
    	return find(d_lowerBoundInfo, trailIndex);
    }

    inline const VariableBoundInfo& getUpperBoundInfo() const {
    	assert(hasUpperBound());
    	return d_upperBoundInfo.back();
    }

    inline const VariableBoundInfo& getUpperBoundInfo(unsigned trailIndex) const {
    	assert(hasUpperBound(trailIndex));
    	return find(d_upperBoundInfo, trailIndex);
    }

    inline bool hasLowerBound() const {
        return !d_lowerBoundInfo.empty();
    }

    inline bool hasLowerBound(unsigned trailIndex) const {
        return !d_lowerBoundInfo.empty() && d_lowerBoundInfo[0].trailIndex <= trailIndex;
    }

    inline int getLowerBoundIndex() const {
    	assert(hasLowerBound());
        return d_lowerBoundInfo.back().boundIndex;
    }

    inline int getLowerBoundIndex(unsigned trailIndex) const {
    	assert(hasLowerBound(trailIndex));
    	return find(d_lowerBoundInfo, trailIndex).boundIndex;
    }

    inline int getLowerBoundTrailIndex() const {
    	if (!hasLowerBound()) return -1;
        return d_lowerBoundInfo.back().boundIndex;
    }

    inline int getLowerBoundTrailIndex(unsigned trailIndex) const {
    	if (!hasLowerBound(trailIndex)) return -1;
    	return find(d_lowerBoundInfo, trailIndex).boundIndex;
    }

    inline ConstraintRef getLowerBoundConstraint() const {
    	assert(hasLowerBound());
    	return d_lowerBoundInfo.back().boundConstraint;
    }

    inline ConstraintRef getLowerBoundConstraint(unsigned trailIndex) const {
        assert(hasLowerBound(trailIndex));
    	return find(d_lowerBoundInfo, trailIndex).boundConstraint;
    }

    inline void setLowerBoundInfo(size_t boundIndex, ConstraintRef constraintRef, unsigned trailIndex) {
    	assert(d_lowerBoundInfo.empty() || d_lowerBoundInfo.back().trailIndex <= trailIndex);
    	d_lowerBoundInfo.push_back(VariableBoundInfo(boundIndex, constraintRef, trailIndex));
    }

    inline bool hasUpperBound() const {
        return !d_upperBoundInfo.empty();
    }

    inline bool hasUpperBound(unsigned trailIndex) const {
        return !d_upperBoundInfo.empty() && d_upperBoundInfo[0].trailIndex <= trailIndex;
    }

    inline int getUpperBoundIndex() const {
    	assert(hasUpperBound());
        return d_upperBoundInfo.back().boundIndex;
    }

    inline int getUpperBoundIndex(unsigned trailIndex) const {
    	assert(hasUpperBound(trailIndex));
        return find(d_upperBoundInfo, trailIndex).boundIndex;
    }

    inline int getUpperBoundTrailIndex() const {
    	if (!hasUpperBound()) return -1;
        return d_upperBoundInfo.back().trailIndex;
    }

    inline int getUpperBoundTrailIndex(unsigned trailIndex) const {
    	if (!hasUpperBound(trailIndex)) return -1;
        return find(d_upperBoundInfo, trailIndex).trailIndex;
    }

    inline ConstraintRef getUpperBoundConstraint() const {
    	assert(hasUpperBound());
        return d_upperBoundInfo.back().boundConstraint;
    }

    inline ConstraintRef getUpperBoundConstraint(unsigned trailIndex) const {
    	assert(hasUpperBound(trailIndex));
        return find(d_upperBoundInfo, trailIndex).boundConstraint;
    }

    template<bool includeAssignment>
    inline int getLastModificationTrailIndex(unsigned trailIndex) const {
    	if (!includeAssignment) {
    		ValueStatus valueStatus = getValueStatus(trailIndex);
    		if (valueStatus == ValueStatusAssignedToLower) {
    			trailIndex = getUpperBoundTrailIndex(trailIndex) - 1;
    		} else if (valueStatus == ValueStatusAssignedToUpper) {
    			trailIndex = getLowerBoundTrailIndex(trailIndex) - 1;
    		}
    	}
    	return std::max(
    		getLowerBoundTrailIndex(trailIndex),
    		getUpperBoundTrailIndex(trailIndex)
    	);
    }

    template<bool includeAssignment>
    inline int getLastModificationTrailIndex() const {
    	int topIndex = std::max(
    			getLowerBoundTrailIndex(),
    			getUpperBoundTrailIndex()
    			);
    	return getLastModificationTrailIndex<includeAssignment>(topIndex);
    }

    inline void setUpperBoundInfo(size_t boundIndex, ConstraintRef constraintRef, unsigned trailIndex) {
    	assert(d_upperBoundInfo.empty() || d_upperBoundInfo.back().trailIndex <= trailIndex);
    	d_upperBoundInfo.push_back(VariableBoundInfo(boundIndex, constraintRef, trailIndex));
    }

    inline void popLowerBoundInfo() {
    	d_lowerBoundInfo.pop_back();
    }

    inline void popUpperBoundInfo() {
    	d_upperBoundInfo.pop_back();
    }

    inline void gcUpdate(const std::map<ConstraintRef, ConstraintRef>& reallocMap) {
    	for(unsigned i = 0, i_end = d_lowerBoundInfo.size(); i < i_end; ++ i) {
    		ConstraintRef cRef = d_lowerBoundInfo[i].boundConstraint;
    		if (cRef != ConstraintManager::NullConstraint) {
    			if (ConstraintManager::getFlag(cRef)) {
    				assert(reallocMap.find(ConstraintManager::unsetFlag(cRef)) != reallocMap.end());
    				cRef = ConstraintManager::setFlag(reallocMap.find(ConstraintManager::unsetFlag(cRef))->second);
    			} else {
    				assert(reallocMap.find(cRef) != reallocMap.end());
    				cRef = reallocMap.find(cRef)->second;
    			}
    		}
    		d_lowerBoundInfo[i].boundConstraint = cRef;
    	}
    	for(unsigned i = 0, i_end = d_upperBoundInfo.size(); i < i_end; ++ i) {
    		ConstraintRef cRef = d_upperBoundInfo[i].boundConstraint;
    		if (cRef != ConstraintManager::NullConstraint) {
    			if (ConstraintManager::getFlag(cRef)) {
    				assert(reallocMap.find(ConstraintManager::unsetFlag(cRef)) != reallocMap.end());
    				cRef = ConstraintManager::setFlag(reallocMap.find(ConstraintManager::unsetFlag(cRef))->second);
    			} else {
    				assert(reallocMap.find(cRef) != reallocMap.end());
    				cRef = reallocMap.find(cRef)->second;
    			}
    		}
    		d_upperBoundInfo[i].boundConstraint = cRef;
    	}
    }

};

class VariableCompareByAssignmentTime {

	typedef boost::uint16_t assignment_id_type;

	/** Assignment ids by variable */
	std::vector<assignment_id_type> d_assignmentId;

	/** Current count of ids */
	assignment_id_type d_currentId;

public:

	VariableCompareByAssignmentTime()
	: d_currentId(0) {}

	inline void resize(size_t nVariables) {
		assert(boost::integer_traits<assignment_id_type>::const_max > nVariables);
		d_assignmentId.resize(nVariables);
	}

	inline void justAssigned(Variable var) {
		d_assignmentId[var.getId()] = d_currentId ++;
	}

	inline void unAssign() {
		assert(d_currentId > 0);
		-- d_currentId;
	}

	inline bool lt(Variable v1, Variable v2) const {
		return d_assignmentId[v1.getId()] < d_assignmentId[v2.getId()];
	}

	inline bool le(Variable v1, Variable v2) const {
		return d_assignmentId[v1.getId()] <= d_assignmentId[v2.getId()];
	}
};

}
