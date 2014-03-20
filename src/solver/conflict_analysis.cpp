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

#include "util/config.h"
#include "util/scoped.h"
#include "solver/solver.h"

using namespace std;
using namespace cutsat;

void Solver::analyzeConflict() {

    CUTSAT_TRACE_FN("solver");

    assert(d_state.inConflict());

    // Disable the propagation loop at this point
    Scoped<bool> disablePropagation(d_disablePropagation);
    d_disablePropagation = true;

    if (d_solverStats.conflicts == 0) {
        Trace::enableAll();
        d_verbosity = VERBOSITY_EXTREME;
    }

    // Although we resolve the conflict, the learned constraing might be stronger than
    // We ensvisioned, so we must check again
    while (d_state.inConflict()) {

        // Clear the conflict variables
        d_conflictVariables.clear();

        // Clear the conflict constraints
        d_conflictConstraints.clear();

        // Clear the cache
        //d_tightConstraintCache.clear();

        // Get the variable
        Variable conflictVariable = d_state.getConflictVariable();

        // Map from variables to coefficients for the lower bound
        constraint_coefficient_map lowerBoundCoefficients;
        // Map from variables to coefficients for the upper bound
        constraint_coefficient_map upperBoundCoefficients;
        // Map from variable to coefficients for the resulting constraint
        constraint_coefficient_map resultCoefficients;

        // Constants
        Integer cLower, cUpper, resultConstant;

        // Get the initial lower and upper bound constraints
        setUpConstraintMap(conflictVariable, d_state.getTrailSize(), MODIFICATION_LOWER_BOUND_REFINE, lowerBoundCoefficients, cLower);
        setUpConstraintMap(conflictVariable, d_state.getTrailSize(), MODIFICATION_UPPER_BOUND_REFINE, upperBoundCoefficients, cUpper);

        // We start by taking resolvent to be the one of the constraints (any strategy is better than no strategy)
        VariableModificationType resolvent;
        if (NumberUtils<Integer>::abs(lowerBoundCoefficients.coefficients[conflictVariable]) <
            NumberUtils<Integer>::abs(upperBoundCoefficients.coefficients[conflictVariable])) {
            resolvent = MODIFICATION_LOWER_BOUND_REFINE;
        } else {
            resolvent = MODIFICATION_UPPER_BOUND_REFINE;
        }

        // Resolve until not in conflict
        while (true) {

            // One more conflict in analysis
            d_solverStats.conflictsInAnalysis ++;

            // Print the trail if asked
            if (d_verbosity >= VERBOSITY_EXTREME) {
                cout << "Conflict detected at level " << d_state.getTrail().getDecisionLevel() << "(" << d_solverStats.conflicts << ")" << std::endl;
                cout << "Current trail:" << std::endl;
                d_state.printTrail(cout);
            }

            CUTSAT_TRACE("solver") << "LB: " << lowerBoundCoefficients << ">= " << cLower << endl;
            CUTSAT_TRACE("solver") << "UB: " << upperBoundCoefficients << ">= " << cUpper << endl;
            CUTSAT_TRACE("solver") << "In conflict: " << conflictVariable << std::endl;

            // Try and resolve the two constraints
            if (d_tryFourierMotzkin) {
                resolveCoefficientMaps(conflictVariable, lowerBoundCoefficients, cLower, upperBoundCoefficients, cUpper, resultCoefficients, resultConstant);
                CUTSAT_TRACE("solver") << "Resolved FM " << resultCoefficients << ">= " << resultConstant << std::endl;
            }
            // If FM not possible go for tight cuts
            if (!d_tryFourierMotzkin || !isInConflict(resultCoefficients, resultConstant)) {
                CUTSAT_TRACE("solver") << "FM not possible, going for tight constraints";

                // Get the tight constraint for the lower bound (if it's not the resolvent)
                if (resolvent != MODIFICATION_LOWER_BOUND_REFINE) {
                    getTighltyPropagatingConstraint<MODIFICATION_LOWER_BOUND_REFINE, true>(conflictVariable, d_state.getTrailSize()-1, lowerBoundCoefficients, cLower);
                }
                CUTSAT_TRACE("solver") << "Lower tight " << lowerBoundCoefficients << ">= " << cLower << std::endl;

                // Get the tight constraint for the upper bound (if it's not the resolvent)
                if (resolvent != MODIFICATION_UPPER_BOUND_REFINE) {
                    getTighltyPropagatingConstraint<MODIFICATION_UPPER_BOUND_REFINE, true>(conflictVariable, d_state.getTrailSize()-1, upperBoundCoefficients, cUpper);
                }
                CUTSAT_TRACE("solver") << "Upper tight " << upperBoundCoefficients << ">= " << cUpper << std::endl;

                // Try and resolve the two constraints
                resolveCoefficientMaps(conflictVariable, lowerBoundCoefficients, cLower, upperBoundCoefficients, cUpper, resultCoefficients, resultConstant);
                CUTSAT_TRACE("solver") << "Resolved tight " << resultCoefficients << ">= " << resultConstant << std::endl;

                d_solverStats.dynamicCuts ++;
            } else {
                d_solverStats.fourierMotzkinCuts ++;
            }

            // If no literals, we're done, the problem is unsat
            if (resultCoefficients.coefficients.size() == 0) {
                assert(resultConstant > 0);
                d_status = Unsatisfiable;
                return;
            }

            // Get the top decision variable and the top trail index at the previous level to backtrack to
            int topTrailIndex;
            getTopTrailInfo(resultCoefficients, conflictVariable, topTrailIndex);
            backtrack(topTrailIndex);

            // Asserting a constraint might introduce a conflict on the top variable so let's check
            if (isInConflict(resultCoefficients, resultConstant)) {
                // If we're in conflict at level 0, we're done
                if (d_state.isSafe()) {
                    d_status = Unsatisfiable;
                    return;
                }
                // Since in conflict, we keep the result on one side
                if (resultCoefficients.coefficients[conflictVariable] > 0) {
                    // Lower bound
                    lowerBoundCoefficients.swap(resultCoefficients);
                    std::swap(resultConstant, cLower);
                    // Upper bound
                    setUpConstraintMap(conflictVariable, d_state.getTrailSize(), MODIFICATION_UPPER_BOUND_REFINE, upperBoundCoefficients, cUpper);
                    // Mark the resolvent
                    resolvent = MODIFICATION_LOWER_BOUND_REFINE;
                } else {
                    // Lower bound
                    setUpConstraintMap(conflictVariable, d_state.getTrailSize(), MODIFICATION_LOWER_BOUND_REFINE, lowerBoundCoefficients, cLower);
                    // Upper bound
                    upperBoundCoefficients.swap(resultCoefficients);
                    std::swap(resultConstant, cUpper);
                    // Mark the resolvent
                    resolvent = MODIFICATION_UPPER_BOUND_REFINE;
                }
            } else {
                // We're done, not in conflict anymore
                break;
            }
        }

        // Assert the new constraint
        unsigned oldTrailSize = d_state.getTrailSize();
        d_propagators.setPropagatingInfo(conflictVariable);
        ConstraintRef conflictConstraint = assertTightConstraint(resultCoefficients, resultConstant);
        assert(d_state.getTrailSize() > oldTrailSize);

        // Bump the conflict variables
        std::map<Variable, double>::const_iterator vars_it = d_conflictVariables.begin();
        std::map<Variable, double>::const_iterator vars_it_end = d_conflictVariables.end();
        for (; vars_it != vars_it_end; ++ vars_it) {
            d_state.bumpVariable(vars_it->first, vars_it->second);
        }

        // Bump the conflict constraints
        std::set<ConstraintRef>::const_iterator c_it = d_conflictConstraints.begin();
        std::set<ConstraintRef>::const_iterator c_it_end = d_conflictConstraints.end();
        for (; c_it != c_it_end; ++ c_it) {
            bumpConstraint(*c_it);
        }

        // Bump the conflict constraint
        if (conflictConstraint != ConstraintManager::NullConstraint) {
            bumpConstraint(conflictConstraint);
            if (d_verbosity >= VERBOSITY_DETAILED) {
                cout << "Learned cut: ";
                switch(ConstraintManager::getType(conflictConstraint)) {
                    case ConstraintTypeClause:
                        d_state.printConstraint<ConstraintTypeClause>(d_cm.get<ConstraintTypeClause>(conflictConstraint), cout, OutputFormatIlp);
                        break;
                    case ConstraintTypeCardinality:
                        d_state.printConstraint<ConstraintTypeCardinality>(d_cm.get<ConstraintTypeCardinality>(conflictConstraint), cout, OutputFormatIlp);
                        break;
                    case ConstraintTypeInteger:
                        d_state.printConstraint<ConstraintTypeInteger>(d_cm.get<ConstraintTypeInteger>(conflictConstraint), cout, OutputFormatIlp);
                        break;
                    default:
                        assert(false);
                }
                cout << std::endl;
            }
        }

        // Also, go though the caches constraints and assert any unit constraints
        std::map<prop_variable_tag, tight_cache_element>::const_iterator t_it = d_tightConstraintCache.begin();
        std::map<prop_variable_tag, tight_cache_element>::const_iterator t_it_end = d_tightConstraintCache.end();
        for (; t_it != t_it_end; ++ t_it) {
            const constraint_coefficient_map& coefficients = t_it->second.coefficients;
            Integer rhs = t_it->second.constant;
            if (coefficients.coefficients.size() == 1) {
                Variable var = coefficients.coefficients.begin()->first;
                Integer coefficient = coefficients.coefficients.begin()->second;
                if (coefficient > 0) {
                    // Lower bound
                    Integer bound = NumberUtils<Integer>::divideUp(rhs, coefficient);
                    if (!d_state.hasLowerBound(var) || bound > d_state.getLowerBound<TypeInteger>(var)) {
                        assertTightConstraint(coefficients, rhs);
                    }
                } else {
                    // Upper bound
                    Integer bound = NumberUtils<Integer>::divideDown(rhs, coefficient);
                    if (!d_state.hasUpperBound(var) || bound < d_state.getUpperBound<TypeInteger>(var)) {
                        assertTightConstraint(coefficients, rhs);
                    }
                }
            }
        }
    }
}

