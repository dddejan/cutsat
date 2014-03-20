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

#include <vector>
#include <sstream>

#include "util/config.h"
#include "solver/search_trail.h"
#include "solver/variable_info.h"
#include "solver/solver_state.h"
#include "solver/solver_stats.h"
#include "constraints/constraint_manager.h"
#include "propagators/watch_list_manager.h"

#include "propagators/propagators.h"

#include "heuristics/restart_luby.h"
#include "heuristics/explanation_removal.h"

#include "util/enums.h"

namespace cutsat {

/**
 * The current sate of the solver with respect to solving the problem.
 */
enum SolverStatus {
    /** Solver has not (yet) determined the state of the problem */
    Unknown,
    /** The problem is unsatisfiable */
    Unsatisfiable,
    /** The problem is satisfiable */
    Satisfiable,
    /** Solver has been interrupted */
    Interrupted
};

inline std::ostream& operator << (std::ostream& out, const SolverStatus& status) {
    switch(status) {
    case Unknown:
        out << "unknown";
        break;
    case Unsatisfiable:
        out << "unsat";
        break;
    case Satisfiable:
        out << "sat";
        break;
    case Interrupted:
        out << "interrupted";
        break;
    }
    return out;
}

class Solver {

public:

    struct constraint_coefficient_map {
    	typedef std::map<Variable, Integer> coefficient_map;
    	typedef coefficient_map::const_iterator const_iterator ;
    	typedef coefficient_map::iterator iterator;

    	ConstraintType constraintType;
    	coefficient_map coefficients;

    	Integer& operator [] (Variable var) { return coefficients[var]; }

    	void swap(constraint_coefficient_map& other) {
    		coefficients.swap(other.coefficients);
    		std::swap(constraintType, other.constraintType);
    	}

    	constraint_coefficient_map() : constraintType(ConstraintTypeLast) {}

    	const_iterator begin() const { return coefficients.begin(); }
    	const_iterator end() const { return coefficients.end(); }
    	iterator begin() { return coefficients.begin(); }
    	iterator end() { return coefficients.end(); }
    	const_iterator find(Variable var) const { return coefficients.find(var); }
    	void erase(Variable var) { coefficients.erase(var); }

    	void clear() { coefficients.clear(); constraintType = ConstraintTypeLast; }
    };

private:

    /** The constraint manager managing to manage the problem constraints */
    ConstraintManager& d_cm;

    /** All the solver statistics in one place */
    SolverStats d_solverStats;

    /** The Luby restart heuristic */
    LubyRestartHeuristic d_restartHeuristic;

    /** The clause cleanup heuristic */
    ExplanationRemovalHeuristic d_explanationRemovalHeuristic;

    /** The type of each constraint database */
    typedef std::vector<ConstraintRef> ConstraintDB;

    /** The original problem constraints */
    ConstraintDB d_problemConstraints;
    /** Constraints derived as explanations of conflicts */
    ConstraintDB d_explanationConstraints;
    /** Constraints derived as global cuts */
    ConstraintDB d_globalCutConstraints;

    /** How much to increase the variable score per bump */
    double d_constraintHeuristicIncrease;
    /** Decay factor for the variable scores */
    double d_constraintHeuristicDecay;

    /** Bidirectional map from variables to their names */
    std::map<std::string, Variable> d_variableNameToVariable;

    std::map<Variable, Variable> d_variableToPositiveSlack;
    std::map<Variable, Variable> d_variableToNegativeSlack;

    /** Index into the trail, to know what we've propagated so far */
    unsigned d_propagationTrailIndex;

    /** Initial index when we started solving */
    int d_initialTrailIndex;

    /**
     * Attaches a constraint to the given constraint database. This includes setting up
     * all the internal data structures such as the watch literal lists.
     * @param db the database of constraints to insert the constraint into
     * @param constraint the constraint to add
     */
    void attachConstraint(ConstraintRef constraint, ConstraintClass constraintClass);

    /**
     * Tries to simplify the constraint database. Some simple checks are also
     * performed and the status is returned.
     * @return the status
     */
    void simplifyConstraintDatabase();

    /**
     * Eliminates a subset of learned constraints (both explanation and global cuts).
     */
    void reduceConstraintDatabase();

    /**
     * Remove the constraint.
     */
    void removeConstraint(ConstraintRef constraintRef, ConstraintClass constraintClass);

    /** The current status of the solver */
    SolverStatus d_status;

    /** The state encapsulating the state of all the variables */
    SolverState d_state;

    /**
     * Main routine. Performs a cut-and-search procedure and returns whether
     * the problem is satisfiable or not.
     * @return the status of the problem
     */
    SolverStatus search();

