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

#include "heuristics/restart_luby.h"

using namespace cutsat;

unsigned LubyRestartHeuristic::luby(unsigned index) {

    unsigned size = 1;
    unsigned maxPower = 0;

    // Find the first full subsequence such that total size covers the index
    while(size <= index) {
        size = 2*size + 1;
        ++ maxPower;
    }

    // Now get the exact value
    while (size > index + 1) {
        size = size / 2;
        -- maxPower;
        if (size <= index) {
            index -= size;
        }
    }

    return maxPower + 1;
}

void LubyRestartHeuristic::conflict() {
    d_conflictsCount ++;
}

void LubyRestartHeuristic::restart() {
    d_restartsCount ++;
    d_conflictsCount = 0;
    d_conflictsLimit = d_restartInit * pow(d_restartBase, luby(d_restartsCount));
}

bool LubyRestartHeuristic::decide() {
    return d_conflictsCount > d_conflictsLimit;
}