ConstraintRef Solver::assertTightConstraint(const constraint_coefficient_map& coefficients, Integer& constant) {

    CUTSAT_TRACE_FN("solver");

    // Construct the literals
    constraint_coefficient_map::const_iterator it = coefficients.coefficients.begin();
    constraint_coefficient_map::const_iterator it_end = coefficients.coefficients.end();

    switch(coefficients.constraintType) {
    case ConstraintTypeInteger: {
        // general integer constraint
        std::vector<IntegerConstraintLiteral> literals;
        for (; it != it_end; ++ it) {
            literals.push_back(IntegerConstraintLiteral(it->second, it->first));
        }
        Integer constantCopy = constant;
        return assertIntegerConstraint(literals, constantCopy, CONSTRAINT_CLASS_EXPLANATION);
    }
    case ConstraintTypeCardinality: {
    	// Clause constraint
    	std::vector<CardinalityConstraintLiteral> literals;
    	int negativeLiterals = 0;
    	for (; it != it_end; ++ it) {
    		bool negated = it->second < 0;
    		if (negated) { negativeLiterals ++; }
    		literals.push_back(CardinalityConstraintLiteral(it->first, negated));
    	}
    	unsigned c = NumberUtils<Integer>::toInt(constant) + negativeLiterals;
    	return assertCardinalityConstraint(literals, c, CONSTRAINT_CLASS_EXPLANATION);
    }
    case ConstraintTypeClause: {
    	// Clause constraint
    	std::vector<ClauseConstraintLiteral> literals;
    	for (; it != it_end; ++ it) {
    		literals.push_back(ClauseConstraintLiteral(it->first, it->second < 0));
    	}
    	return assertClauseConstraint(literals, CONSTRAINT_CLASS_EXPLANATION);
    }
    default:
    	assert(false);
    }
    return ConstraintManager::NullConstraint;
}


