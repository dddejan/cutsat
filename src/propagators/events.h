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

namespace cutsat {

/**
 * Types of events that can change the state of a variable.
 */
enum VariableModificationType {
	/** Value of the lower bound was refined */
    MODIFICATION_LOWER_BOUND_REFINE,
    /** Value of the upper bound was refined */
    MODIFICATION_UPPER_BOUND_REFINE,
    /** Anything related to this variable has changed */
    MODIFICATION_ANY,
    /** Number of modification types */
    MODIFICATION_COUNT
};

inline std::ostream& operator << (std::ostream& out, const VariableModificationType type) {
    switch (type) {
    case MODIFICATION_LOWER_BOUND_REFINE:
        out << "rafine_lower_bound";
        break;
    case MODIFICATION_UPPER_BOUND_REFINE:
        out << "rafine_upper_bound";
        break;
    default:
        assert(false);
    }
    return out;
}

}
