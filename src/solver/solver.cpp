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
#include "solver/solver.h"

#include "propagators/propagators.h"

#include <ext/pb_ds/priority_queue.hpp>
#include <sstream>
#include <fstream>

#include <cstdlib>
#include <coin/CoinMpsIO.hpp>

using namespace std;
using namespace cutsat;

TraceTag solver("solver");

Solver::Solver(ConstraintManager& cm)
: d_cm(cm),
  d_restartHeuristic(d_solverStats),
  d_explanationRemovalHeuristic(d_solverStats),
  d_constraintHeuristicIncrease(1),
  d_constraintHeuristicDecay(1.001),
  d_propagationTrailIndex(0),
  d_status(Unknown),
  d_state(cm),
  d_propagators(d_cm, d_state),
  d_checkModel(true),
  d_disablePropagation(false),
  d_outputCuts(false),
  d_verbosity(VERBOSITY_NO_OUTPUT),
  d_boundEstimate(0),
  d_defaultBound(-1),
  d_replaceVarsWithSlacks(false),
  d_tryFourierMotzkin(false)
{
}

SolverStatus Solver::solve() {
    CUTSAT_TRACE_FN("solver");

    // Start the timer
    d_solverStats.timer.restart();

    // Set the initial trail index
    d_initialTrailIndex = d_state.getTrailSize() - 1;

    // If we already have a state, return it
    if (d_status != Unknown) {
        return d_status;
    }

    // Go through all the unbounded variables and add the slack
    map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
    map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
    for (; var_it != var_it_end; ++ var_it) {
        Variable var = var_it->second;
        if (!d_state.hasLowerBound(var) || !d_state.hasUpperBound(var)) {
            addSlackVariableBound(var);
        }
    }

	if (d_verbosity >= VERBOSITY_BASIC_INFO) {
		if (!d_state.isDynamicOrderOn()) {
			cout << "Using linear order: ";
			vector<Variable> order;
			d_state.getLinearOrder(order);
			for(unsigned i = 0, i_end = order.size(); i < i_end; ++ i) {
				cout << d_state.getVariableName(order[i]) << " ";
			}
			cout << endl;
		}
	}

    // Do the search with restarts
    while (d_status == Unknown) {
        d_status = search();
        // Update the memory stats
        d_solverStats.constraintManagerCapacity = d_cm.getCapacity();
        d_solverStats.constraintManagerSize = d_cm.getSize();
        d_solverStats.constraintManagerWasted = d_cm.getWasted();
        // Print the stats as necessary
        if (d_verbosity >= VERBOSITY_BASIC_INFO) {
            cout << "--------------------------------------------------------------" << endl;
            cout << d_solverStats << endl;
            if (d_verbosity >= VERBOSITY_EXTREME) {
            	d_state.printHeuristic(cout);
            }
        }
        // Do the restart stuff
        ++ d_solverStats.restarts;
        d_restartHeuristic.restart(); d_explanationRemovalHeuristic.restart();
    }

    if (d_status != Satisfiable) {
        // Undo the trail
        backtrack(-1);
    } else {
    	if (d_checkModel) {
    		checkModel();
    	}
    }

    // Return the result
    return d_status;
}

SolverStatus Solver::search() {

    assert(d_status == Unknown);

    while (d_status != Unsatisfiable) {

        // Propagate as much as possible
    	propagate();

        // If in conflict, analyze  it
        if (d_state.inConflict())
        {
            // We have a new conflict
            d_solverStats.conflicts ++;
            d_restartHeuristic.conflict(); d_explanationRemovalHeuristic.conflict();

            // If we are at decision level 0, we are done
            if (d_state.isSafe()) {
            	if (d_verbosity >= VERBOSITY_BASIC_INFO) {
            		cout << "Conflict at level 0!" << std::endl;
            	}
            	return Unsatisfiable;
            }
            if (d_variablesToTrace.size() > 0) {
            	bool any = false;
            	for (unsigned i = 0; i < d_variablesToTrace.size(); ++ i) {
            		if (d_state.isAssigned(d_variablesToTrace[i])) {
            			if (i > 0) { cout << ","; }
            			any = true;
            			cout << d_state.getVariableName(d_variablesToTrace[i]) << ":";
            			d_state.printLowerBound(cout, d_variablesToTrace[i], d_state.getTrailSize());
            		}
            	}
            	if (any) {
            		cout << endl;
            	}
            }
            // Analyze the conflict
            analyzeConflict();
            // If not in conflict adapt to the new situation
            if (d_status != Unsatisfiable) {
            	// Repropagate the constraints that might still be propagating after a backtrack
            	d_propagators.repropagate();
            	// Decay the activity based heuristics
            	decayActivities();
            }
        }
        else
        {
            // Restart if needed
            if (d_restartHeuristic.decide()) {
                backtrack(d_state.getSafeIndex());
                d_propagators.repropagate();
                return Unknown;
            }
            // Derive new cuts
            if (false) {
                generateCuts();
            }
            // Simplify the set of problem constraints
            if (d_state.getTrail().getDecisionLevel() == 0) {
                simplifyConstraintDatabase();
                if (d_status != Unknown) return d_status;
            }
            // Reduce the database
            if (d_explanationRemovalHeuristic.decide()) {
                reduceConstraintDatabase();
            }
            // Select the next variable and branch on it (returns the ones assigned by propagation too)
            Variable decisionVar;
            if (d_slackVariable != VariableNull && !d_state.isAssigned(d_slackVariable)) {
                decisionVar = d_slackVariable;
            } else {
                decisionVar = d_state.decideVariable();
            }
            if (decisionVar == VariableNull) {
                // All variables are assigned
                return Satisfiable;
            } else {
                // Compute the bounds
                computeBounds(decisionVar);
            	// If the variable has no bounds, we need to introduce them
				if (!d_state.hasLowerBound(decisionVar) && !d_state.hasUpperBound(decisionVar)) {
                    addSlackVariableBound(decisionVar);
                    computeBounds(decisionVar);
				}
            	// If we are in conflict, we continue with conflict analysis
            	// Or if the value was assigned by bound computation (lb == ub)
            	if (d_state.inConflict() || d_state.isAssigned(decisionVar)) {
            		continue;
            	}
            	// Set the phase based on the size of the watchlist (if not a boolean variable)
            	if (!isBoolean(decisionVar)) {
            		bool phase = d_cm.getOccuranceCount(decisionVar, false) >= d_cm.getOccuranceCount(decisionVar, true);
            		d_state.setPhase(decisionVar, phase);
            	}
            	// We now decision on the value
                d_solverStats.decisions ++;
            	d_state.decideValue(decisionVar);
            }
        }
    }

    return d_status;
}

