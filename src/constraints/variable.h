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
#include <algorithm>

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <boost/integer_traits.hpp>

#include "util/config.h"
#include "constraints/number.h"

namespace cutsat {

/** Types of variables */
enum VariableType {
    /** An integer variable */
    TypeInteger,
    /** Rational variable */
    TypeRational
};

inline std::ostream& operator << (std::ostream& out, const VariableType& v) {
    switch(v) {
    case TypeInteger: out << "int"; break;
    case TypeRational: out << "rational"; break;
    default:
        assert(false);
    }
    return out;
}

class ConstraintManager;

/** Class representing the variables */
class Variable {

public:

    /** Number of bits reserved for the type information */
    static const unsigned s_typeBitsCount = 1;
    /** Number of bits reserved for the variable id */
    static const unsigned s_variableIdBitsCount = 31;
    /** Maximal id will represent the null variable */
    static const unsigned s_biggestVariableId = ((unsigned)(1 << s_variableIdBitsCount)) - 1;

    /** A variable should fit in 32 bits */
    BOOST_STATIC_ASSERT(s_typeBitsCount + s_variableIdBitsCount == 32);

    friend class ConstraintManager;

private:

    /** Type of the variable */
    unsigned d_type : s_typeBitsCount;

    /** Id if the variable */
    unsigned d_id   : s_variableIdBitsCount;

public:

    Variable()
    : d_type(TypeInteger), d_id(s_biggestVariableId) {}

    Variable(VariableType type, unsigned id)
    : d_type(type), d_id(id) {
    }

    inline unsigned getId() const { return d_id; }

    inline VariableType getType() const { return (VariableType) d_type; }

    bool operator == (const Variable& var) const {
        return d_id == var.d_id;
    }

    bool operator != (const Variable& var) const {
        return !(*this == var);
    }

    bool operator > (const Variable& var) const {
        return d_id > var.d_id;
    }

    bool operator < (const Variable& var) const {
        return d_id < var.d_id;
    }

    void print(std::ostream& out) const {
        out << d_id << ":" << (VariableType)d_type;
    }
};

static Variable VariableNull;

inline std::ostream& operator << (std::ostream& out, const Variable& var) {
    var.print(out); return out;
}

template<VariableType type>
struct VariableTraits;

template<>
struct VariableTraits<TypeInteger> {
    typedef Integer value_type;
};

template<>
struct VariableTraits<TypeRational> {
    typedef Rational value_type;
};

}
