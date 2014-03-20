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

#include "util/config.h"
#include "util/enums.h"
#include "constraints/variable.h"

#include <algorithm>

namespace cutsat {

class ConstraintManager;

/**
 * Types of constraint we allow
 */
enum ConstraintType {
    /** Clause constraints */
    ConstraintTypeClause = 0,
    /** Cardinality constraints */
    ConstraintTypeCardinality,
    /** Constraints with integer coefficients */
    ConstraintTypeInteger,
    /** Last constraint type */
    ConstraintTypeLast
};

/**
 * We distinguish and managed these types of constraints in different ways.
 */
enum ConstraintClass {
    /** Constraints coming from the input problem */
    CONSTRAINT_CLASS_PROBLEM,
    /** Constraints we got while explaining conflicts */
    CONSTRAINT_CLASS_EXPLANATION,
    /** Constraints we got while generating global cuts */
    CONSTRAINT_CLASS_GLOBAL_CUT
};


inline std::ostream& operator << (std::ostream& out, ConstraintType type) {
    switch (type) {
    case ConstraintTypeClause:
        out << "Clause";
        break;
    case ConstraintTypeCardinality:
        out << "Cardinality";
        break;
    case ConstraintTypeInteger:
        out << "Integer";
        break;
    default:
        assert(false);
    }
    return out;
}

inline std::ostream& operator << (std::ostream& out, ConstraintClass constraintClass) {
    switch (constraintClass) {
    case CONSTRAINT_CLASS_PROBLEM: out << "problem constraint"; break;
    case CONSTRAINT_CLASS_EXPLANATION: out << "explanation constraint"; break;
    case CONSTRAINT_CLASS_GLOBAL_CUT: out << "global cut constraint"; break;
    default:
        assert(false);
    }
    return out;
}

template<ConstraintType type> class Literal;

template<>
class Literal<ConstraintTypeClause> {

    unsigned d_negated  : 1;
    unsigned d_variable : Variable::s_variableIdBitsCount;

public:

    /** Default constructor for vectors and such */
    Literal() : d_negated(0), d_variable(0) {}

    Literal(const Variable& v, bool negated) :
        d_negated(negated), d_variable(v.getId()) {
    }

    Variable getVariable() const {
    	return Variable(TypeInteger, d_variable);
    }

    unsigned getValue(const Integer& variableValue) const {
    	if (d_negated == 0) return variableValue > 0 ? 1 : 0;
    	else return variableValue > 0 ? 0 : 1;
    }

    int getCoefficient() const {
    	if (d_negated) {
    		return -1;
    	} else {
    		return +1;
    	}
    }

    void print(std::ostream& out, OutputFormat format) const {
        switch (format) {
        case OutputFormatSmt:
        	if (d_negated == 0) {
        		out << "var[" << d_variable << "]";
        	} else {
        		out << "(~ var[" << d_variable << "])";
        	}
        	break;
        default:
        	if (d_negated == 0) {
        		out << "var[" << d_variable << "]";
        	} else {
        		out << "~var[" << d_variable << "]";
        	}
        	break;
        }
    }

    template<class VariableResolver>
    void print(std::ostream& out, const VariableResolver& resolver, OutputFormat format) const {
    	std::string name = resolver.getVariableName(getVariable());
    	switch (format) {
        case OutputFormatSmt:
        	if (d_negated == 0) {
        		out << name;
        	} else {
        		out << "(~ " << name << ")";
        	}
        	break;
        case OutputFormatCnf:
        	if (d_negated == 0) {
        		out << (d_variable + 1);
        	} else {
        		out << -(d_variable + 1);
        	}
        	break;
        default:
        	if (d_negated == 0) {
        		out << name;
        	} else {
        		out << "~" << name;
        	}
        	break;
        }
    }

    bool isNegated() const { return d_negated == 1; }

    void print(std::ostream& out) const {
        if (d_negated)
            out << "~";
        out << "var[" << d_variable << "]";
    }

    bool operator < (const Literal& lit) const {
        return d_variable < lit.d_variable || (d_variable == lit.d_variable && d_negated < lit.d_negated);
    }

    bool operator == (const Literal& lit) const {
        return d_variable == lit.d_variable && d_negated == lit.d_negated;
    }
};

template<>
class Literal<ConstraintTypeCardinality> {

    unsigned d_negated  : 1;
    unsigned d_variable : Variable::s_variableIdBitsCount;

public:

    /** Default constructor for vectors and such */
    Literal() : d_negated(0), d_variable(0) {}

    Literal(const Variable& v, bool negated) :
        d_negated(negated), d_variable(v.getId()) {
    }

    Variable getVariable() const {
    	return Variable(TypeInteger, d_variable);
    }