void Solver::attachConstraint(ConstraintRef constraintRef, ConstraintClass constraintClass) {

	CUTSAT_TRACE_FN("solver") << "Attaching: " << ConstraintManager::getType(constraintRef) << endl;

    // Add the constraint to the appropriate list
    switch (constraintClass) {
    case CONSTRAINT_CLASS_PROBLEM:
        d_problemConstraints.push_back(constraintRef);
        d_solverStats.problemConstraints ++;
        break;
    case CONSTRAINT_CLASS_EXPLANATION:
        d_explanationConstraints.push_back(constraintRef);
        d_solverStats.explanationConstraints ++;
        break;
    case CONSTRAINT_CLASS_GLOBAL_CUT:
        d_globalCutConstraints.push_back(constraintRef);
        d_solverStats.globalCutConstraints ++;
        break;
    default:
        assert(false);
    }

    // Add the watches for this constraint
    switch(ConstraintManager::getType(constraintRef)) {
    case ConstraintTypeClause:
        d_solverStats.clauseConstraints ++;
    	d_propagators.attachConstraint<ConstraintTypeClause>(constraintRef);
    	break;
    case ConstraintTypeCardinality:
    	d_solverStats.cardinalityConstraints ++;
    	d_propagators.attachConstraint<ConstraintTypeCardinality>(constraintRef);
    	break;
    case ConstraintTypeInteger:
        d_solverStats.integerConstraints ++;
    	d_propagators.attachConstraint<ConstraintTypeInteger>(constraintRef);
    	break;
    default:
    	assert(false);
    }
}

void Solver::generateCuts() {

}

/**
 * Conflict info is used for conflict analysis. It states that we must eliminate the non-divisibeleness of the
 * coefficient by using the reason for this trail index element (variable propagation).
 */
struct ConflictInfo {
	unsigned trailIndex;
	Integer coefficient;
	bool operator < (const ConflictInfo& other) {
		return trailIndex < other.trailIndex;
	}
};

void Solver::decayActivities() {
    d_state.decayActivities();
    d_constraintHeuristicIncrease *= d_constraintHeuristicDecay;
}

ConstraintRef Solver::assertClauseConstraint(vector<ClauseConstraintLiteral>& literals, ConstraintClass constraintClass) {
    CUTSAT_TRACE_FN("solver") << literals << endl;

    ConstraintRef constraint = ConstraintManager::NullConstraint;

    if (d_status == Unknown && !d_state.inConflict()) {

    	// Compute the constant
    	int constant = 1;
    	for (unsigned i = 0; i < literals.size(); ++ i) {
    		if (literals[i].isNegated()) {
    			constant -= 1;
    		}
    	}

        // Normalize the constraint (wrt to the current 0 level assignment)
        PreprocessStatus preprocess = d_propagators.preprocess<ConstraintTypeClause>(literals, constant, d_state.getSafeIndex());

    	switch (preprocess) {
        case PREPROCESS_OK:
            if (literals.size() > 1) {
            	// Create the constraint
                constraint = d_cm.newConstraint<ConstraintTypeClause>(literals, constant, constraintClass != CONSTRAINT_CLASS_PROBLEM);
                // If we're outputting cuts, print the problem
                if (d_outputCuts && constraintClass == CONSTRAINT_CLASS_EXPLANATION) {
                    stringstream filename;
                    filename << "cutsat_proof_" << d_solverStats.conflicts << ".smt";
                    ofstream output(filename.str().c_str());
                    printProblem(output, OutputFormatCnf, constraint);
                }
                // Attach the constraint watchers
                attachConstraint(constraint, constraintClass);
            } else {
            	if (literals.size() == 1) {
            		int coefficient = literals[0].getCoefficient();
            		Variable var = literals[0].getVariable();
                    if (coefficient < 0) {
                    	// Asserting x = 0
                    	if (d_state.getUpperBound<TypeInteger>(var) == 1) {
                            setUpperBound<TypeInteger>(var, 0);
                        }
                    } else {
                    	// Asserting x = 1
                    	if (d_state.getLowerBound<TypeInteger>(var) == 0) {
                            setLowerBound<TypeInteger>(var, 1);
                        }
                    }
            		if (d_verbosity >= VERBOSITY_DETAILED) {
            			cout << "Adding " << constraintClass << ": Clause["
            					<< literals[0].getCoefficient() << "*" << d_state.getVariableName(literals[0].getVariable()) << " >= " << constant
            					<< "]" << std::endl;
            		}
            	}
            }
            propagate();
            break;
        case PREPROCESS_TAUTOLOGY:
        	break;
        case PREPROCESS_INCONSISTENT:
       		d_status = Unsatisfiable;
            break;
        default:
            assert(false);
        }
    }

    return constraint;
}