template<VariableModificationType type, bool replace>
void Solver::getTighltyPropagatingConstraint(Variable x, unsigned trailIndex, constraint_coefficient_map& outCoefficients, Integer& outConstant) {

    CUTSAT_TRACE_FN("solver") << "[" << type << "] " << x << " for " << outCoefficients << ">= " << outConstant << " at trail index "  << trailIndex << std::endl;

    // Get the coefficient with x
    Integer xCoefficient = outCoefficients[x];
    assert(xCoefficient != 0);
    Integer xCoefficientAbs = NumberUtils<Integer>::abs(xCoefficient);

    // Constraint is tight if coefficient with x is 1
    if (xCoefficientAbs == 1) {
        // Cache the result
        prop_variable_tag cache_tag(x, trailIndex, type);
        d_tightConstraintCache[cache_tag].coefficients = outCoefficients;
        d_tightConstraintCache[cache_tag].constant = outConstant;
        return;
    }

    // Check if it's in the cache already (unless we are dealing with the resolvent)
    if (!replace) {
        constraint_coefficient_map tightCoefficients;
        prop_variable_tag var_tag(x, trailIndex, type);
        std::map<prop_variable_tag, tight_cache_element>::const_iterator find = d_tightConstraintCache.find(var_tag);
        if (find != d_tightConstraintCache.end()) {
            // In cache
            outCoefficients = find->second.coefficients;
            outConstant = find->second.constant;
            return;
        }
    }

    std::map<prop_variable_tag, Integer> coefficients;
    std::set<prop_variable_tag> inQueue;

    typedef __gnu_pbds::priority_queue<prop_variable_tag> var_queue;
    var_queue queue;

    // Construct the initial coefficients of the propagating constraint
    constraint_coefficient_map::const_iterator it = outCoefficients.begin();
    constraint_coefficient_map::const_iterator it_end = outCoefficients.end();
    for(; it != it_end; ++ it) {
        Variable variable = it->first;
        VariableModificationType modType = MODIFICATION_COUNT;
        unsigned propIndex = trailIndex;
        if (variable != x) {
            switch(d_state.getValueStatus(variable, trailIndex)) {
                case ValueStatusAssignedToLower:
                    propIndex = d_state.getLowerBoundTrailIndex(variable, trailIndex);
                    modType = MODIFICATION_LOWER_BOUND_REFINE;
                    break;
                case ValueStatusAssignedToUpper:
                    propIndex = d_state.getUpperBoundTrailIndex(variable, trailIndex);
                    modType = MODIFICATION_UPPER_BOUND_REFINE;
                    break;
                default:
                    if (it->second > 0) {
                        // Upper bound
                        propIndex = d_state.getUpperBoundTrailIndex(variable, trailIndex);
                        modType = MODIFICATION_UPPER_BOUND_REFINE;
                    } else {
                        // Lower bound
                        propIndex = d_state.getLowerBoundTrailIndex(variable, trailIndex);
                        modType = MODIFICATION_LOWER_BOUND_REFINE;
                    }
            }
        }
        prop_variable_tag var_tag(variable, propIndex, modType);
        if (variable != x) {
            CUTSAT_TRACE("solver") << "Adding " << variable << " with time " << propIndex << std::endl;
            queue.push(var_tag);
            inQueue.insert(var_tag);
        }
        coefficients[var_tag] = it->second;
    }
    // Clear the output
    outCoefficients.clear();

    // Constraint is not tight, so we setup the queue for the variables
    while (!queue.empty()) {
        // Get the variable
        prop_variable_tag var_tag = queue.top();
        queue.pop();
        inQueue.erase(var_tag);
        Variable variable = var_tag.variable;
        Integer variableCoefficient = coefficients[var_tag];
        unsigned variableIndex = var_tag.lastModificationTime;

        CUTSAT_TRACE("solver") << "Eliminating " << variable << " with coefficient " << variableCoefficient << " at time " << variableIndex << std::endl;

        // If the coefficient is integer, we're done with this one
        if (NumberUtils<Integer>::divides(xCoefficientAbs, variableCoefficient) && variable != x) {
            continue;
        }

        // Otherwise get the propagating constraint for this one
        constraint_coefficient_map tightCoefficients;
        Integer tightRHS;
        std::map<prop_variable_tag, tight_cache_element>::const_iterator find = d_tightConstraintCache.find(var_tag);
        if (find == d_tightConstraintCache.end()) {
            // Not in cache
            switch (var_tag.type) {
            case MODIFICATION_LOWER_BOUND_REFINE:
                setUpConstraintMap(variable, variableIndex, MODIFICATION_LOWER_BOUND_REFINE, tightCoefficients, tightRHS);
                getTighltyPropagatingConstraint<MODIFICATION_LOWER_BOUND_REFINE, false>(variable, variableIndex, tightCoefficients, tightRHS);
                break;
            case MODIFICATION_UPPER_BOUND_REFINE:
                setUpConstraintMap(variable, variableIndex, MODIFICATION_UPPER_BOUND_REFINE, tightCoefficients, tightRHS);
                getTighltyPropagatingConstraint<MODIFICATION_UPPER_BOUND_REFINE, false>(variable, variableIndex, tightCoefficients, tightRHS);
                break;
            default:
                assert(false);
            }
        } else {
            // In cache
            tightCoefficients = find->second.coefficients;
            tightRHS = find->second.constant;
        }
        CUTSAT_TRACE("solver") << "Tight: " << tightCoefficients << ">= " << tightRHS << std::endl;

        // The coefficient of the variable in the tight one
        Integer variableCoefficientTight = tightCoefficients[variable];
        assert(variableCoefficientTight == 1 || variableCoefficientTight == -1);

        // Our multiplier is now computed like this (non-tight = a , tight coef = b)
        // ... + ax + ... >= ... (non-tight)
        //       bx + ... >= ... (tight)
        // We need the resulting coefficient to be divisible by xCoefficient. If we can cancel it out, we just do so.
        Integer multiplier = -variableCoefficient*variableCoefficientTight;
        // If we can't cancel it out, we must add just enough multiples of xCoefficient to make it positive
        if (multiplier < 0) {
            multiplier +=
                    NumberUtils<Integer>::divideUp(-multiplier, xCoefficientAbs)*xCoefficientAbs;
        }

        CUTSAT_TRACE("solver") << "Multiplier: " << multiplier << std::endl;

        // Almost done
        constraint_coefficient_map::const_iterator it = tightCoefficients.begin();
        constraint_coefficient_map::const_iterator it_end = tightCoefficients.end();
        for(; it != it_end; ++ it) {
            Variable tightVariable = it->first;
            unsigned propIndex = variableIndex;
            VariableModificationType modType = MODIFICATION_COUNT;
            if (tightVariable != variable) {
                switch(d_state.getValueStatus(tightVariable, variableIndex)) {
                case ValueStatusAssignedToLower:
                    propIndex = d_state.getLowerBoundTrailIndex(tightVariable, variableIndex);
                    modType = MODIFICATION_LOWER_BOUND_REFINE;
                    break;
                case ValueStatusAssignedToUpper:
                    propIndex = d_state.getUpperBoundTrailIndex(tightVariable, variableIndex);
                    modType = MODIFICATION_UPPER_BOUND_REFINE;
                    break;
                default:
                    if (it->second > 0) {
                        // Upper bound
                        propIndex = d_state.getUpperBoundTrailIndex(tightVariable, variableIndex);
                        modType = MODIFICATION_UPPER_BOUND_REFINE;
                    } else {
                        // Lower bound
                        propIndex = d_state.getLowerBoundTrailIndex(tightVariable, variableIndex);
                        modType = MODIFICATION_LOWER_BOUND_REFINE;
                    }
                }
            }
            if (tightVariable != variable) {
                prop_variable_tag tight_var_tag(tightVariable, propIndex, modType);
                if (inQueue.find(tight_var_tag) == inQueue.end()) {
                    inQueue.insert(tight_var_tag);
                    queue.push(tight_var_tag);
                }
                coefficients[tight_var_tag] += tightCoefficients[tightVariable] * multiplier;
            } else {
                coefficients[var_tag] += tightCoefficients[tightVariable] * multiplier;
            }
        }

        // Also add to the RHS
        outConstant += tightRHS * multiplier;

        CUTSAT_TRACE("solver") << "RHS" << outConstant << std::endl;
    }

    // Move the coefficients into the output (all divisible by xCoefficient)
    std::map<prop_variable_tag, Integer>::const_iterator out_it = coefficients.begin();
    std::map<prop_variable_tag, Integer>::const_iterator out_it_end = coefficients.end();
    for(; out_it != out_it_end; ++ out_it) {
        Variable variable = out_it->first.variable;
        const Integer& coefficient = out_it->second;
        if (coefficient != 0) {
            assert(NumberUtils<Integer>::divides(xCoefficientAbs, coefficient));
            outCoefficients[variable] += NumberUtils<Integer>::divideUp(coefficient, xCoefficientAbs);
        }
    }

    // Round the constant
    outConstant = NumberUtils<Integer>::divideUp(outConstant, xCoefficientAbs);

    // Cache the result
    prop_variable_tag cache_tag(x, trailIndex, type);
    d_tightConstraintCache[cache_tag].coefficients = outCoefficients;
    d_tightConstraintCache[cache_tag].constant = outConstant;
}