    /**
     * Performs propagation of values and bounds by consuming the part of the trail
     * that has not been traversed yet. The method returns whether a conflict has been
     * encountered during propagation. If a conflict has been encountered the constraints
     * responsible, and the variable in conflict are available in the solver state.
     */
    void propagate();

    /**
     * Backtrack to the given trail index.
     * @param backtrackIndex the index
     */
    void backtrack(int backtrackIndex);

    /**
     * Construct a tightly propagating constraint, for the bound type, using the bounds below trailIndex.
     * A tightly propagating constraint
     * ax + p >= b
     * such that
     * a divides (b - p) => hence it propagates the correct bound on x withouth rounding.
     *
     * @param x the variable that is propagated
     * @param trailIndex the index below which to look for explanantions
     * @param out_coefficients output coefficients
     */
    template<VariableModificationType type, bool replace>
    void getTighltyPropagatingConstraint(Variable x, unsigned trailIndex, constraint_coefficient_map& outCoefficients, Integer& outConstant);

    /** Gets the top variable currently on the trail of the constraint */
    Variable getTopVariable(ConstraintRef cosntraintRef);

    /** Return whether the constraint is in conflict at given level */
    bool isInConflict(const constraint_coefficient_map& coefficiets, const Integer& constant);

    /** Return the top variable in the constraint and the top trail index */
    void getTopTrailInfo(const constraint_coefficient_map& coefficients, Variable& topVariable, int& topTrailIndex);

    /** Variables involved in the conflict */
    std::map<Variable, double> d_conflictVariables;

    /** Constraints involved in the conflict */
    std::set<ConstraintRef> d_conflictConstraints;

    struct prop_variable_tag {
    	Variable variable;
    	unsigned lastModificationTime;
        VariableModificationType type;
    	prop_variable_tag(Variable variable, unsigned lastModificationTime, VariableModificationType type)
    	: variable(variable), lastModificationTime(lastModificationTime), type(type) {}
    	prop_variable_tag(const prop_variable_tag& other)
    	: variable(other.variable), lastModificationTime(other.lastModificationTime), type(other.type) {}
    	bool operator < (const prop_variable_tag& other) const {
    		if (lastModificationTime != other.lastModificationTime) {
    			return lastModificationTime < other.lastModificationTime;
    		}
    		if (variable != other.variable) {
    			return variable < other.variable;
    		}
    		return type < other.type;
    	}
    	bool operator == (const prop_variable_tag& other) const {
    		return type == other.type && variable == other.variable && lastModificationTime == other.lastModificationTime;
    	}
    };

    struct tight_cache_element {
    	constraint_coefficient_map coefficients;
    	Integer constant;
    };

    std::map<prop_variable_tag, tight_cache_element> d_tightConstraintCache;

    ConstraintRef assertTightConstraint(const constraint_coefficient_map& coefficients, Integer& constant);

    /**
     * Analyzes the current conflict. Adds the learned constraint and backtrack accordingly. New state might be
     * inconsistent at a lover level
     */
    void analyzeConflict();

    /**
     * Setup the coefficient map for the given variable at trail index, for the given modification type.
     * @param var
     * @param trailIndex
     * @param modificationType
     * @param coefficients
     * @param constant
     */
    void setUpConstraintMap(Variable var, unsigned trailIndex, VariableModificationType modificationType, constraint_coefficient_map& coefficients, Integer& constant);

    /**
     * Resolve the two constraints with FM over the variable var
     * @param var
     * @param lowerBoundCoefficients
     * @param cLower
     * @param upperBoundCoefficients
     * @param cUpper
     * @param resultCoefficients
     * @param resultConstant
     */
    void resolveCoefficientMaps(Variable var,
    		const constraint_coefficient_map& lowerBoundCoefficients, const Integer& cLower,
    		const constraint_coefficient_map& upperBoundCoefficients, const Integer& cUpper,
    		constraint_coefficient_map& resultCoefficients, Integer& resultConstant);



    /**
     * Decay the activity based heuristic data in the state.
     */
    void decayActivities();

    /**
     * Try and generate more global cuts.
     */
    void generateCuts();

    template<ConstraintType type>
    void printConstraint(const TypedConstraint<type>& constraint, std::ostream& out, OutputFormat format) const {
    	d_state.printConstraint<type>(constraint, out, format);
    }

    ConstraintRef assertClauseConstraint(std::vector<ClauseConstraintLiteral>& literals, ConstraintClass constraintClass);
    ConstraintRef assertCardinalityConstraint(std::vector<CardinalityConstraintLiteral>& literals, unsigned& c, ConstraintClass constraintClass);
    ConstraintRef assertIntegerConstraint(std::vector<IntegerConstraintLiteral>& literals, Integer& c, ConstraintClass constraintClass);