ConstraintRef Solver::assertCardinalityConstraint(vector<CardinalityConstraintLiteral>& literals, unsigned& c, ConstraintClass constraintClass) {
    CUTSAT_TRACE_FN("solver") << literals << " : " << c << endl;

    ConstraintRef constraint = ConstraintManager::NullConstraint;

    if (d_status == Unknown && !d_state.inConflict()) {

        // Normalize the constraint (wrt to the current 0 level assignment)
        PreprocessStatus preprocess = d_propagators.preprocess<ConstraintTypeCardinality>(literals, c, d_state.getSafeIndex());

    	switch (preprocess) {
        case PREPROCESS_OK:
        	// Create the constraint
            constraint = d_cm.newConstraint<ConstraintTypeCardinality>(literals, c, constraintClass != CONSTRAINT_CLASS_PROBLEM);
            // If we're outputting cuts, print the problem
            if (d_outputCuts && constraintClass == CONSTRAINT_CLASS_EXPLANATION) {
            	stringstream filename;
                filename << "cutsat_proof_" << d_solverStats.conflicts << ".smt";
                ofstream output(filename.str().c_str());
                printProblem(output, OutputFormatCnf, constraint);
            }
            // Attach the constraint watchers
            attachConstraint(constraint, constraintClass);
            // Propagate
            propagate();
            break;
        case PREPROCESS_TAUTOLOGY:
        	break;
        case PREPROCESS_INCONSISTENT:
       		d_status = Unsatisfiable;
            break;
        default:
            assert(false);
        }
    }

    return constraint;
}

ConstraintRef Solver::assertIntegerConstraint(vector<IntegerConstraintLiteral>& literals, Integer& c, ConstraintClass constraintClass) {
    CUTSAT_TRACE_FN("solver") << literals << "," << c << endl;

    ConstraintRef constraint = ConstraintManager::NullConstraint;

    if (d_status == Unknown && !d_state.inConflict()) {

    	// If we are adding slack, replace them all
    	if (d_replaceVarsWithSlacks && constraintClass == CONSTRAINT_CLASS_PROBLEM) {
    		for (unsigned i = 0, i_end = literals.size(); i < i_end; ++ i) {
    			Variable variable = literals[i].getVariable();
    			Integer coefficient = literals[i].getCoefficient();
    			// Replacing x with x+ - x-
    			literals[i] = IntegerConstraintLiteral(coefficient, d_variableToPositiveSlack[variable]);
    			literals.push_back(IntegerConstraintLiteral(-coefficient, d_variableToNegativeSlack[variable]));
    		}
    	}

        // Normalize the constraint (wrt to the current 0 level assignment)
        PreprocessStatus preprocess = d_propagators.preprocess<ConstraintTypeInteger>(literals, c, d_state.getSafeIndex());

        switch (preprocess) {
        case PREPROCESS_OK:
            if (literals.size() > 1) {
            	// Create the constraint
                constraint = d_cm.newConstraint<ConstraintTypeInteger>(literals, c, constraintClass != CONSTRAINT_CLASS_PROBLEM);
                // If we're outputting cuts, print the problem
                if (d_outputCuts && constraintClass == CONSTRAINT_CLASS_EXPLANATION) {
                    stringstream filename;
                    filename << "cutsat_proof_" << d_solverStats.conflicts << ".smt";
                    ofstream output(filename.str().c_str());
                    printProblem(output, OutputFormatCnf, constraint);
                }
                // Attach the constraint watchers
                attachConstraint(constraint, constraintClass);
            } else {
            	if (literals.size() == 1) {
            		Integer coefficient = literals[0].getCoefficient();
            		Variable var = literals[0].getVariable();
                    if (coefficient < 0) {
                        Integer bound = NumberUtils<Integer>::divideDown(c, coefficient);
                    	if (!d_state.hasUpperBound(var) || bound < d_state.getUpperBound<TypeInteger>(var)) {
                            setUpperBound<TypeInteger>(var, bound);
                        }
                    } else {
                        Integer bound = NumberUtils<Integer>::divideUp(c, coefficient);
                        if (!d_state.hasLowerBound(var) || bound > d_state.getLowerBound<TypeInteger>(var)) {
                            setLowerBound<TypeInteger>(var, bound);
                        }
                    }
            		if (d_verbosity >= VERBOSITY_DETAILED) {
            			cout << "Adding " << constraintClass << ": Integer["
            					<< literals[0].getCoefficient() << "*" << d_state.getVariableName(literals[0].getVariable()) << " >= " << c
            					<< "]" << std::endl;
            		}
            	}
            }
            propagate();
            break;
        case PREPROCESS_TAUTOLOGY:
        	break;
        case PREPROCESS_INCONSISTENT:
       		d_status = Unsatisfiable;
            break;
        default:
            assert(false);
        }
    }

    return constraint;
}