Variable Solver::getTopVariable(ConstraintRef constraintRef) {

    int topTrailIndex = -1;
    Variable topVariable;

    // General integer constraints
    if (ConstraintManager::getType(constraintRef) == ConstraintTypeInteger) {
        const IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(constraintRef);
        CUTSAT_TRACE_FN("solver") << constraint;
        for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
            const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
            Variable literalVariable = literal.getVariable();
            int trailIndex = d_state.getLastModificationTrailIndex<true>(literalVariable);
            if (trailIndex > topTrailIndex) {
                topTrailIndex = trailIndex;
                topVariable = literalVariable;
            }
        }
    }

    // Clause constraints
    if (ConstraintManager::getType(constraintRef) == ConstraintTypeClause) {
        const ClauseConstraint& constraint = d_cm.get<ConstraintTypeClause>(constraintRef);
        CUTSAT_TRACE_FN("solver") << constraint;
        for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
            const ClauseConstraintLiteral& literal = constraint.getLiteral(i);
            Variable literalVariable = literal.getVariable();
            int trailIndex = d_state.getLastModificationTrailIndex<true>(literalVariable);
            if (trailIndex > topTrailIndex) {
                topTrailIndex = trailIndex;
                topVariable = literalVariable;
            }
        }
    }

    CUTSAT_TRACE_FN("solver") << "=> " << topVariable;

    return topVariable;
}