    unsigned getValue(const Integer& variableValue) const {
    	if (d_negated == 0) return variableValue > 0 ? 1 : 0;
    	else return variableValue > 0 ? 0 : 1;
    }

    int getCoefficient() const {
    	if (d_negated) {
    		return -1;
    	} else {
    		return +1;
    	}
    }

    void print(std::ostream& out, OutputFormat format) const {
        switch (format) {
        case OutputFormatSmt:
        	if (d_negated == 0) {
        		out << "var[" << d_variable << "]";
        	} else {
        		out << "(~ var[" << d_variable << "])";
        	}
        	break;
        default:
        	if (d_negated == 0) {
        		out << "var[" << d_variable << "]";
        	} else {
        		out << "~var[" << d_variable << "]";
        	}
        	break;
        }
    }

    template<class VariableResolver>
    void print(std::ostream& out, const VariableResolver& resolver, OutputFormat format) const {
    	std::string name = resolver.getVariableName(getVariable());
    	switch (format) {
        case OutputFormatSmt:
        	if (d_negated == 0) {
        		out << name;
        	} else {
        		out << "(~ " << name << ")";
        	}
        	break;
        case OutputFormatCnf:
        	if (d_negated == 0) {
        		out << (d_variable + 1);
        	} else {
        		out << -(d_variable + 1);
        	}
        	break;
        default:
        	if (d_negated == 0) {
        		out << name;
        	} else {
        		out << "~" << name;
        	}
        	break;
        }
    }

    bool isNegated() const { return d_negated == 1; }

    void print(std::ostream& out) const {
        if (d_negated)
            out << "~";
        out << "var[" << d_variable << "]";
    }

    bool operator < (const Literal& lit) const {
        return d_variable < lit.d_variable || (d_variable == lit.d_variable && d_negated < lit.d_negated);
    }

    bool operator == (const Literal& lit) const {
        return d_variable == lit.d_variable && d_negated == lit.d_negated;
    }
};

template<>
class Literal<ConstraintTypeInteger> {

    Integer d_coefficient;
    Variable d_variable;

public:

    Literal(): d_coefficient(0), d_variable() {}

    Literal(const Integer& coefficient, const Variable& variable) :
        d_coefficient(coefficient), d_variable(variable) {
    }

    Variable getVariable() const { return d_variable; }
    bool isNegated() const { return d_coefficient < 0; }

    inline Integer getValue(const Integer& variableValue) const {
    	return variableValue * d_coefficient;
    }

    const Integer& getCoefficient() const { return d_coefficient; }
    Integer& getCoefficient() { return d_coefficient; }

    void print(std::ostream& out, OutputFormat format) const {
        switch (format) {
        case OutputFormatSmt:
        	if (d_coefficient == 1) {
        		out << "var[" << d_variable.getId() << "]";
        	} else
        	if (d_coefficient == -1) {
        		out << "(~ var[" << d_variable.getId() << "])";
        	} else
        	if (d_coefficient >= 0) {
        		out << "(* " << d_coefficient << " var[" << d_variable.getId() << "])";
        	} else {
        		out << "(* (~ " << -d_coefficient << ") var[" << d_variable.getId() << "])";
        	}
        	break;
        default:
        	out << d_coefficient << "*var[" << d_variable.getId() << "]";
        }
    }

    template<class VariableResolver>
    void print(std::ostream& out, const VariableResolver& resolver, OutputFormat format) const {
    	std::string name = resolver.getVariableName(getVariable());
    	switch (format) {
        case OutputFormatSmt:
        	if (d_coefficient == 1) {
        		out << name;
        	} else
        	if (d_coefficient == -1) {
        		out << "(~ " << name << ")";
        	} else
        	if (d_coefficient >= 0) {
        		out << "(* " << d_coefficient << " " << name << ")";
        	} else {
        		out << "(* (~ " << -d_coefficient << ") " << name << ")";
        	}
        	break;
        default:
        	out << d_coefficient << "*var[" << d_variable.getId() << "]";
        }
    }

    bool operator < (const Literal& lit) const {
        return getVariable().getId() < lit.getVariable().getId() || (getVariable().getId() == lit.getVariable().getId() && getCoefficient() < lit.getCoefficient());
    }

    bool operator == (const Literal& lit) const {
        return getVariable() == lit.getVariable() && getCoefficient() == lit.getCoefficient();
    }
};

template<ConstraintType type>
std::ostream& operator << (std::ostream& out, const Literal<type>& literal) {
    literal.print(out, OutputFormatIlp);
    return out;
}

typedef Literal<ConstraintTypeClause> ClauseConstraintLiteral;
typedef Literal<ConstraintTypeCardinality> CardinalityConstraintLiteral;
typedef Literal<ConstraintTypeInteger> IntegerConstraintLiteral;

template<ConstraintType type> struct ConstraintTraits;

struct empty_data {};

inline std::ostream& operator << (std::ostream& out, const empty_data&) {
	return out;
}

template<>
struct ConstraintTraits<ConstraintTypeClause> {
    static size_t minLiterals()     { return 2; }

