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

#include "heuristics/heuristic.h"

namespace cutsat {

class LubyRestartHeuristic : public Heuristic {

    /** The base of the luby sequence powers */
    static const double d_restartBase = 2;

    /** The initial number of conflicts for the restart */
    static const unsigned d_restartInit = 50;

    /** Number of restarts we had so far */
    unsigned d_restartsCount;

    /** Number of conflicts we had in this restart */
    unsigned d_conflictsCount;

    /** Limit on the number of conflicts in this restart */
    unsigned d_conflictsLimit;

    /**
     * Returns the power of the element at the given index in the Luby sequence.
     */
    unsigned luby(unsigned index);

public:

    /**
     * Called when the solver starts solving.
     */
    LubyRestartHeuristic(const SolverStats& solverStats)
    : Heuristic(solverStats), d_restartsCount(0), d_conflictsCount(0), d_conflictsLimit(d_restartInit) {}

    /**
     * Called when a conflict is encountered.
     */
    void conflict();

    /**
     * Called when the solver goes for a restart.
     */
    void restart();

    /**
     * Main method that should return true if the heuristic decides that it's time
     * to perform something.
     */
    bool decide();
};

}