void Solver::setUpConstraintMap(Variable var, unsigned trailIndex, VariableModificationType modificationType, constraint_coefficient_map& coefficients, Integer& constant) {

    coefficients.clear();

    ConstraintRef propagatingConstraintRef = ConstraintManager::NullConstraint;

    // We prefer the variables that propagate withouth being assigned
    double bumpValue = 1;

    // Get the constraint that propagated the variable and handle the case when it's unit
    switch (modificationType) {
    case MODIFICATION_LOWER_BOUND_REFINE:
        propagatingConstraintRef = d_state.getLowerBoundConstraint(var, trailIndex);
        if (propagatingConstraintRef == ConstraintManager::NullConstraint) {
            // var >= bound
            coefficients[var] = 1;
            constant = d_state.getLowerBound<TypeInteger>(var, trailIndex);
            if (constant >= 0 && d_state.hasUpperBound(var, trailIndex) && d_state.getUpperBound<TypeInteger>(var, trailIndex) <= 1) {
            	coefficients.constraintType = ConstraintTypeClause;
            } else {
            	coefficients.constraintType = ConstraintTypeInteger;
            }
            d_conflictVariables[var] += bumpValue;
            return;
        }
        break;
    case MODIFICATION_UPPER_BOUND_REFINE:
        propagatingConstraintRef = d_state.getUpperBoundConstraint(var, trailIndex);
        if (propagatingConstraintRef == ConstraintManager::NullConstraint) {
            // -x >= -bound
            coefficients[var] = -1;
            constant = -d_state.getUpperBound<TypeInteger>(var, trailIndex);
            if (constant <= 1 && d_state.hasLowerBound(var, trailIndex) && d_state.getLowerBound<TypeInteger>(var, trailIndex) >= 0) {
            	coefficients.constraintType = ConstraintTypeClause;
            } else {
            	coefficients.constraintType = ConstraintTypeInteger;
            }
            d_conflictVariables[var] += bumpValue;
            return;
        }
        break;
    default:
        assert(false);
    }

    // Bump the constraint (we are using it)
    d_conflictConstraints.insert(propagatingConstraintRef);

    // If not unit, get the constraint and setup the map
    coefficients.constraintType = ConstraintManager::getType(propagatingConstraintRef);
    switch (coefficients.constraintType) {
    case ConstraintTypeClause: {
            const ClauseConstraint& constraint = d_cm.get<ConstraintTypeClause>(propagatingConstraintRef);
            for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
                const ClauseConstraintLiteral& literal = constraint.getLiteral(i);
                Variable literalVariable = literal.getVariable();
                coefficients[literalVariable] = literal.getCoefficient();
                d_conflictVariables[literalVariable] += bumpValue;
            }
            constant = constraint.getConstant();
            break;
        }
    case ConstraintTypeCardinality: {
        const ClauseConstraint& constraint = d_cm.get<ConstraintTypeClause>(propagatingConstraintRef);
        int negativeCoefficients = 0;
        for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
            const ClauseConstraintLiteral& literal = constraint.getLiteral(i);
            Variable literalVariable = literal.getVariable();
            coefficients[literalVariable] = literal.getCoefficient();
            if (literal.getCoefficient() < 0) negativeCoefficients ++;
            d_conflictVariables[literalVariable] += bumpValue;
        }
        constant = (int)constraint.getConstant() - negativeCoefficients;
        break;
    }
    case ConstraintTypeInteger: {
            const IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(propagatingConstraintRef);
            for(unsigned i = 0, i_end = constraint.getSize(); i < i_end; ++ i) {
                const IntegerConstraintLiteral& literal = constraint.getLiteral(i);
                Variable literalVariable = literal.getVariable();
                coefficients[literalVariable] = literal.getCoefficient();
                d_conflictVariables[literalVariable] += bumpValue;
            }
            constant = constraint.getConstant();
            break;
        }
    default:
    	assert(false);
    }
}