Variable Solver::newVariable(VariableType type, const char* varNameInput) {

    string varName = varNameInput;
    // Sanitize
    replace(varName.begin(), varName.end(), ',', '_');

    CUTSAT_TRACE("solver") << "newVariable(" << type << "," << varName << ")" << endl;

    Variable var = d_cm.newVariable(type);
    unsigned varId = var.getId();
    d_solverStats.variables ++;

    CUTSAT_TRACE("solver") << "newVariable() => " << var << endl;

    d_state.newVariable(var, varName.c_str(), !d_replaceVarsWithSlacks);

    if (!d_replaceVarsWithSlacks) {
    	d_variableNameToVariable[varName] = var;
    	d_propagators.addVariable(var);
    } else {
    	// Introduce the positive slack
    	stringstream x_plusName;
    	x_plusName << varName << "_plus";
    	Variable x_plusVar = d_cm.newVariable(type);
    	d_state.newVariable(x_plusVar, x_plusName.str().c_str(), true);
    	d_variableNameToVariable[x_plusName.str()] = x_plusVar;
    	d_propagators.addVariable(x_plusVar);
    	d_variableToPositiveSlack[var] = x_plusVar;
    	d_state.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x_plusVar, 0, ConstraintManager::NullConstraint);

    	// Introduce the negative slack
    	stringstream x_minusName;
    	x_minusName << varName << "_minus";
    	Variable x_minusVar = d_cm.newVariable(type);
    	d_state.newVariable(x_minusVar, x_minusName.str().c_str(), true);
    	d_variableNameToVariable[x_minusName.str()] = x_minusVar;
    	d_propagators.addVariable(x_minusVar);
    	d_variableToNegativeSlack[var] = x_minusVar;
    	d_state.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(x_minusVar, 0, ConstraintManager::NullConstraint);

    	varId = x_minusVar.getId();
    }

    if (d_slackConstraintsLower.size() <= varId) {
    	d_slackConstraintsLower.resize(varId + 1, ConstraintManager::NullConstraint);
    	d_slackConstraintsUpper.resize(varId + 1, ConstraintManager::NullConstraint);
    }

    if (d_defaultBound >= 0) {
    	d_state.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(var, -d_defaultBound, ConstraintManager::NullConstraint);
    	d_state.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, TypeInteger>(var, d_defaultBound, ConstraintManager::NullConstraint);
    }

    d_initialTrailIndex = d_state.getTrailSize() - 1;

	return var;
}

void Solver::propagate()
{
    // If propagation is disabled, just return
    if (d_disablePropagation || d_state.inConflict()) {
    	return;
    }

    CUTSAT_TRACE_FN("solver");

    // Get the trail
    const SearchTrail& trail = d_state.getTrail();

    // Propagate until everything done, or a conflict has been found
    for(; d_propagationTrailIndex < trail.getSize(); ++ d_propagationTrailIndex) {

        // Find the event
        const TrailElement& event = trail[d_propagationTrailIndex];

        // Propagate
        switch (event.modificationType) {
        case MODIFICATION_LOWER_BOUND_REFINE:
            d_propagators.propagateEvent<MODIFICATION_LOWER_BOUND_REFINE>(event.var);
            break;
        case MODIFICATION_UPPER_BOUND_REFINE:
            d_propagators.propagateEvent<MODIFICATION_UPPER_BOUND_REFINE>(event.var);
            break;
        default:
            assert(false);
        }

        // If we have a conflict, setup the constraints for conflict analysis
        if (d_state.inConflict()) {
            return;
        }
    }
}


void Solver::backtrack(int backtrackIndex) {

  CUTSAT_TRACE_FN("solver") << "Backtracking to index " << backtrackIndex
      << std::endl;

  if (d_verbosity >= VERBOSITY_EXTREME) {
    cout << "Backtracking to trail index " << backtrackIndex << std::endl;
  }

  // Undo the state
  assert(backtrackIndex >= d_state.getSafeIndex());
  d_state.cancelUntil(backtrackIndex);

  // Update the propagation index to the size of the trail
  d_propagationTrailIndex = std::min(d_propagationTrailIndex, (unsigned) d_state.getTrail().getSize());

  // Clean the tight constraint cache above this index
  prop_variable_tag var_tag(VariableNull, backtrackIndex, MODIFICATION_COUNT);
  d_tightConstraintCache.erase(d_tightConstraintCache.upper_bound(var_tag), d_tightConstraintCache.end());

  // Backtrack the propagators
  d_propagators.cancelUntil(backtrackIndex);
}

void Solver::checkModel() {
	CUTSAT_TRACE_FN("solver");

	assert(d_status == Satisfiable);

	// Check the constraints and single variable bounds
	bool ok = true;
	unsigned i;
	for (i = 0; i < d_problemConstraints.size(); ++ i) {
		ConstraintRef constraint = d_problemConstraints[i];
		switch(ConstraintManager::getType(constraint)) {
		case ConstraintTypeClause:
			if (!d_cm.get<ConstraintTypeClause>(constraint).isSatisfied(d_state)) {
				ok = false;
				CUTSAT_TRACE("solver") << "Unsat: " << d_cm.get<ConstraintTypeClause>(constraint) << std::endl;
				if (d_verbosity >= VERBOSITY_BASIC_INFO) {
					cout << "Constraint not satisfied: ";
					printConstraint<ConstraintTypeClause>(d_cm.get<ConstraintTypeClause>(constraint), cout, OutputFormatIlp);
					cout << std::endl;
				}
			}
			break;
		case ConstraintTypeCardinality:
			if (!d_cm.get<ConstraintTypeCardinality>(constraint).isSatisfied(d_state)) {
				ok = false;
				CUTSAT_TRACE("solver") << "Unsat: " << d_cm.get<ConstraintTypeCardinality>(constraint) << std::endl;
				if (d_verbosity >= VERBOSITY_BASIC_INFO) {
					cout << "Constraint not satisfied: ";
					printConstraint<ConstraintTypeCardinality>(d_cm.get<ConstraintTypeCardinality>(constraint), cout, OutputFormatIlp);
					cout << std::endl;
				}
			}
			break;
		case ConstraintTypeInteger:
			if (!d_cm.get<ConstraintTypeInteger>(constraint).isSatisfied(d_state)) {
				ok = false;
				CUTSAT_TRACE("solver") << "Unsat: " << d_cm.get<ConstraintTypeInteger>(constraint) << std::endl;
				if (d_verbosity >= VERBOSITY_BASIC_INFO) {
					cout << "Constraint not satisfied: ";
					printConstraint<ConstraintTypeInteger>(d_cm.get<ConstraintTypeInteger>(constraint), cout, OutputFormatIlp);
					cout << std::endl;
				}
			}
			break;
		default:
			assert(false);
		}
	}

	if (ok && d_verbosity >= VERBOSITY_BASIC_INFO) {
		cout << "All constraints satisfied" << std::endl;
	}

	assert(ok);
}

