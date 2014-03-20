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

#include "constraints/constraint_manager.h"
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace boost;
using namespace cutsat;

TraceTag constraints("constraints");

ConstraintManager::ConstraintManager(size_t initialSize)
: d_capacity(initialSize), d_size(0), d_variablesCount(0) {
    // Initialize all the memory
    d_memory = (char*)std::malloc(s_initialSize);
    d_wasted   = 0;

    CUTSAT_TRACE("constraints") << "Data bits: " << s_data_bits << std::endl;
    CUTSAT_TRACE("constraints") << "Type mask: " << s_type_mask << std::endl;
    CUTSAT_TRACE("constraints") << "Flag mask: " << s_flag_mask << std::endl;
}

Variable ConstraintManager::newVariable(VariableType type) {
    Variable newVariable(type, d_variablesCount ++);
    CUTSAT_TRACE("constraints") << "newVariable(" << type << ") => " << newVariable << std::endl;
    d_variableOccursCount.resize(2*d_variablesCount);
    return newVariable;
}

void ConstraintManager::gcBegin() {
	d_gcMemory = (char*)malloc(d_capacity);
	d_gcSize = 0;
}

void ConstraintManager::gcMove(std::vector<ConstraintRef>& constraints, std::map<ConstraintRef, ConstraintRef>& reallocMap) {
	for(unsigned i = 0, i_end = constraints.size(); i < i_end; ++ i) {
		// Old constraint
		ConstraintRef oldConstraintRef = constraints[i];
		if (oldConstraintRef == ConstraintManager::NullConstraint) {
			continue;
		}
		assert(!getFlag(oldConstraintRef));
		unsigned index = getIndex(oldConstraintRef);

		// Size of the constraint
		size_t size = 0;
		ConstraintType type = getType(oldConstraintRef);
		switch (type) {
        case ConstraintTypeClause:
            size = sizeof(ClauseConstraint) + sizeof(ClauseConstraintLiteral)*get<ConstraintTypeClause>(oldConstraintRef).getSize();
            break;
		case ConstraintTypeInteger:
			size = sizeof(IntegerConstraint) + sizeof(IntegerConstraintLiteral)*get<ConstraintTypeInteger>(oldConstraintRef).getSize();
			break;
		default:
			assert(false);
		}
		size = align(size);
		// Move the memory
		memcpy(d_gcMemory + d_gcSize, d_memory + index, size);
        // The new reference
        ConstraintRef newConstraintRef = getConstraintRef(type, d_gcSize);
		// Add to the realloc map
		reallocMap[oldConstraintRef] = newConstraintRef;
		// Adjust the new size
		d_gcSize += size;

        // Replace in the list
        constraints[i] = newConstraintRef;
    }
}

void ConstraintManager::gcEnd() {
	free(d_memory);
	d_memory = d_gcMemory;
	d_size = d_gcSize;
	d_wasted = 0;
	d_gcMemory = NULL;
}