void Solver::resolveCoefficientMaps(Variable var,
        const constraint_coefficient_map& lowerBoundCoefficients, const Integer& cLower,
        const constraint_coefficient_map& upperBoundCoefficients, const Integer& cUpper,
        constraint_coefficient_map& resultCoefficients, Integer& resultConstant) {

    resultCoefficients.clear();

    // Now we have
    // I1: a1 x + p >= c1
    // I2: a2 x + q >= c2
    // a1 > 0, a2 < 0
    // We resolve them as usual -a2 I1 + a1 I2

    assert(lowerBoundCoefficients.find(var) != lowerBoundCoefficients.end());
    assert(NumberUtils<Rational>::getDenominator(lowerBoundCoefficients.find(var)->second) == 1);
    Integer aLower = NumberUtils<Rational>::getNumerator(lowerBoundCoefficients.find(var)->second);
    assert(upperBoundCoefficients.find(var) != upperBoundCoefficients.end());
    assert(NumberUtils<Rational>::getDenominator(upperBoundCoefficients.find(var)->second) == 1);
    Integer aUpper = NumberUtils<Rational>::getNumerator(upperBoundCoefficients.find(var)->second);

    bool boolean = true;
    bool bothConstraintsAreClauses =
    		lowerBoundCoefficients.constraintType == ConstraintTypeClause &&
    		upperBoundCoefficients.constraintType == ConstraintTypeClause;

    // Add the coefficients of the lower bound constraint
    constraint_coefficient_map::const_iterator it1 = lowerBoundCoefficients.begin();
    constraint_coefficient_map::const_iterator it1_end = lowerBoundCoefficients.end();
    for(; it1 != it1_end; ++ it1) {
    	if (!isBoolean(it1->first)) { boolean = false; }
    	resultCoefficients[it1->first] = -aUpper * it1->second;
    }

    // Add the coefficients of the upper bound constraint
    constraint_coefficient_map::const_iterator it2 = upperBoundCoefficients.begin();
    constraint_coefficient_map::const_iterator it2_end = upperBoundCoefficients.end();
    for(; it2 != it2_end; ++ it2) {
    	if (!isBoolean(it2->first)) { boolean = false; }
        resultCoefficients[it2->first] += aLower * it2->second;
    }

    // Remove the zero variables
    std::vector<Variable> canceledVariables;
    constraint_coefficient_map::iterator it = resultCoefficients.begin();
    constraint_coefficient_map::iterator it_end = resultCoefficients.end();
    int negativeCount = 0;
    Integer gcd = 0;
    for(; it != it_end; ++ it) {
        if (it->second == 0) {
            canceledVariables.push_back(it->first);
        } else {
        	// If we know it's a clause, just drop everything to 1
        	if (it->second > 0) {
        		if (bothConstraintsAreClauses) {
        			it->second = 1;
        		}
        	} else {
        		if (bothConstraintsAreClauses) {
        			it->second = -1;
        		}
        		negativeCount ++;
        	}
            if (gcd == 0) {
                gcd = NumberUtils<Integer>::abs(it->second);
            } else {
                gcd = NumberUtils<Integer>::gcd(gcd, it->second);
            }
        }
    }

    // If gcd == 0, everything got canceled
    if (gcd == 0) gcd = 1;

    // Erase the canceled variables and normalize if a clause
    for (unsigned i = 0; i < canceledVariables.size(); ++ i) {
        resultCoefficients.erase(canceledVariables[i]);
    }

    // Divide the constraint with the gcd
    bool cardinality = boolean;
    it = resultCoefficients.begin();
    it_end = resultCoefficients.end();
    for (; it != it_end; ++ it) {
        assert(it->second != 0);
        assert(NumberUtils<Integer>::divides(gcd, it->second));
        it->second = NumberUtils<Integer>::divideDown(it->second, gcd);
        if (cardinality && NumberUtils<Integer>::abs(it->second) != 1) { cardinality = false; }
    }

    // The constant
    resultConstant = NumberUtils<Integer>::divideUp(-aUpper*cLower + aLower*cUpper, gcd);

    // If it's not a cardinality constraint, just make it a integer constraint
    if (!cardinality) {
    	resultCoefficients.constraintType = ConstraintTypeInteger;
    } else {
    	// If both inputs were clauses, the result is a clause
    	if (bothConstraintsAreClauses || resultConstant == 1 - negativeCount) {
    		resultCoefficients.constraintType = ConstraintTypeClause;
    		// We also update the constant in this case
    		resultConstant = 1 - negativeCount;;
    	} else {
    		// Otherwise we have a cardinality constraint
    		resultCoefficients.constraintType = ConstraintTypeCardinality;
    	}
    }
}

