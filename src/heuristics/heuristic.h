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

#include "solver/solver_stats.h"
#include "constraints/constraint_manager.h"

namespace cutsat {

class Heuristic {

protected:

    const SolverStats& d_solverStats;

public:

    Heuristic(const SolverStats& solverStats)
    : d_solverStats(solverStats) {}

    /**
     * Called when a conflict is encountered.
     */
    virtual void conflict() {};

    /**
     * Called when the solver goes for a new start.
     */
    virtual void restart() {};

    /**
     * Main method that should return true if the heuristic decides that it's time
     * to perform something.
     */
    virtual bool decide() { return false; }
};

}