void Solver::computeBounds(Variable variable) {
	// Bound the given variable
	CUTSAT_TRACE_FN("solver") << "Bounding " << variable << std::endl;
	// Bound the variable for the incomplete propagators
	d_propagators.bound(variable);
}


void Solver::printConstraintSMT(const std::vector<IntegerConstraintLiteral>& literals, const Integer& c, std::ostream& out) {
    out << "(>= ";
    if (literals.size() > 1) out << "(+ ";
    for(unsigned i = 0, i_end = literals.size(); i < i_end; ++ i) {
        const Integer& coefficient = literals[i].getCoefficient();
        Variable variable = literals[i].getVariable();
        if (coefficient < 0) {
            out << "(* (~ " << -coefficient << ") " << d_state.getVariableName(variable) << ") ";
        } else {
            out << "(* " << coefficient << " " << d_state.getVariableName(variable) << ") ";
        }
    }
    if (literals.size() > 1) out << ") ";
    if (c < 0) out << "(~ " << -c << ")";
    else out << c;
    out << ")";
}

void Solver::addSlackVariableBound(Variable var) {

    assert(!d_state.hasLowerBound(var) || !d_state.hasUpperBound(var));

    Integer zero;
    std::vector<IntegerConstraintLiteral> literals;

    if (d_slackVariable == VariableNull) {
        d_slackVariable = newVariable(TypeInteger, "slack");
    }

    if (d_slackConstraintsLower[var.getId()] == ConstraintManager::NullConstraint) {

    	// Add the lower bound constraint (x >= -slack) x + slack >= 0
		zero = 0;
		literals.clear();
		literals.push_back(IntegerConstraintLiteral(1, var));
		literals.push_back(IntegerConstraintLiteral(1, d_slackVariable));
		d_slackConstraintsLower[var.getId()] = assertIntegerConstraint(literals, zero, CONSTRAINT_CLASS_PROBLEM);

		// Add the upper bound constraint (x <= slack) slack -x >= 0
		zero = 0;
		literals.clear();
		literals.push_back(IntegerConstraintLiteral(-1, var));
		literals.push_back(IntegerConstraintLiteral(1, d_slackVariable));
		d_slackConstraintsUpper[var.getId()] = assertIntegerConstraint(literals, zero, CONSTRAINT_CLASS_PROBLEM);
    }

    // Assert the lower bound to 0
    if (!d_state.hasLowerBound(d_slackVariable)) {
        d_state.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, TypeInteger>(d_slackVariable, d_boundEstimate, ConstraintManager::NullConstraint);
    }

    if (!d_state.isAssigned(d_slackVariable)) {
        // Decide the upper bound to 0
        d_state.decideValue(d_slackVariable);
    }

    // And now we know that the lower and upper bound of this (x >= -slack), x <= slack), but we still need to check
    // other bounds, we might have some other stuff around with slack inside
    if (d_verbosity >= VERBOSITY_BASIC_INFO) {
		cout << "Adding slack variable for variable " << d_state.getVariableName(var) << std::endl;
	}
}

struct ConstraintCmpByScore {
    const ConstraintManager& d_cm;
    ConstraintCmpByScore(const ConstraintManager& cm)
    : d_cm(cm) {}
    bool operator () (ConstraintRef c1Ref, ConstraintRef c2Ref) const {
        const IntegerConstraint& c1 = d_cm.get<ConstraintTypeInteger>(c1Ref);
        const IntegerConstraint& c2 = d_cm.get<ConstraintTypeInteger>(c2Ref);

        return c1.getScore() < c2.getScore();
    }
};

void Solver::reduceConstraintDatabase() {

    CUTSAT_TRACE_FN("solver") << d_solverStats << std::endl;

    ConstraintCmpByScore cmp(d_cm);
    std::sort(d_explanationConstraints.begin(), d_explanationConstraints.end(), cmp);
    unsigned j = 0;
    unsigned size = d_explanationConstraints.size();
    unsigned halfSize = size / 2;
    for(unsigned i = 0; i < size; ++ i) {
        ConstraintRef constraintRef = d_explanationConstraints[i];
        IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(constraintRef);
        if (constraint.isDeleted()) {
        	continue;
        }
        if (constraint.inUse()) {
        	d_explanationConstraints[j++] = constraintRef;
        	continue;
        }
        if (i < halfSize) {
        	removeConstraint(constraintRef, CONSTRAINT_CLASS_EXPLANATION);
        } else {
            d_explanationConstraints[j++] = constraintRef;
        }
    }

    CUTSAT_TRACE("solver") << "removed " << size -j << " constraints." << endl;

    d_solverStats.removedConstraints += (size - j);
    d_explanationConstraints.resize(j);

    if (d_cm.getWasted() > 0.5*d_cm.getSize()) {
    	collectGarbage();
    }
}