bool Solver::isInConflict(const constraint_coefficient_map& coefficients, const Integer& constant) {

    CUTSAT_TRACE_FN("solver") << "Checking: " << coefficients << ">= " << constant << std::endl;

    // Figure out if the constraint is satisfiable
    Integer constraintSum = 0;
    constraint_coefficient_map::const_iterator it = coefficients.begin();
    constraint_coefficient_map::const_iterator it_end = coefficients.end();
    for(; it != it_end; ++ it) {
        Variable variable = it->first;
        if (it->second > 0) {
            if (d_state.hasUpperBound(variable)) {
                CUTSAT_TRACE("solver") << it->first << " with ub = " << d_state.getUpperBound<TypeInteger>(variable) << std::endl;
                constraintSum += d_state.getUpperBound<TypeInteger>(variable) * it->second;
            } else {
                return false;
            }
        } else {
            if (d_state.hasLowerBound(variable)) {
                CUTSAT_TRACE("solver") << it->first << " with lb = " << d_state.getLowerBound<TypeInteger>(variable) << std::endl;
                constraintSum += d_state.getLowerBound<TypeInteger>(variable) * it->second;
            } else {
                return false;
            }
        }
    }

    CUTSAT_TRACE("solver") << "Sum:" << constraintSum << std::endl << constant << std::endl;

    // Otherwise just check for the conflict
    return constraintSum < constant;
}