    void bumpConstraint(ConstraintRef constraintRef);

public:

    /**
     * Constructs a solver that will be using a given constraint manager.
     * @param cm the constraint manager
     */
    Solver(ConstraintManager& cm);

    /**
     * Returns the constraint manager this solver is using.
     * @return the constraint manager
     */
    ConstraintManager& getConstraintManagaer() const { return d_cm; }

    /**
     * Main public method responsible for solving a problem.
     * @return
     */
    SolverStatus solve();

    /**
     * Are we in conflict state.
     */
    bool inConflict() const {
    	return d_status == Unsatisfiable || d_state.inConflict();
    }

    /**
     * Creates a new variable of a given name (if any).
     * @param type the type of the variable
     * @param varName name of the variable (optional)
     * @return the variable reference
     */
    Variable newVariable(VariableType type, const char* varName = 0);

    /**
     * Get a reference to a variable that has been already defined.
     * @param varName the name of the variable.
     * @return the variable.
     */
    Variable getVariableByName(const char* varName) {
        return d_variableNameToVariable[varName];
    }

    /**
     * Returns true if the variable is Boolean at 0 level, i.e. 0 <= x <= 1.
     */
    bool isBoolean(Variable var) const;

    /**
     * Assert a new clause constraint, i.e. a constraint of the form
     * l1 || l2 || ... || l_n
     * @param literals the clause literals
     * @return the reference to the asserted constraint
     */
    ConstraintRef assertClauseConstraint(std::vector<ClauseConstraintLiteral>& literals) {
    	return assertClauseConstraint(literals, CONSTRAINT_CLASS_PROBLEM);
    }

    /**
     * Assert a new cardinality constraint, i.e. a constraint of the form
     * l_1 + l2 + \cdots + ln  \geq C. Terms li are boolean.
     * @param literals the integer literals (a_i x_i)
     * @param C the right hand side
     * @return the reference to the asserted constraint
     */
    ConstraintRef assertCardinalityConstraint(std::vector<CardinalityConstraintLiteral>& literals, unsigned& c) {
    	return assertCardinalityConstraint(literals, c, CONSTRAINT_CLASS_PROBLEM);
    }

    /**
     * Assert a new general integer inequality, i.e. a constraint of the form
     * a_1 x_1 + a_2 x_2 + \cdots + a_n x_n \geq C. Terms a_i x_i are literals.
     * @param literals the integer literals (a_i x_i)
     * @param C the right hand side
     * @return the reference to the asserted constraint
     */
    ConstraintRef assertIntegerConstraint(std::vector<IntegerConstraintLiteral>& literals, Integer& c) {
    	return assertIntegerConstraint(literals, c, CONSTRAINT_CLASS_PROBLEM);
    }

    inline bool hasUpperBound(Variable var) {
        return d_state.hasUpperBound(var);
    }

    /**
     * Sets a global upper bound for a given variable.
     * @param var the variable
     * @param bound the bound
     */
    template<VariableType type>
    void setUpperBound(Variable var, typename VariableTraits<type>::value_type bound) {
        assert(!hasUpperBound(var) || bound < getUpperBound<type>(var));
        if (d_status != Unsatisfiable) {
        	d_state.enqueueEvent<MODIFICATION_UPPER_BOUND_REFINE, type>(var, bound, ConstraintManager::NullConstraint);
        	propagate();
        }
    }

    inline bool hasLowerBound(Variable var) {
        return d_state.hasLowerBound(var);
    }

    /**
     * Returns the upper bound of the given variable.
     * @param var the variable
     * @return the upper bound
     */
    template<VariableType type>
    typename VariableTraits<type>::value_type getUpperBound(Variable var) {
        return d_state.getUpperBound<type>(var);
    }

    /**
     * Sets a global lower bound for a given variable.
     * @param var the variable
     * @param bound the bound
     */
    template<VariableType type>
    void setLowerBound(Variable var, typename VariableTraits<type>::value_type bound) {
        assert(!hasLowerBound(var) || bound > getLowerBound<type>(var));
        if (d_status != Unsatisfiable) {
        	d_state.enqueueEvent<MODIFICATION_LOWER_BOUND_REFINE, type>(var, bound, ConstraintManager::NullConstraint);
            propagate();
        }
    }

    /**
     * Returns the lower bound of the given variable.
     * @param var the variable
     * @return the lower bound
     */
    template<VariableType type>
    typename VariableTraits<type>::value_type getLowerBound(Variable var) {
        return d_state.getLowerBound<type>(var);
    }