void Solver::removeConstraint(ConstraintRef constraintRef, ConstraintClass constraintClass) {

    assert(!d_cm.get<ConstraintTypeInteger>(constraintRef).inUse());

    CUTSAT_TRACE_FN("solver") << "Removing: " << constraintRef << std::endl;

    switch (constraintClass) {
        case CONSTRAINT_CLASS_PROBLEM:
            d_solverStats.problemConstraints --;
            break;
        case CONSTRAINT_CLASS_EXPLANATION:
        	assert(d_solverStats.explanationConstraints > 0);
            d_solverStats.explanationConstraints --;
            break;
        case CONSTRAINT_CLASS_GLOBAL_CUT:
            d_solverStats.globalCutConstraints --;
            break;
        default:
            assert(false);
    }

    // Detach from the propagators
    switch(ConstraintManager::getType(constraintRef)) {
    case ConstraintTypeClause:
        d_solverStats.clauseConstraints --;
    	d_propagators.removeConstraint<ConstraintTypeClause>(constraintRef);
        d_cm.eraseConstraint<ConstraintTypeClause>(constraintRef);
    	break;
    case ConstraintTypeCardinality:
        d_solverStats.cardinalityConstraints --;
        d_propagators.removeConstraint<ConstraintTypeCardinality>(constraintRef);
        d_cm.eraseConstraint<ConstraintTypeCardinality>(constraintRef);
        break;
    case ConstraintTypeInteger:
        d_solverStats.integerConstraints --;
    	d_propagators.removeConstraint<ConstraintTypeInteger>(constraintRef);
        d_cm.eraseConstraint<ConstraintTypeInteger>(constraintRef);
    	break;
    default:
    	assert(false);
    }
}

void Solver::printProblemSmt(std::ostream& output, ConstraintRef implied) const {
  // Initialize the SMT output
  output << "(benchmark cutsat" << endl;
  output << ":logic QF_LIA" << endl;

  // Add all the variables
  stringstream bounds;
  map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
  map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
  for (; var_it != var_it_end; ++ var_it) {
      string variableName = var_it->first;
      Variable variable = var_it->second;
      output << ":extrafuns ((" << variableName << " Int" << "))" << endl;
      if (d_initialTrailIndex >= 0 && d_state.hasLowerBound(variable, d_initialTrailIndex)) {
          Integer bound = d_state.getLowerBound<TypeInteger>(variable, d_initialTrailIndex);
          if (bound >= 0) {
                  bounds << ":assumption (>= " << variableName << " " << bound << ")" << endl;
          } else {
                  bounds << ":assumption (>= " << variableName << " (~ " << -bound << "))" << endl;
          }
      }
      if (d_initialTrailIndex >= 0 && d_state.hasUpperBound(variable, d_initialTrailIndex)) {
          Integer bound = d_state.getUpperBound<TypeInteger>(variable, d_initialTrailIndex);
          if (bound >= 0) {
                  bounds << ":assumption (<= " << variableName << " " << bound << ")" << endl;
          } else {
                  bounds << ":assumption (<= " << variableName << " (~ " << -bound << "))" << endl;
          }
      }
  }

  // Output the bounds
  output << bounds.str() << endl;

  // Output the constraints
  for (unsigned i = 0; i < d_problemConstraints.size(); ++ i) {
          output << ":assumption ";
          switch(ConstraintManager::getType(d_problemConstraints[i])) {
          case ConstraintTypeClause:
              printConstraint(d_cm.get<ConstraintTypeClause>(d_problemConstraints[i]), output, OutputFormatSmt);
              break;
          case ConstraintTypeInteger:
              printConstraint(d_cm.get<ConstraintTypeInteger>(d_problemConstraints[i]), output, OutputFormatSmt);
              break;
          default:
              assert(false);
      }
          output << endl;
  }

  if (implied == ConstraintManager::NullConstraint) {
          output << ":formula true" << endl;
  } else {
          output << ":formula (not ";
      switch(ConstraintManager::getType(implied)) {
          case ConstraintTypeClause:
              printConstraint(d_cm.get<ConstraintTypeClause>(implied), output, OutputFormatSmt);
              break;
          case ConstraintTypeInteger:
              printConstraint(d_cm.get<ConstraintTypeInteger>(implied), output, OutputFormatSmt);
              break;
          default:
              assert(false);
      }
          output << ")" << endl;
  }
  output << ")" << endl;
}

void Solver::printProblemSmt2(std::ostream& output, ConstraintRef implied) const {
  // Initialize the SMT output
  output << "(set-logic QF_UFLIA)" << endl;
  output << "(set-info :smt-lib-version 2.0)" << endl;

  // Add all the variables
  stringstream bounds;
  map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
  map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
  for (; var_it != var_it_end; ++ var_it) {
      string variableName = var_it->first;
      Variable variable = var_it->second;
      output << "(declare-fun " << variableName << " () Int)" << endl;
      if (d_initialTrailIndex >= 0 && d_state.hasLowerBound(variable, d_initialTrailIndex)) {
          Integer bound = d_state.getLowerBound<TypeInteger>(variable, d_initialTrailIndex);
          if (bound >= 0) {
                  bounds << "(assert (>= " << variableName << " " << bound << "))" << endl;
          } else {
                  bounds << "(assert (>= " << variableName << " (- " << -bound << ")))" << endl;
          }
      }
      if (d_initialTrailIndex >= 0 && d_state.hasUpperBound(variable, d_initialTrailIndex)) {
          Integer bound = d_state.getUpperBound<TypeInteger>(variable, d_initialTrailIndex);
          if (bound >= 0) {
                  bounds << "(assert (<= " << variableName << " " << bound << "))" << endl;
          } else {
                  bounds << "(assert (<= " << variableName << " (- " << -bound << ")))" << endl;
          }
      }
  }

  // Output the bounds
  output << bounds.str() << endl;

  // Output the constraints
  for (unsigned i = 0; i < d_problemConstraints.size(); ++ i) {
          output << "(assert ";
          switch(ConstraintManager::getType(d_problemConstraints[i])) {
          case ConstraintTypeClause:
              printConstraint(d_cm.get<ConstraintTypeClause>(d_problemConstraints[i]), output, OutputFormatSmt2);
              break;
          case ConstraintTypeInteger:
              printConstraint(d_cm.get<ConstraintTypeInteger>(d_problemConstraints[i]), output, OutputFormatSmt2);
              break;
          default:
              assert(false);
          }
          output << ")" << endl;
  }

  if (implied != ConstraintManager::NullConstraint) {
          output << "(assert (not ";
      switch(ConstraintManager::getType(implied)) {
          case ConstraintTypeClause:
              printConstraint(d_cm.get<ConstraintTypeClause>(implied), output, OutputFormatSmt2);
              break;
          case ConstraintTypeInteger:
              printConstraint(d_cm.get<ConstraintTypeInteger>(implied), output, OutputFormatSmt2);
              break;
          default:
              assert(false);
      }
          output << "))" << endl;
  }

  output << "(check-sat)" << endl;
}