void Solver::getTopTrailInfo(const constraint_coefficient_map& coefficients, Variable& topVariable, int& topTrailIndex) {

    // We need to get the trail info where this constraint can propagate some improvement, which is tricky
	// If the propagator for this kind of constraint is complete, then it would be nice to be able to not
	// repropagate if we backtrack even further. This is a hard problem in the non-Boolean case, as a single
	// constraint can propagate many times. Hence, it seems reasonable to require that the propagator repropagates
	// constraints it got attached on highter leveles on backtracks.
    topTrailIndex = -1;
    topVariable = VariableNull;
    constraint_coefficient_map::const_iterator it = coefficients.begin();
    constraint_coefficient_map::const_iterator it_end = coefficients.end();
    for(; it != it_end; ++ it) {

        Variable variable = it->first;
        assert(it->second != 0);

        // Get the index without the actual decision
        int trailIndex = 0;
        switch (d_state.getCurrentValueStatus(variable)) {
        case ValueStatusAssignedToLower:
            trailIndex = d_state.getUpperBoundTrailIndex(variable) - 1;
            if (trailIndex >= topTrailIndex) {
                topTrailIndex = trailIndex;
                topVariable = variable;
            }
            break;
        case ValueStatusAssignedToUpper:
            trailIndex = d_state.getLowerBoundTrailIndex(variable) - 1;
            if (trailIndex >= topTrailIndex) {
                topTrailIndex = trailIndex;
                topVariable = variable;
            }
            break;
        default:
            trailIndex = d_state.getLastModificationTrailIndex<true>(variable);
            if (trailIndex > topTrailIndex) {
                topTrailIndex = trailIndex;
                topVariable = variable;
            }
        }
    }
}