    /**
     * Returns value of the given variable.
     * @param var the variable
     * @return the value
     */
    template<VariableType type>
    typename VariableTraits<type>::value_type getValue(Variable var) const {
        return d_state.getCurrentValue<type>(var);
    }

    /**
     * Returns all the variables in use.
     */
    const std::map<std::string, Variable>& getVariables() const {
        return d_variableNameToVariable;
    }

private:

    /** All the propagators */
    PropagatorCollection d_propagators;

    /**
     * Compute the bounds on a variable based on the current assignment.
     * @param variable
     */
    void computeBounds(Variable variable);

    /**
     * Output the constraint to the stream in SMT.
     */
    void printConstraintSMT(const std::vector<IntegerConstraintLiteral>& literals, const Integer& c, std::ostream& out);

    /** Whether to check the model */
    bool d_checkModel;

    /** Whether to disable propagation */
    bool d_disablePropagation;

    /** Whether to output the cuts to SMT */
    bool d_outputCuts;

    /** Output verbosity */
    Verbosity d_verbosity;

    Variable d_slackVariable;
    std::vector<ConstraintRef> d_slackConstraintsLower;
    std::vector<ConstraintRef> d_slackConstraintsUpper;

    std::vector<Variable> d_variablesToTrace;

    unsigned d_boundEstimate;

    int d_defaultBound;

    bool d_replaceVarsWithSlacks;

    bool d_tryFourierMotzkin;

    /** Check the model */
    void checkModel();

    /** Adds the slack variable and artificially bounds the unbounded variable var */
    void addSlackVariableBound(Variable var);

    /** Collects the garbage in the constraint manager */
    void collectGarbage();

public:

    void setCheckModel(bool flag) {
    	d_checkModel = flag;
    	if (d_verbosity >= VERBOSITY_BASIC_INFO) {
    		std::cout << "Model verification " << (flag ? "enabled" : "disabled") << std::endl;
    	}
    }

    void setPropagation(bool flag) {
    	d_disablePropagation = !flag;
    	if (d_verbosity >= VERBOSITY_BASIC_INFO) {
    		std::cout << "Propagation " << (flag ? "enabled." : "disabled.") << std::endl;
    	}
    }

    void setDynamicOrder(bool flag) {
    	d_state.setDynamicOrder(flag);
    	if (d_verbosity >= VERBOSITY_BASIC_INFO) {
    		std::cout << "Setting order to " << (flag ? "dynamic." : "linear.") << std::endl;
    	}
    }

    void setVerbosity(Verbosity verbosity) {
    	d_verbosity = verbosity;
    }

    void setBoundEstimate(unsigned bound) {
    	d_boundEstimate = bound;
    }

    void setDefaultBound(int bound) {
    	d_defaultBound = bound;
    }

    void setReplaceVarsWithSlacks(bool flag) {
    	d_replaceVarsWithSlacks = flag;
    	if (d_verbosity >= VERBOSITY_BASIC_INFO && flag) {
    		std::cout << "Replacing variables with the positive and negative slack." << std::endl;
    	}
    }

    void setTryFourierMotzkin(bool flag) {
        d_tryFourierMotzkin = flag;
        if (d_verbosity >= VERBOSITY_BASIC_INFO && flag) {
            std::cout << "Will try Fourier-Motzkin before dynamic cuts." << std::endl;
        }
    }

    void setOutputCuts(bool flag) {
        d_outputCuts = flag;
    }

    const SolverStats& getStatistics() const {
        return d_solverStats;
    }

    void printProblem(std::ostream& out, OutputFormat format, ConstraintRef implied = ConstraintManager::NullConstraint) const;

    void addVaribleToTrace(const char* varName) {
    	if (d_variableNameToVariable.find(varName) != d_variableNameToVariable.end()) {
    		d_variablesToTrace.push_back(d_variableNameToVariable[varName]);
    	}
    }

    void printConstraint(std::ostream& out, const constraint_coefficient_map& coeff) {
        constraint_coefficient_map::const_iterator it = coeff.begin();
        constraint_coefficient_map::const_iterator it_end = coeff.end();
        for (; it != it_end; ++ it) {
            if (it != coeff.begin()) {
                out << "+ ";
            }
            out << it->second << "*" << d_state.getVariableName(it->first) << " ";
        }
    }
};

inline std::ostream& operator << (std::ostream& out, const Solver::constraint_coefficient_map& coeff) {
	Solver::constraint_coefficient_map::const_iterator it = coeff.begin();
	Solver::constraint_coefficient_map::const_iterator it_end = coeff.end();
	for (; it != it_end; ++ it) {
		if (it != coeff.begin()) {
			out << "+ ";
		}
		out << it->second << "*" << it->first << " ";
	}
	return out;
}

} // End namespace cutsat

