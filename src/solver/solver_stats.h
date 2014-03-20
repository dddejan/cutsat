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

#include <iostream>
#include <boost/timer.hpp>

namespace cutsat {

struct SolverStats {
    /** Number of decisions made */
    unsigned decisions;
    /** Number of variables */
    unsigned variables;
    /** Number of conflicts during search */
    unsigned conflicts;
    /** Number of conflicts during conflict analysis */
    unsigned conflictsInAnalysis;
    /** Number of restarts */
    unsigned restarts;
    /** Number of problem constraints */
    unsigned problemConstraints;
    /** Number of explanation constraints */
    unsigned explanationConstraints;
    /** Number of generated global cuts */
    unsigned globalCutConstraints;
    /** Number of removed constraints */
    unsigned removedConstraints;
    /** Number of created clause constraints */
    unsigned clauseConstraints;
    /** Number of created cardinality constraints */
    unsigned cardinalityConstraints;
    /** Number of created integer constraints */
    unsigned integerConstraints;
    /** Number of Fourier-Motzkin cuts */
    unsigned fourierMotzkinCuts;
    /** Number of dynamic cuts */
    unsigned dynamicCuts;
    /** Allocated constraint manager memory */
    unsigned constraintManagerCapacity;
    /** Size of the constraint manager memory */
    unsigned constraintManagerSize;
    /** Wasted part of the constraint manager memory */
    unsigned constraintManagerWasted;
    /** The timer */
    boost::timer timer;

    SolverStats()
    : decisions(0),
      variables(0),
      conflicts(0),
      conflictsInAnalysis(0),
      restarts(0),
      problemConstraints(0),
      explanationConstraints(0),
      globalCutConstraints(0),
      removedConstraints(0),
      clauseConstraints(0),
      cardinalityConstraints(0),
      integerConstraints(0),
      fourierMotzkinCuts(0),
      dynamicCuts(0),
      constraintManagerCapacity(0),
      constraintManagerSize(0),
      constraintManagerWasted(0)
      {}
};


inline std::ostream& operator << (std::ostream& out, const SolverStats& stats) {
    out << "Decisions               : " << stats.decisions << std::endl
        << "Conflicts (search)      : " << stats.conflicts << std::endl
        << "Conflicts (analysis)    : " << stats.conflictsInAnalysis << std::endl
        << "Restarts                : " << stats.restarts << std::endl
        << "Variables               : " << stats.variables << std::endl
        << "Problem constraints     : " << stats.problemConstraints << std::endl
        << "Explanations            : " << stats.explanationConstraints << std::endl
        << "Global cuts             : " << stats.globalCutConstraints << std::endl
        << "Clause constraints      : " << stats.clauseConstraints << std::endl
        << "Cardinality constraints : " << stats.cardinalityConstraints << std::endl
        << "Integer constraints     : " << stats.integerConstraints << std::endl
        << "Removed constraints     : " << stats.removedConstraints << std::endl
        << "Fourier-Motzkin cuts    : " << stats.fourierMotzkinCuts << std::endl
        << "Dynamic cuts            : " << stats.dynamicCuts << std::endl
        << "Allocated memory        : " << stats.constraintManagerCapacity << std::endl
        << "Used memory             : " << stats.constraintManagerSize << std::endl
        << "Wasted memory           : " << stats.constraintManagerWasted << std::endl
        << "Elapsed time            : " << stats.timer.elapsed() << "s" << std::endl;
    return out;
}


}