void Solver::printProblemMps(std::ostream& output, ConstraintRef implied) const {
  // Model we will be creating
  CoinMpsIO model;

  // Add slack variables and count constraints
  unsigned constraintsCount = d_problemConstraints.size();

  // Setup the vectors
  unsigned variablesCount = d_cm.getVariablesCount();

  const char* names[variablesCount];
  const char* ineq_names[constraintsCount];
  double varBoundsLow[variablesCount];
  double varBoundsUp[variablesCount];
  char integrality[variablesCount];
  double rowUb[constraintsCount];
  double rowLb[constraintsCount];
  double objective[variablesCount];

  // Infinity
  const double inf = model.getInfinity();

  // Add all the variables
  map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
  map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
  for (; var_it != var_it_end; ++ var_it) {
      string variableName = var_it->first;
      Variable variable = var_it->second;
      unsigned variableIndex = variable.getId();
      assert(variableIndex < variablesCount);
      double lowerBound = -inf;
      double upperBound = +inf;
      if (d_state.hasLowerBound(variable)) {
          lowerBound = NumberUtils<Integer>::toInt(d_state.getLowerBound<TypeInteger>(variable));
      }
      if (d_state.hasUpperBound(variable)) {
          upperBound = NumberUtils<Integer>::toInt(d_state.getUpperBound<TypeInteger>(variable));
      }
      integrality [variableIndex] = 1;
      objective   [variableIndex] = 0;
      names       [variableIndex] = strdup(variableName.c_str());
      varBoundsLow[variableIndex] = lowerBound;
      varBoundsUp [variableIndex] = upperBound;
  }

  for(unsigned i = 0; i < variablesCount; ++ i) {
      integrality[i] = 1;
  }

  // Add the constraints
  CoinPackedMatrix matrix(false, 0, 0);
  matrix.setDimensions(-1, variablesCount);

  // Define the rows
  for (unsigned i = 0; i < d_problemConstraints.size(); ++ i) {
      const IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(d_problemConstraints[i]);

      int* indices = (int*)calloc(constraint.getSize(), sizeof(int));
      double* elements = (double*)calloc(constraint.getSize(), sizeof(double));

      for(unsigned lit = 0; lit < constraint.getSize(); ++ lit) {
          const IntegerConstraintLiteral& literal = constraint.getLiteral(lit);
          indices[lit] = literal.getVariable().getId();
          elements[lit] = NumberUtils<Integer>::toInt(literal.getCoefficient());
      }

      rowUb[i] = +inf;
      rowLb[i] = NumberUtils<Integer>::toInt(constraint.getConstant());

      char row_name[100];
      sprintf(row_name, "ROW%d", i);
      ineq_names[i] = strdup(row_name);

      matrix.appendRow(constraint.getSize(), indices, elements);

      free(indices);
      free(elements);
  }

  model.setMpsData(matrix, inf, varBoundsLow, varBoundsUp, objective, integrality, rowLb, rowUb, names, ineq_names);

  model.setProblemName("CUTSAT");

  // Create a temporary file and write the problem to it
  char filename[100];
  sprintf(filename, "/tmp/cutsat_XXXXXX");
  if(mkstemp(filename) == -1) {
          throw CutSatException("Could not create temporary file for mps output.");
  }
  model.writeMps(filename);
  // Copy the file to the output stream
  ifstream file(filename);
  string line;
  while (file.good()) {
      getline(file, line);
      output << line << endl;
  }
}

void Solver::printProblemOpb(std::ostream& output, ConstraintRef implied) const {

  // Add slack variables and count constraints
  unsigned constraintsCount = d_problemConstraints.size();
  unsigned variablesCount = d_cm.getVariablesCount();

  // Add all the variables
  stringstream bounds;
  map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
  map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
  for (; var_it != var_it_end; ++ var_it) {
      string variableName = var_it->first;
      Variable variable = var_it->second;
      unsigned variableIndex = variable.getId() + 1;
      Integer lowerBound = 0;
      Integer upperBound = 1;
      if (d_state.hasLowerBound(variable, d_initialTrailIndex)) {
          lowerBound = d_state.getLowerBound<TypeInteger>(variable, d_initialTrailIndex);
      }
      if (d_state.hasUpperBound(variable, d_initialTrailIndex)) {
          upperBound = d_state.getUpperBound<TypeInteger>(variable, d_initialTrailIndex);
      }
      if (lowerBound >= 1) {
          bounds << "+1 x" << variableIndex << " >= " << lowerBound << " ;" << endl;
          constraintsCount ++;
      }
      if (upperBound <= 0) {
          bounds << "-1 x" << variableIndex << " >= " << -lowerBound << " ;" << endl;
          constraintsCount ++;
      }
  }

  cout << "* #variable= " << variablesCount << " #constraint= " << constraintsCount << endl;
  cout << bounds.str();

  // Define the rows
  for (unsigned i = 0; i < d_problemConstraints.size(); ++ i) {
      const IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(d_problemConstraints[i]);
      for(unsigned lit = 0; lit < constraint.getSize(); ++ lit) {
          const IntegerConstraintLiteral& literal = constraint.getLiteral(lit);
          const Integer& coefficient = literal.getCoefficient();
          unsigned variable = literal.getVariable().getId() + 1;
          if (coefficient > 0) {
                  cout << "+" << coefficient << " x" << variable << " ";
          } else {
                  cout << coefficient << " x" << variable << " ";
          }
      }
      cout << ">= " << constraint.getConstant() << " ;" << endl;
  }
}