    static const VariableType variableType = TypeInteger;

    typedef ClauseConstraintLiteral literal_type;
    typedef boost::int32_t constant_type;
    typedef boost::int32_t literal_value_type;
    typedef empty_data additional_data;
};

template<>
struct ConstraintTraits<ConstraintTypeCardinality> {

    static size_t minLiterals()    { return 2; }

    static const VariableType variableType = TypeInteger;

    typedef CardinalityConstraintLiteral literal_type;
    typedef boost::uint32_t constant_type;
    typedef boost::int32_t literal_value_type;
    typedef empty_data additional_data;
};

template<>
struct ConstraintTraits<ConstraintTypeInteger> {
    static size_t minLiterals()    { return 2;    }

    static const VariableType variableType = TypeInteger;

    typedef IntegerConstraintLiteral literal_type;
    typedef Integer constant_type;
    typedef Integer literal_value_type;

    typedef empty_data additional_data;
};

class ConstraintManager;

template<ConstraintType constraintType>
class TypedConstraint {

public:

    /** Constraint traits */
    typedef ConstraintTraits<constraintType> constraint_traits;
    /** Type of the constants */
    typedef typename constraint_traits::constant_type constant_type;
    /** Type of the literals */
    typedef typename constraint_traits::literal_type literal_type;
    /** Type of the additional data */
    typedef typename constraint_traits::additional_data additional_data;

private:

    friend class ConstraintManager;

    /** Is this a learned constraint, or a problem constraint */
    unsigned d_learnt  :  1;
    /** Did this constraint get deleted */
    unsigned d_deleted :  1;
    /** The size of the constraint */
    unsigned d_size    : 31;
    /** Number of times this constraint is currently used as an explanation */
    unsigned d_users   : 31;

    /** The score of the constraint for removing clauses */
    float d_score;

    /** The constant */
    constant_type d_constant;

    /** The additional data */
    additional_data d_additional_data;

    /* The literals  */
    literal_type d_literals[];

public:

    /** Constructor with constant */
    TypedConstraint(const std::vector<literal_type>& lits, const constant_type& constant, bool learnt)
    : d_learnt(learnt ? 1 : 0), d_deleted(0), d_size(lits.size()), d_users(0), d_score(0), d_constant(constant) {
        assert(lits.size() >= constraint_traits::minLiterals());
        assert(lits.size() == d_size); // Overflow check
        for (unsigned i = 0; i < lits.size(); ++i) {
            new (d_literals + i) literal_type(lits[i]);
        }
    }

    /** Destructor */
    ~TypedConstraint() {
    	for (unsigned i = 0; i < d_size; ++i) {
    		d_literals[i].~literal_type();
    	}
    }

    void setScore(double value) {
        d_score = value;
    }

    double getScore() const {
        return d_score;
    }

    bool inUse() const {
        return d_users > 0;
    }

    void addUser() {
    	assert(!isDeleted());
        ++ d_users;
        assert(d_users > (d_users >> 1)); // Overflow check
    }

    void removeUser() {
        assert(d_users > 0);
        -- d_users;
    }


    TypedConstraint(const TypedConstraint& other) {
        assert(false);
    }

    TypedConstraint& operator = (const TypedConstraint& other) {
        assert(false);
    }

    /**
     * Returns the size of the constraint (whatever that means for the constraint type).
     */
    size_t getSize() const {
        return (size_t) d_size;
    }

    /**
     * Returns whether the constraint has been learnt.
     */
    bool isLearnt() const {
        return d_learnt == 1;
    }

    /**
     * Sets the constraint to be learnt.
     */
    void setLearnt(bool learnt = true) {
        d_learnt = learnt;
    }

    /**
     * Returns whether the constraint has been deleted.
     */
    bool isDeleted() const {
        return d_deleted == 1;
    }

    /**
     * Sets the constraint as deleted or not.
     */
    void setDeleted(bool deleted = true) {
        d_deleted = deleted ? 1 : 0;
    }

    /**
     * Return the type of the constraint.
     */
    ConstraintType getConstraintType() const {
        return constraintType;
    }

    /**
     * Returns the literal at specified position.
     */
    literal_type& getLiteral(size_t index);

    /**
     * Returns the literal at specified position.
     */
    const literal_type& getLiteral(size_t index) const;

    /**
     * Returns the constant
     */
    constant_type& getConstant();

