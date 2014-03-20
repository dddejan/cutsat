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

class ExplanationRemovalHeuristic : public Heuristic {

    /** The initial factor */
    static const double s_explanationConstraintsFactorInit = 1;

    /** The increase of the factor by time */
    static const double s_explanationConstraintsFactorIncrease = 1;

    /** Number of conflicts before we increase the factor */
    static const unsigned s_explanationConstraintsFactorAdjustInit = 100;

    /** The increase of the number of conflicts to adjust the factor */
    static const double s_explanationConstraintsFactorAdjustIncrease = 1.1;

    /** Fraction of the problem constraints */
    double d_explanationConstraintsFactor;

    /** Number of conflicts before we adjust the factor constraints */
    unsigned d_explanationConstraintsFactorAdjust;

    /** Number of conflicts */
    unsigned d_conflictsCount;

public:

    ExplanationRemovalHeuristic(const SolverStats& solverStats)
    : Heuristic(solverStats),
      d_explanationConstraintsFactor(s_explanationConstraintsFactorInit),
      d_explanationConstraintsFactorAdjust(s_explanationConstraintsFactorAdjustInit),
      d_conflictsCount(0)
    {}

    void conflict() {
        d_conflictsCount ++;
        if (d_conflictsCount == d_explanationConstraintsFactorAdjust) {
            d_conflictsCount = 0;
            d_explanationConstraintsFactor += s_explanationConstraintsFactorIncrease;
            d_explanationConstraintsFactorAdjust = ((double)d_explanationConstraintsFactorAdjust)*s_explanationConstraintsFactorAdjustIncrease;
        }
    }

    bool decide() {
        return (double)d_solverStats.explanationConstraints >=
            ((double)d_solverStats.problemConstraints)*d_explanationConstraintsFactor
                + 2*d_solverStats.variables;
    }
};

}