void Solver::printProblemCnf(std::ostream& output, ConstraintRef implied) const {
  // Add all the non-trivial variable bounds
   stringstream bounds;
   map<string, Variable>::const_iterator var_it = d_variableNameToVariable.begin();
   map<string, Variable>::const_iterator var_it_end = d_variableNameToVariable.end();
   for (; var_it != var_it_end; ++ var_it) {
       Variable variable = var_it->second;
       unsigned variableIndex = variable.getId() + 1;
       Integer lowerBound = d_state.getLowerBound<TypeInteger>(variable, d_initialTrailIndex);
       Integer upperBound = d_state.getUpperBound<TypeInteger>(variable, d_initialTrailIndex);
       if (lowerBound == 1) {
           output << variableIndex << " 0" << endl;
       }
       if (upperBound == 0) {
           bounds << -variableIndex << " 0" << endl;
       }
   }
   // Output the constraints
   for (unsigned i = 0; i < d_problemConstraints.size(); ++ i) {
           switch(ConstraintManager::getType(d_problemConstraints[i])) {
           case ConstraintTypeClause:
               printConstraint(d_cm.get<ConstraintTypeClause>(d_problemConstraints[i]), output, OutputFormatCnf);
               break;
           default:
               assert(false);
       }
           output << endl;
   }
   // Output the negation of the implication
   if (implied != ConstraintManager::NullConstraint) {
           const ClauseConstraint& impliedClause = d_cm.get<ConstraintTypeClause>(implied);
           for (unsigned i = 0; i < impliedClause.getSize(); ++ i) {
                   const ClauseConstraintLiteral& literal = impliedClause.getLiteral(i);
                   if (!literal.isNegated()) { output << literal.getVariable().getId() + 1; }
                   else { output << - (literal.getVariable().getId() + 1); }
                   output << " 0" << endl;
           }
   }
}

void Solver::printProblem(std::ostream& output, OutputFormat format, ConstraintRef implied) const {
  switch (format) {
  case OutputFormatSmt:
    printProblemSmt(output, implied);
    break;
  case OutputFormatSmt2:
    printProblemSmt2(output, implied);
    break;
  case OutputFormatMps:
    printProblemMps(output, implied);
    break;
  case OutputFormatOpb:
    printProblemOpb(output, implied);
    break;
  case OutputFormatCnf:
    printProblemCnf(output, implied);
    break;
  default:
    assert(0);
  }
}

void Solver::collectGarbage() {
	std::map<ConstraintRef, ConstraintRef> reallocMap;

	d_propagators.cleanAll();

	d_cm.gcBegin();
	d_cm.gcMove(d_problemConstraints, reallocMap);
	d_cm.gcMove(d_explanationConstraints, reallocMap);
	d_cm.gcMove(d_globalCutConstraints, reallocMap);
	d_cm.gcEnd();

	d_state.gcUpdate(reallocMap);
    d_propagators.gcUpdate(reallocMap);

    // Update the slack constraints
    if (d_slackVariable != VariableNull) {
    	for (unsigned i = 0, i_end = d_slackConstraintsLower.size(); i < i_end; ++ i) {
    		if (d_slackConstraintsLower[i] != ConstraintManager::NullConstraint) {
    			d_slackConstraintsLower[i] = reallocMap[d_slackConstraintsLower[i]];
    		}
    		if (d_slackConstraintsUpper[i] != ConstraintManager::NullConstraint) {
    			d_slackConstraintsUpper[i] = reallocMap[d_slackConstraintsUpper[i]];
    		}
    	}
    }
}

void Solver::bumpConstraint(ConstraintRef constraintRef) {
    IntegerConstraint& constraint = d_cm.get<ConstraintTypeInteger>(constraintRef);
    if (!constraint.isLearnt()) {
        return;
    }
    double newValue = constraint.getScore() + d_constraintHeuristicIncrease;
    if (newValue > 1e20) {
        for (unsigned i = 0, i_end = d_explanationConstraints.size(); i < i_end; ++ i) {
            IntegerConstraint& explConstraint = d_cm.get<ConstraintTypeInteger>(d_explanationConstraints[i]);
            explConstraint.setScore(explConstraint.getScore() * 1e-20);
        }
        d_constraintHeuristicIncrease *= 1e-20;
    } else {
        constraint.setScore(newValue);
    }
}


void Solver::simplifyConstraintDatabase() {
	assert(d_state.getTrail().getDecisionLevel() == 0);
	// Remove satsified clauses
}

bool Solver::isBoolean(Variable var) const {
	int safeIndex = d_state.getSafeIndex();
	return
          d_state.hasLowerBound(var, safeIndex) &&
          d_state.hasUpperBound(var, safeIndex) &&
          d_state.getLowerBound<TypeInteger>(var, safeIndex) >= 0 &&
          d_state.getUpperBound<TypeInteger>(var, safeIndex) <= 1
    	;
}