    /**
     * Returns the constant
     */
    const constant_type& getConstant() const;

    /** Prints the constraint to the stream */
    void print(std::ostream& out, OutputFormat format) const {
        out << constraintType << "[";
        for (unsigned i = 0; i < d_size; ++ i) {
            if (i > 0) out << ",";
            out << d_literals[i];
        }
        out << ":" << d_constant << "]";
    }

    /** Prints the constraint to the stream */
    template<class VariableResolver>
    void print(std::ostream& out, const VariableResolver& resolver, OutputFormat format) const {
        if (format == OutputFormatSmt) {
        	out << "(>= (+";
            for (unsigned i = 0; i < d_size; ++ i) {
            	out << " ";
                d_literals[i].print(out, resolver, format);
            }
            if (d_constant >= 0) {
            	out << ") " << d_constant << ")";
            } else {
            	out << ") (~ " << -d_constant << "))";
            }
            return;
        }
        if (format == OutputFormatCnf) {
        	for (unsigned i = 0; i < d_size; ++ i) {
        		d_literals[i].print(out, resolver, format);
        		out << " ";
        	}
        	out << " 0";
        	return;
        }
    	out << constraintType << "[";
        for (unsigned i = 0; i < d_size; ++ i) {
            if (i > 0) out << " + ";
            d_literals[i].print(out, resolver, format);
        }
        out << " >= " << d_constant << "]";
    }

    void swapLiterals(unsigned i, unsigned j) {
        assert(i < d_size && j < d_size);
        if (i == j) return;
        literal_type tmp = d_literals[i];
        d_literals[i] = d_literals[j];
        d_literals[j] = tmp;
    }

    additional_data& getAdditionalData() {
    	return d_additional_data;
    }

    const additional_data& getAdditionalData() const {
    	return d_additional_data;
    }

    template<typename T>
    void sort(const T& cmp) {
    	std::sort(d_literals, d_literals + d_size, cmp);
    }

    /**
     * Check if the constraint is satisfied in the given state.
     * @param state
     * @return true if the constraint is satisfied
     */
    template <class State>
    bool isSatisfied(const State& state) const;
};

template<ConstraintType constraintType>
std::ostream& operator << (std::ostream& out, const TypedConstraint<constraintType>& constraint) {
    constraint.print(out, OutputFormatIlp);
    return out;
}

template<ConstraintType constraintType>
typename TypedConstraint<constraintType>::literal_type& TypedConstraint<constraintType>::getLiteral(size_t i) {
    assert(i < d_size);
    return d_literals[i];
}

template<ConstraintType constraintType>
typename TypedConstraint<constraintType>::literal_type const& TypedConstraint<constraintType>::getLiteral(size_t i) const {
    assert(i < d_size);
    return d_literals[i];
}

template<ConstraintType constraintType>
typename TypedConstraint<constraintType>::constant_type& TypedConstraint<constraintType>::getConstant() {
    return d_constant;
}

template<ConstraintType constraintType>
typename TypedConstraint<constraintType>::constant_type const & TypedConstraint<constraintType>::getConstant() const {
    return d_constant;
}

template<>
template<class State>
bool TypedConstraint<ConstraintTypeClause>::isSatisfied(const State& state) const {
    // Find a true literal
    for (unsigned i = 0; i < d_size; ++ i) {
        if (state.template getCurrentValue<ConstraintTypeClause>(d_literals[i]) == 1) return true;
    }
    return false;
}

template<>
template<class State>
bool TypedConstraint<ConstraintTypeCardinality>::isSatisfied(const State& state) const {
    // Count the number of true literals
	unsigned trueLiterals = 0;
    for (unsigned i = 0; i < d_size; ++ i) {
        if (state.template getCurrentValue<ConstraintTypeCardinality>(d_literals[i]) == 1) {
        	trueLiterals ++;
        }
    }
    return trueLiterals >= d_constant;
}

template<>
template<class State>
bool TypedConstraint<ConstraintTypeInteger>::isSatisfied(const State& state) const {
    CUTSAT_TRACE("constraints::check") << *this << std::endl;
	Integer sum = 0;
    for (unsigned i = 0; i < d_size; ++ i) {
        sum += state.template getCurrentValue<ConstraintTypeInteger>(d_literals[i]);
    }
    CUTSAT_TRACE("constraints::check") << "LHS: " << sum << ", RHS: " << d_constant << std::endl;
    return sum >= d_constant;
}

typedef TypedConstraint<ConstraintTypeClause> ClauseConstraint;
typedef TypedConstraint<ConstraintTypeCardinality> CardinalityConstraint;
typedef TypedConstraint<ConstraintTypeInteger> IntegerConstraint;

}
